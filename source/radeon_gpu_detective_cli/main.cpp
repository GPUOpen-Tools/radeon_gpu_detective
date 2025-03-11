//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  main entry point of RGD CLI.
//=============================================================================

// C++.
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <vector>

// Local.
#include "rgd_asic_info.h"
#include "rgd_data_types.h"
#include "rgd_utils.h"
#include "rgd_parsing_utils.h"
#include "rgd_serializer.h"
#include "rgd_serializer_json.h"
#include "rgd_resource_info_serializer.h"
#include "rgd_version_info.h"
#include "rgd_exec_marker_tree_serializer.h"
#include "rgd_enhanced_crash_info_serializer.h"

// cxxopts.
#include "cxxopts.hpp"

// RDF.
#include "rdf/rdf/inc/amdrdf.h"

// System info.
#include "system_info_utils/source/system_info_reader.h"

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

// Return true if the user input is valid, and false otherwise.
static bool IsInputValid(const Config& user_config)
{
    bool ret = false;
    if (!user_config.crash_dump_file.empty())
    {
        ret = RgdUtils::IsFileExists(user_config.crash_dump_file);
        if (!ret)
        {
            std::cerr << "ERROR: input file does not exist: " << user_config.crash_dump_file << std::endl;
        }
    }
    if (ret && !user_config.output_file_txt.empty())
    {
        ret = RgdUtils::IsValidFilePath(user_config.output_file_txt);
        if (!ret)
        {
            std::cerr << "ERROR: invalid output file path: " << user_config.output_file_txt << std::endl;
        }
    }
    if (ret && !user_config.output_file_json.empty())
    {
        ret = RgdUtils::IsValidFilePath(user_config.output_file_json);
        if (!ret)
        {
            std::cerr << "ERROR: invalid output file path: " << user_config.output_file_txt << std::endl;
        }
    }
    return ret;
}

static void ParseApiInfoChunk(rdf::ChunkFile& chunk_file, TraceChunkApiInfo& api_info, bool is_verbose)
{
    const char* kChunkApiInfo = "ApiInfo";
    const int64_t kChunkCount = chunk_file.GetChunkCount(kChunkApiInfo);

    // Parse if ApiInfo chunk is present in the file. ApiChunk will not be present for the files captured with RDP 2.12 and before.
    if (kChunkCount > 0)
    {
        // Only one ApiInfo chunk is expected so chunk index is set to 0 (first chunk).
        assert(kChunkCount == 1);
        const int64_t kChunkApiIdx = 0;
        uint64_t payload_size = chunk_file.GetChunkDataSize(kChunkApiInfo, kChunkApiIdx);
        assert(payload_size > 0);
        if (payload_size > 0)
        {
            chunk_file.ReadChunkDataToBuffer(kChunkApiInfo, kChunkApiIdx, (void*)&api_info);
        }
        else
        {
            RgdUtils::PrintMessage("invalid chunk data size for ApiInfo chunk. Capture API type information is not available.", RgdMessageType::kError, true);
        }
    }
    else
    {
        RgdUtils::PrintMessage("ApiInfo chunk not found.", RgdMessageType::kInfo, is_verbose);
    }
}

static bool ParseCrashDump(const Config& user_config, RgdCrashDumpContents& contents)
{
    std::cout << "Parsing crash dump file..." << std::endl;

    // Read and parse the RDF file.
    auto         file = rdf::Stream::OpenFile(user_config.crash_dump_file.c_str());

    std::string error_msg;
    bool ret = false;
    bool is_system_info_parsed = false;
    bool is_driveroverrides_parsed = false;
    bool is_codeobject_db_parsed = false;
    bool is_codeobject_loader_events_parsed = false;
    bool is_pso_correlations_parsed         = false;

    try
    {
        rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

        // Parse the UMD and KMD crash data chunk.
        const char* kChunkCrashData = "DDEvent";
        ret = RgdParsingUtils::ParseCrashDataChunks(chunk_file, kChunkCrashData, contents.umd_crash_data, contents.kmd_crash_data, error_msg);

        // Parse System Info chunk.
        is_system_info_parsed = system_info_utils::SystemInfoReader::Parse(chunk_file, contents.system_info);
        if (is_system_info_parsed)
        {
            ecitrace::GpuSeries gpu_series = ecitrace::GpuSeries::kUnknown;
            for (auto& gpu_info : contents.system_info.gpus)
            {
                // Get the asic family and e revision from system info.
                uint32_t asic_family = gpu_info.asic.id_info.family;
                uint32_t asic_e_rev  = gpu_info.asic.id_info.e_rev;

                gpu_series = ecitrace::AsicInfo::GetGpuSeries(asic_family, asic_e_rev);

                // Check if supported GPU series found.
                if (gpu_series != ecitrace::GpuSeries::kUnknown && gpu_series != ecitrace::GpuSeries::kNavi1)
                {
                    contents.gpu_series = gpu_series;
                    break;
                }
            }
        }
        // If ApiInfo chunk is available, parse chunk.
        ParseApiInfoChunk(chunk_file, contents.api_info, user_config.is_verbose);

        // Parse TraceProcessInfo chunk.
        RgdParsingUtils::ParseTraceProcessInfoChunk(chunk_file, kChunkIdTraceProcessInfo, contents.crashing_app_process_info);

        // Parse the 'DriverOverrides' chunk.
        is_driveroverrides_parsed = RgdParsingUtils::ParseDriverOverridesChunk(chunk_file, kChunkIdDriverOverrides, contents.driver_experiments_json);

        // Parse the 'CodeObject' chunk.
        is_codeobject_db_parsed = RgdParsingUtils::ParseCodeObjectChunk(chunk_file, kChunkIdCodeObject, contents.code_objects_map);

        // Parse the 'COLoadEvent' chunk.
        is_codeobject_loader_events_parsed = RgdParsingUtils::ParseCodeObjectLoadEventChunk(chunk_file, kChunkIdCOLoadEvent, contents.code_object_load_events);

        // Parse the 'PsoCorrelation' chunk.
        is_pso_correlations_parsed = RgdParsingUtils::PsoCorrelationChunk(chunk_file, kChunkIdPsoCorrelation, contents.pso_correlations);

    }
    catch (const std::exception& e)
    {
        std::stringstream error_txt;
        error_txt << " (" << e.what() << ")";
        error_msg += error_txt.str();
    }

    // This will hold the summary contents.
    std::stringstream txt;

    if (!ret)
    {
        std::stringstream err;
        err << "could not parse input file " << user_config.crash_dump_file << error_msg << std::endl;
        RgdUtils::PrintMessage(err.str().c_str(), RgdMessageType::kError, user_config.is_verbose);
    }
    else
    {
        RgdUtils::PrintMessage("crash data parsed successfully.", RgdMessageType::kInfo, user_config.is_verbose);

        // Build the command buffer ID mapping.
        ret = RgdParsingUtils::BuildCommandBufferMapping(user_config, contents.umd_crash_data, contents.cmd_buffer_mapping);
        assert(ret);
        if (ret)
        {
            RgdUtils::PrintMessage("command buffer mapping built successfully.", RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            RgdUtils::PrintMessage("failed to build command buffer mapping.", RgdMessageType::kError, user_config.is_verbose);
        }

        assert(is_system_info_parsed);
        if (is_system_info_parsed && !contents.system_info.cpus.empty())
        {
            RgdUtils::PrintMessage("system information parsed successfully.", RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            std::cerr << "ERROR: failed to parse system information contents in crash dump file." << std::endl;
        }

        assert(is_driveroverrides_parsed);
        if (is_driveroverrides_parsed)
        {
            RgdUtils::PrintMessage("driver experiments information parsed successfully.", RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            std::cerr << "ERROR: failed to parse DriverOverrides chunk in crash dump file." << std::endl;
        }

        // Done parsing the file here.
        file.Close();

        if (is_system_info_parsed && ret)
        {
            std::cout << "Crash dump file parsed successfully." << std::endl;
        }
        else
        {
            std::cout << "Failed to parse crash dump file." << std::endl;
        }
    }
    return ret;
}

// Generate the text analysis output (to either file or console).
static void SerializeTextOutput(const RgdCrashDumpContents&     contents,
                                const Config&                   user_config,
                                RgdResourceInfoSerializer&      resource_serializer,
                                RgdEnhancedCrashInfoSerializer& enhanced_crash_info_serializer)
{
    bool is_ok = false;
    std::stringstream txt;

    std::string input_info_str;
    RgdSerializer::InputInfoToString(user_config, contents.crashing_app_process_info, contents.system_info, contents.api_info, input_info_str);
    txt << input_info_str;

    std::string system_info_str;
    RgdSerializer::ToString(user_config, contents.system_info, contents.driver_experiments_json, system_info_str);
    txt << system_info_str << std::endl;

    std::unordered_map<uint64_t, RgdCrashingShaderInfo> in_flight_shader_api_pso_hashes_to_shader_info_;
    enhanced_crash_info_serializer.GetInFlightShaderApiPsoHashes(in_flight_shader_api_pso_hashes_to_shader_info_);

    std::cout << "Generating text representation of the execution marker information..." << std::endl;
    ExecMarkerDataSerializer exec_marker_serializer(in_flight_shader_api_pso_hashes_to_shader_info_);

    RgdUtils::PrintMessage("generating text representation of the list of markers in progress...", RgdMessageType::kInfo, user_config.is_verbose);
    std::string exec_marker_summary;
    is_ok = exec_marker_serializer.GenerateExecutionMarkerSummaryList(user_config, contents.umd_crash_data,
        contents.cmd_buffer_mapping, exec_marker_summary);
    assert(is_ok);
    txt << std::endl << std::endl;
    txt << "===================" << std::endl;
    txt << "MARKERS IN PROGRESS" << std::endl;
    txt << "===================" << std::endl << std::endl;
    txt << exec_marker_summary;

    if (is_ok)
    {
        bool is_marker_list_empty = (exec_marker_summary.find("INFO:") == 0);
        if (!is_marker_list_empty)
        {
            RgdUtils::PrintMessage("text representation of the list of markers in progress generated successfully.",
                RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            RgdUtils::PrintMessage(kStrInfoNoCmdBuffersInFlight, RgdMessageType::kInfo, user_config.is_verbose);
        }
    }
    else
    {
        RgdUtils::PrintMessage("failed to generate text representation of the list of markers in progress.", RgdMessageType::kError, user_config.is_verbose);
    }

    RgdUtils::PrintMessage("generating text representation of the execution marker tree...", RgdMessageType::kInfo, user_config.is_verbose);

    // Generate the execution marker tree.
    std::string exec_marker_tree;
    is_ok = exec_marker_serializer.GenerateExecutionMarkerTree(user_config, contents.umd_crash_data,
        contents.cmd_buffer_mapping, exec_marker_tree);
    assert(is_ok);
    txt << std::endl << std::endl;
    txt << "=====================" << std::endl;
    txt << "EXECUTION MARKER TREE" << std::endl;
    txt << "=====================" << std::endl << std::endl;

    bool is_empty_tree = (exec_marker_tree.find("INFO:") == 0);
    if (is_ok)
    {
        if (!is_empty_tree)
        {
            txt << "Legend" << std::endl;
            txt << "======" << std::endl;
            txt << "[X] finished" << std::endl;
            txt << "[>] in progress" << std::endl;
            txt << "[#] shader in flight" << std::endl;
            txt << "[ ] not started" << std::endl << std::endl;

            RgdUtils::PrintMessage("text representation of the execution marker tree generated successfully.",
                RgdMessageType::kInfo, user_config.is_verbose);
        }
        else if (!user_config.is_verbose)
        {
            // In verbose mode, we would have already printed this message while trying to generate the marker summary.
            RgdUtils::PrintMessage(kStrInfoNoCmdBuffersInFlight, RgdMessageType::kInfo, user_config.is_verbose);
        }
    }
    else
    {
        RgdUtils::PrintMessage("failed to generate text representation of execution tree.", RgdMessageType::kError, user_config.is_verbose);
    }

    // Serialize the tree information or related messages into the output file.
    txt << exec_marker_tree;

    if (user_config.is_raw_event_data)
    {
        // Print UMD data for debugging purposes.
        txt << std::endl << std::endl;
        txt << "==============" << std::endl;
        txt << "UMD CRASH DATA" << std::endl;
        txt << "==============" << std::endl << std::endl;
        txt << RgdSerializer::CrashAnalysisTimeInfoToString(contents.umd_crash_data.time_info) << std::endl;
        txt << RgdSerializer::SerializeUmdCrashEvents(contents.umd_crash_data.events) << std::endl;
        txt << std::endl << std::endl;

        // Print KMD data for debugging purposes.
        txt << std::endl << std::endl;
        txt << "==============" << std::endl;
        txt << "KMD CRASH DATA" << std::endl;
        txt << "==============" << std::endl << std::endl;
        txt << RgdSerializer::CrashAnalysisTimeInfoToString(contents.kmd_crash_data.time_info) << std::endl;
        txt << RgdSerializer::SerializeKmdCrashEvents(contents.kmd_crash_data.events) << std::endl;

        // Print the code object load events.
        txt << std::endl << std::endl;
        txt << "=======================" << std::endl;
        txt << "CODE OBJECT LOAD EVENTS" << std::endl;
        txt << "=======================" << std::endl << std::endl;
        txt << RgdSerializer::CodeObjectLoadEventsToString(contents.code_object_load_events) << std::endl;

        // Print the code object database.
        txt << std::endl << std::endl;
        txt << "====================" << std::endl;
        txt << "CODE OBJECT DATABASE" << std::endl;
        txt << "====================" << std::endl << std::endl;
        txt << RgdSerializer::CodeObjectsToString(contents.code_objects_map) << std::endl;

        // Print the PSO correlations.
        txt << std::endl << std::endl;
        txt << "================" << std::endl;
        txt << "PSO CORRELATIONS" << std::endl;
        txt << "================" << std::endl << std::endl;
        txt << RgdSerializer::PsoCorrelationsToString(contents.pso_correlations) << std::endl;
    }

    // Retrieve indices of array contents.kmd_crash_data.events for all KMD DEBUG NOP events which tell us about the offending VAs.
    std::vector<size_t> debug_nop_events;
    std::vector<size_t> page_fault_events;
    for(size_t i = 0, count = contents.kmd_crash_data.events.size(); i < count; ++i)
    {
        const RgdEventOccurrence& current_event = contents.kmd_crash_data.events[i];
        assert(current_event.rgd_event != nullptr);
        if (current_event.rgd_event != nullptr)
        {
            const RgdEvent& rgd_event = *current_event.rgd_event;
            if (rgd_event.header.eventId == uint8_t(KmdEventId::RgdEventVmPageFault))
            {
                // Page fault events for page fault analysis.
                page_fault_events.push_back(i);
            }
        }
    }
    std::cout << "Text representation of the execution marker information generated successfully." << std::endl;

    // Page fault information.
    std::cout << "Analyzing page fault information for text representation..." << std::endl;
    RgdUtils::PrintMessage("generating text representation of the page fault information...", RgdMessageType::kInfo, user_config.is_verbose);

    txt << std::endl << std::endl;
    txt << "==================" << std::endl;
    txt << "PAGE FAULT SUMMARY" << std::endl;
    txt << "==================" << std::endl << std::endl;

    if (!page_fault_events.empty())
    {
        for (size_t page_fault_event_index : page_fault_events)
        {
            const RgdEventOccurrence& page_fault_event = contents.kmd_crash_data.events[page_fault_event_index];

            // Retrieve virtual address info.
            const VmPageFaultEvent& curr_event = static_cast<const VmPageFaultEvent&>(*page_fault_event.rgd_event);
            txt << "Offending VA: 0x" << std::hex << curr_event.faultVmAddress << std::dec << std::endl << std::endl;

            if (curr_event.faultVmAddress != kVaReserved)
            {
                std::string resource_info_string;
                is_ok = resource_serializer.GetVirtualAddressHistoryInfo(user_config, curr_event.faultVmAddress, resource_info_string);

                assert(is_ok);

                bool is_resource_info_empty = (resource_info_string.find("INFO:") != std::string::npos);
                txt << resource_info_string;
                if (is_ok)
                {
                    if (!is_resource_info_empty)
                    {
                        RgdUtils::PrintMessage("text representation of the page fault information generated successfully.",
                            RgdMessageType::kInfo, user_config.is_verbose);
                    }
                    else
                    {
                        std::stringstream console_msg;
                        console_msg << "no resources associated with the faulting virtual address: 0x" << std::hex << curr_event.faultVmAddress << std::dec << ".";
                        RgdUtils::PrintMessage(console_msg.str().c_str(), RgdMessageType::kInfo, user_config.is_verbose);
                    }
                }
                else
                {
                    txt << "ERROR: failed to generate resource information and timeline for the offending VA." << std::endl << std::endl;
                    RgdUtils::PrintMessage("failed to generate text representation of the page fault information.", RgdMessageType::kError, user_config.is_verbose);
                }
            }
            else
            {
                txt << "INFO: no resources associated with virtual address 0x0." << std::endl << std::endl;
            }
        }
    }
    else
    {
        txt << "INFO: no page fault detected." << std::endl;
    }

    txt << std::endl << std::endl;
    txt << "===========" << std::endl;
    txt << "SHADER INFO" << std::endl;
    txt << "===========" << std::endl << std::endl;
    std::string shader_info_text;
    enhanced_crash_info_serializer.GetInFlightShaderInfo(user_config, shader_info_text);
    const char* kStrNoShaderInfoAvailable = "INFO: no information available about in-flight shaders.";
    if (shader_info_text.empty())
    {
        txt << kStrNoShaderInfoAvailable << std::endl;
    }
    else
    {
        txt << shader_info_text << std::endl;
    }

    if (user_config.is_all_disassembly)
    {
        txt << std::endl;
        txt << "================" << std::endl;
        txt << "CODE OBJECT INFO" << std::endl;
        txt << "================" << std::endl << std::endl;
        txt << "This section includes the complete disassembly of all Code Object binaries that had at least one shader in flight during the crash."
            << std::endl
            << "You can use the Shader info ID handle to correlate what shader was part of what Code Object." << std::endl << std::endl;
        std::string complete_disassembly_text;
        is_ok = enhanced_crash_info_serializer.GetCompleteDisassembly(user_config, complete_disassembly_text);

        if (is_ok && !complete_disassembly_text.empty())
        {
            txt << complete_disassembly_text << std::endl;
        }
        else
        {
            txt << kStrNoShaderInfoAvailable << std::endl;  
        }
    }

    if (user_config.is_all_resources)
    {
        txt << std::endl << std::endl;
        txt << "=========================" << std::endl;
        txt << "COMPLETE RESOURCE HISTORY" << std::endl;
        txt << "=========================" << std::endl << std::endl;
        uint64_t virtual_address = kVaReserved;
        std::string resource_info_string;
        resource_serializer.GetVirtualAddressHistoryInfo(user_config, virtual_address, resource_info_string);
        txt << resource_info_string << std::endl;
    }

    std::cout << "Page fault information analysis for the text representation completed." << std::endl;

    // Write the output to a file if required, otherwise print to console.
    if (!user_config.output_file_txt.empty())
    {
        RgdUtils::WriteTextFile(user_config.output_file_txt, txt.str());
    }
    else
    {
        std::cout << txt.str();
    }
}

static bool PerformCrashAnalysis(const Config& user_config)
{
    RgdCrashDumpContents contents;
    bool ret = ParseCrashDump(user_config, contents);

    // True if the output we are required to produce is in text format (file or console).
    bool is_text_required = !user_config.output_file_txt.empty() || user_config.output_file_json.empty();
    bool is_json_required = !user_config.output_file_json.empty();

    RgdResourceInfoSerializer resource_serializer;
    resource_serializer.InitializeWithTraceFile(user_config.crash_dump_file);

    RgdEnhancedCrashInfoSerializer enhanced_crash_info_serializer;
    enhanced_crash_info_serializer.Initialize(contents, RgdParsingUtils::GetIsPageFault());
    
    if (ret && is_text_required)
    {
        SerializeTextOutput(contents, user_config, resource_serializer, enhanced_crash_info_serializer);
    }
    if (ret && is_json_required)
    {
        // JSON.
        RgdSerializerJson serializer_json;
        serializer_json.SetInputInfo(user_config, contents.crashing_app_process_info, contents.system_info, contents.api_info);

        serializer_json.SetSystemInfoData(user_config, contents.system_info);
        serializer_json.SetDriverExperimentsInfoData(contents.driver_experiments_json);

        std::unordered_map<uint64_t, RgdCrashingShaderInfo> in_flight_shader_api_pso_hashes_to_shader_info_;
        enhanced_crash_info_serializer.GetInFlightShaderApiPsoHashes(in_flight_shader_api_pso_hashes_to_shader_info_);

        ExecMarkerDataSerializer exec_marker_serializer(in_flight_shader_api_pso_hashes_to_shader_info_);

        std::cout << "Generating JSON representation of the execution marker information..." << std::endl;

        serializer_json.SetExecutionMarkerTree(user_config,
            contents.umd_crash_data,
            contents.cmd_buffer_mapping,
            exec_marker_serializer);

        serializer_json.SetExecutionMarkerSummaryList(user_config,
            contents.umd_crash_data,
            contents.cmd_buffer_mapping,
            exec_marker_serializer);

        std::cout << "JSON representation of the execution marker information generated successfully." << std::endl;

        if (user_config.is_raw_event_data)
        {
            serializer_json.SetUmdCrashData(contents.umd_crash_data);
            serializer_json.SetKmdCrashData(contents.kmd_crash_data);
        }

        std::cout << "Analyzing page fault information for the JSON representation..." << std::endl;

        // Retrieve indices of array contents.kmd_crash_data.events for all KMD DEBUG NOP events which tell us about the offending VAs.
        std::vector<size_t> debug_nop_events;
        std::vector<size_t> page_fault_events;
        for(size_t i = 0, count = contents.kmd_crash_data.events.size(); i < count; ++i)
        {
            const RgdEventOccurrence& current_event = contents.kmd_crash_data.events[i];
            assert(current_event.rgd_event != nullptr);
            if (current_event.rgd_event != nullptr)
            {
                const RgdEvent& rgd_event = *current_event.rgd_event;
                if (rgd_event.header.eventId == uint8_t(KmdEventId::RgdEventVmPageFault))
                {
                    // Page fault events for page fault analysis.
                    page_fault_events.push_back(i);
                }
            }
        }

        for (size_t page_fault_event_index : page_fault_events)
        {
            const RgdEventOccurrence& page_fault_event = contents.kmd_crash_data.events[page_fault_event_index];

            // Retrieve virtual address info.
            const VmPageFaultEvent& curr_event = static_cast<const VmPageFaultEvent&>(*page_fault_event.rgd_event);

            if (curr_event.faultVmAddress != kVaReserved)
            {
                serializer_json.SetVaResourceData(resource_serializer, user_config, curr_event.faultVmAddress);
            }
        }

        // Set shader info.
        serializer_json.SetShaderInfo(user_config, enhanced_crash_info_serializer);

        if (user_config.is_all_resources)
        {
            uint64_t virtual_address = kVaReserved;
            serializer_json.SetVaResourceData(resource_serializer, user_config, virtual_address);
        }

        std::cout << "Page fault information analysis for the JSON representation completed." << std::endl;

        // Save the JSON file.
        serializer_json.SaveToFile(user_config);
    }

    return ret;
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

int main(int argc, char* argv[])
{
    assert(argv != nullptr);
    bool ret = false;
    if (argv != nullptr)
    {
        try
        {
            Config user_config;

            // List command line options.
            cxxopts::Options opts(argv[0]);
            opts.add_options()
                ("h,help", "Print help manual.")
                ("p,parse", "Full path to input crash dump file to be parsed by rgd.", cxxopts::value<std::string>(user_config.crash_dump_file))
                ("o,output", "Full path to output text file to be generated by rgd with the analysis contents. If no output file path is provided (neither text or JSON), the tool will print the output to the console.", cxxopts::value<std::string>(user_config.output_file_txt))
                ("j,json", "Full path to output JSON file to be generated by rgd with the analysis contents.", cxxopts::value<std::string>(user_config.output_file_json))
                ("v,version", "Print the rgd command line tool version.")
                ("verbose", "If specified, the tool's console output will include more verbose low-level information such as task completion status. Error reposting may be more detailed as well.", cxxopts::value<bool>(user_config.is_verbose))
                ("all-resources", "If specified, the tool's output will include all the resources regardless of their virtual address from the input crash dump file.", cxxopts::value<bool>(user_config.is_all_resources))
                ("va-timeline", "If specified, Print a timeline of all memory events that impact the offending VA (only applicable if the crash was caused by a page fault).", cxxopts::value<bool>(user_config.is_va_timeline))
                ("marker-src", "If specified, the tool's output will include the marker source information for the execution marker tree text visualization.", cxxopts::value<bool>(user_config.is_marker_src))
                ("expand-markers", "If specified, all the marker nodes will be expanded in the execution marker tree visualization.", cxxopts::value<bool>(user_config.is_expand_markers))
                ("compact-json", "If specified, print compact unindented JSON output. The default is pretty formatted JSON output.", cxxopts::value<bool>(user_config.is_compact_json))
                ("internal-barriers", "If specified, include internal barriers in the execution marker tree.", cxxopts::value<bool>(user_config.is_include_internal_barriers))
                ("all-disassembly", "If specified, the output will include the complete disassembly of all shaders in the SHADER INFO section.", cxxopts::value<bool>(user_config.is_all_disassembly))
                ;

            opts.add_options("internal")
                ("raw-data", "If specified, the tool's output will include the raw data of each and every event (for internal debugging purposes).", cxxopts::value<bool>(user_config.is_raw_event_data))
                ("raw-time", "If specified, the raw timestamp will be printed in the text output.", cxxopts::value<bool>(user_config.is_raw_time))
                ("extended-sysinfo", "If specified, the tool output will include additional system information.", cxxopts::value<bool>(user_config.is_extended_sysinfo))
                ("implicit-res", "If specified, implicit resources will be included in the output summary file (DX12 only).", cxxopts::value<bool>(user_config.is_include_implicit_resources))
                ;

            // Parse command line.
            auto result = opts.parse(argc, argv);
            if (result.count("help"))
            {
                std::cout << opts.help({""}) << std::endl;
                exit(EXIT_SUCCESS);
            }
            else if (result.count("version"))
            {
                std::cout << RGD_TITLE << std::endl;
                exit(EXIT_SUCCESS);
            }

            // Validate input.
            ret = IsInputValid(user_config);
            if (ret && RgdUtils::IsFileExists(user_config.crash_dump_file))
            {
                // Default: text or console.
                ret = PerformCrashAnalysis(user_config);
            }
            else
            {
                RgdUtils::PrintMessage("crash dump input file missing. Use --parse "
                    "<full path to crash dump file> (run -h for more details).", RgdMessageType::kError, user_config.is_verbose);
                ret = false;
            }
        }
        catch (const cxxopts::OptionException& e)
        {
            std::stringstream msg;
            msg << "failed parsing options : " << e.what();
            RgdUtils::PrintMessage(msg.str().c_str(), RgdMessageType::kError, true);
            ret = false;
        }
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
