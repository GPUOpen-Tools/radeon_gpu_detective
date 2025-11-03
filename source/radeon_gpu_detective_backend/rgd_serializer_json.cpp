//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer to JSON format.
//=============================================================================

// Local.
#include "rgd_serializer_json.h"
#include "rgd_utils.h"
#include "rgd_version_info.h"

// C++.
#include <string>
#include <sstream>
#include <cassert>
#include <iostream>
#include <fstream>

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - BEGIN ***

static const char* kJsonElemTimestampElement          = "timestamp";
static const char* kJsonElemSystemInfo                = "system_info";
static const char* kJsonElemDriverInfo                = "driver_info";
static const char* kJsonElemPdbFiles                  = "pdb_files";
static const char* kJsonElemCrashAnalysisFile         = "crash_analysis_file";
static const char* kJsonElemPdbSearchPathsFromRgdFile = "pdb_search_paths_from_rgd_file";
static const char* kJsonElemPdbSearchPathsFromRgdCli  = "pdb_search_paths_from_rgd_cli";

// GPR data JSON keys - optimized string constants.
static const char* kJsonElemGprTimestamp       = "timestamp";
static const char* kJsonElemGprType           = "type";
static const char* kJsonElemGprShaderId       = "shader_id";
static const char* kJsonElemGprSeId           = "se_id";
static const char* kJsonElemGprSaId           = "sa_id";
static const char* kJsonElemGprWgpId          = "wgp_id";
static const char* kJsonElemGprSimdId         = "simd_id";
static const char* kJsonElemGprWaveId         = "wave_id";
static const char* kJsonElemGprWorkItem       = "work_item";
static const char* kJsonElemGprRegistersToRead = "registers_to_read";
static const char* kJsonElemGprRegisterValues = "register_values";
static const char* kJsonElemGprRawData        = "raw_vgpr_and_sgpr_data";
static const char* kJsonElemGprTypeVgpr       = "VGPR";
static const char* kJsonElemGprTypeSgpr       = "SGPR";

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - ENDS ***

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

// Generates a name in the format "<name_prefix>_<index+1>" (e.g. "cpu_1", "gpu_3")
static std::string GenerateCountName(const std::string& name_prefix, uint32_t index)
{
    // Generate the count name of the current CPU.
    std::stringstream count_name_stream;
    count_name_stream << name_prefix << "_" << (index + 1);
    return count_name_stream.str();
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

void RgdSerializerJson::SetInputInfo(const Config& user_config, const RgdCrashDumpContents& contents, const std::vector<std::string>& debug_info_files)
{
    json_[kJsonElemCrashAnalysisFile]["input_crash_dump_file_name"] = user_config.crash_dump_file;
    json_[kJsonElemCrashAnalysisFile]["input_crash_dump_file_creation_time"] = RgdUtils::GetFileCreationTime(user_config.crash_dump_file);
    std::string rgd_version_string;
    RgdUtils::TrimLeadingAndTrailingWhitespace(RGD_TITLE, rgd_version_string);
    json_[kJsonElemCrashAnalysisFile]["rgd_cli_version"] = rgd_version_string;
    json_[kJsonElemCrashAnalysisFile]["json_schema_version"] = RGD_JSON_SCHEMA_VERSION;
    json_[kJsonElemCrashAnalysisFile]["crashing_process_id"] = contents.crashing_app_process_info.process_id;
    json_[kJsonElemCrashAnalysisFile]["crashing_process_path"] =
        (contents.crashing_app_process_info.process_path.empty() ? kStrNotAvailable : contents.crashing_app_process_info.process_path);
    json_[kJsonElemCrashAnalysisFile]["api"] = RgdUtils::GetApiString(contents.api_info.apiType);

    // PDB files.
    if (contents.api_info.apiType != TraceApiType::DIRECTX_12)
    {
        json_[kJsonElemCrashAnalysisFile][kJsonElemPdbFiles] = kStrNotAvailable;
    }
    else if (debug_info_files.empty())
    {
        json_[kJsonElemCrashAnalysisFile][kJsonElemPdbFiles] = "no PDB files found.";
    }
    else
    {
        json_[kJsonElemCrashAnalysisFile][kJsonElemPdbFiles] = nlohmann::json::array();
        for (const std::string& pdb_file : debug_info_files)
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbFiles].push_back(pdb_file);
        }
    }

    // Search paths (only when extended output is enabled).
    if (user_config.is_extended_output)
    {
        // PDB search paths from .rgd file.
        if (contents.api_info.apiType != TraceApiType::DIRECTX_12)
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdFile] = kStrNotAvailable;
        }
        else if (contents.rgd_extended_info.pdb_search_paths.empty())
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdFile] = kStrNone;
        }
        else
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdFile] = nlohmann::json::array();

            for (const std::string& path : contents.rgd_extended_info.pdb_search_paths)
            {
                json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdFile].push_back(path);
            }
        }

        // PDB search paths from CLI.
        if (contents.api_info.apiType != TraceApiType::DIRECTX_12)
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdCli] = kStrNotAvailable;
        }
        else if (user_config.pdb_dir.empty())
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdCli] = kStrNone;
        }
        else
        {
            json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdCli] = nlohmann::json::array();

            for (const std::string& path : user_config.pdb_dir)
            {
                json_[kJsonElemCrashAnalysisFile][kJsonElemPdbSearchPathsFromRgdCli].push_back(path);
            }
        }
    }

    // Hardware Crash Analysis status.
    json_[kJsonElemCrashAnalysisFile]["hardware_crash_analysis"] = contents.rgd_extended_info.is_hca_enabled ? kStrEnabled : kStrDisabled;

    // SGPR/VGPR collection status.
    json_[kJsonElemCrashAnalysisFile]["sgpr_vgpr_collection"] = contents.rgd_extended_info.is_capture_sgpr_vgpr_data ? kStrEnabled : kStrDisabled;
}

void RgdSerializerJson::SetSystemInfoData(const Config& user_config, const system_info_utils::SystemInfo& system_info)
{
    if (user_config.is_extended_sysinfo)
    {
        // Version.
        json_[kJsonElemSystemInfo]["system_info_version"]["major"] = system_info.version.major;
        json_[kJsonElemSystemInfo]["system_info_version"]["minor"] = system_info.version.minor;
        json_[kJsonElemSystemInfo]["system_info_version"]["patch"] = system_info.version.patch;
        json_[kJsonElemSystemInfo]["system_info_version"]["build"] = system_info.version.build;
    }

    // Driver info.
    json_[kJsonElemSystemInfo][kJsonElemDriverInfo]["packaging_version"] = system_info.driver.packaging_version;
    json_[kJsonElemSystemInfo][kJsonElemDriverInfo]["software_version"] = system_info.driver.software_version;
    json_[kJsonElemSystemInfo][kJsonElemDriverInfo]["dev_driver_version"] = (system_info.devdriver.tag.empty() ? kStrNotAvailable : system_info.devdriver.tag);

    // Operating system info.
    json_[kJsonElemSystemInfo]["os"]["name"] = system_info.os.name;
    json_[kJsonElemSystemInfo]["os"]["description"] = system_info.os.desc;
    json_[kJsonElemSystemInfo]["os"]["hostname"] = system_info.os.hostname;
    json_[kJsonElemSystemInfo]["os"]["memory"] = nlohmann::json::array();
    json_[kJsonElemSystemInfo]["os"]["memory"].push_back({
        {"physical_bytes", system_info.os.memory.physical},
        {"swap_bytes", system_info.os.memory.swap}
        });

    // CPU info.
    json_[kJsonElemSystemInfo]["cpu"] = nlohmann::json::array();
    for (uint32_t i = 0; i < system_info.cpus.size(); i++)
    {
        std::string cpu_name;
        RgdUtils::TrimLeadingAndTrailingWhitespace(system_info.cpus[i].name, cpu_name);
        if (user_config.is_extended_sysinfo)
        {
            json_[kJsonElemSystemInfo]["cpu"].push_back({
                {"name", cpu_name },
                {"architecture", system_info.cpus[i].architecture },
                {"cpu_id", system_info.cpus[i].cpu_id },
                {"device_id", system_info.cpus[i].device_id },
                {"max_clock_speed_mhz", system_info.cpus[i].max_clock_speed },
                {"logical_core_count", system_info.cpus[i].num_logical_cores },
                {"physical_core_count", system_info.cpus[i].num_physical_cores },
                {"vendor_id", system_info.cpus[i].vendor_id },
                {"virtualization", system_info.cpus[i].virtualization },
                });
        }
        else
        {
            json_[kJsonElemSystemInfo]["cpu"].push_back({
                {"name", cpu_name },
                {"architecture", system_info.cpus[i].architecture },
                {"cpu_id", system_info.cpus[i].cpu_id },
                {"virtualization", system_info.cpus[i].virtualization },
                });
        }
    }

    // GPU info.
    json_[kJsonElemSystemInfo]["gpu"] = nlohmann::json::array();
    for (uint32_t g = 0; g < system_info.gpus.size(); g++)
    {
        // Memory heaps.
        auto heaps_array = nlohmann::json::array();
        for (uint32_t k = 0; k < system_info.gpus[g].memory.heaps.size(); k++)
        {
            const std::string kHeapCount = GenerateCountName("memory_heap", k);

            if (user_config.is_extended_sysinfo)
            {
                heaps_array.push_back(
                    {
                        {"heap_type", RgdUtils::ToHeapTypeString(system_info.gpus[g].memory.heaps[k].heap_type)},
                        {"heap_size_bytes", system_info.gpus[g].memory.heaps[k].size},
                        {"heap_physical_location_offset_bytes", system_info.gpus[g].memory.heaps[k].phys_addr}
                    });
            }
            else if(system_info.gpus[g].memory.heaps[k].size > 0)
            {
                heaps_array.push_back(
                    {
                        {"heap_type", RgdUtils::ToHeapTypeString(system_info.gpus[g].memory.heaps[k].heap_type)},
                        {"heap_size_bytes", system_info.gpus[g].memory.heaps[k].size}
                    });
            }
        }

        if (user_config.is_extended_sysinfo)
        {
            // Excluded VA ranges.
            auto excluded_va_ranges_array = nlohmann::json::array();
            for (uint32_t x = 0; x < system_info.gpus[g].memory.excluded_va_ranges.size(); x++)
            {
                excluded_va_ranges_array.push_back(
                    {
                        {"base_address", system_info.gpus[g].memory.excluded_va_ranges[x].base},
                        {"size_bytes", system_info.gpus[g].memory.excluded_va_ranges[x].size}
                    });
            }

            json_[kJsonElemSystemInfo]["gpu"].push_back({
                {"name", system_info.gpus[g].name },
                {"engine_clock_max_hz", system_info.gpus[g].asic.engine_clock_hz.max },
                {"engine_clock_min_hz", system_info.gpus[g].asic.engine_clock_hz.min },
                {"gpu_display_adapter_order", system_info.gpus[g].asic.gpu_index },
                {"device_id", system_info.gpus[g].asic.id_info.device },
                {"device_revision_id", system_info.gpus[g].asic.id_info.e_rev },
                {"device_family_id", system_info.gpus[g].asic.id_info.family },
                {"device_graphics_engine_id", system_info.gpus[g].asic.id_info.gfx_engine },
                {"device_pci_revision_id", system_info.gpus[g].asic.id_info.revision },
                {"big_sw_version", { {"major",system_info.gpus[g].big_sw.major }, {"minor", system_info.gpus[g].big_sw.minor}, {"misc", system_info.gpus[g].big_sw.misc} }},
                {"memory", {
                    {"type", system_info.gpus[g].memory.type },
                    {"clock_max_hz", system_info.gpus[g].memory.mem_clock_hz.max },
                    {"clock_min_hz", system_info.gpus[g].memory.mem_clock_hz.min },
                    {"ops_per_clock", system_info.gpus[g].memory.mem_ops_per_clock },
                    {"bandwidth_bytes_per_sec", system_info.gpus[g].memory.bandwidth },
                    {"bus_width_bits", system_info.gpus[g].memory.bus_bit_width },
                    {"heaps", heaps_array },
                    {"excluded_va_ranges", excluded_va_ranges_array }
                }},
                });
        }
        else
        {
            json_[kJsonElemSystemInfo]["gpu"].push_back({
                {"name", system_info.gpus[g].name },
                {"device_id", system_info.gpus[g].asic.id_info.device },
                {"e_rev", system_info.gpus[g].asic.id_info.e_rev },
                {"device_family_id", system_info.gpus[g].asic.id_info.family },
                {"device_graphics_engine_id", system_info.gpus[g].asic.id_info.gfx_engine },
                {"revision", system_info.gpus[g].asic.id_info.revision },
                {"big_sw_version", { {"major",system_info.gpus[g].big_sw.major }, {"minor", system_info.gpus[g].big_sw.minor}, {"misc", system_info.gpus[g].big_sw.misc} }},
                {"memory", {
                    {"type", system_info.gpus[g].memory.type },
                    {"heaps", heaps_array }
                }},
                });
        }
    }
}

void RgdSerializerJson::SetUmdCrashData(const CrashData& umd_crash_data)
{
    const char* kJsonElemExecMarkers = "execution_markers";
    static const char* kJsonElemMarkerType = "marker_type";
    static const char* kJsonElemMarkerValue = "marker_value";

    // Serialize the execution markers.
    json_[kJsonElemExecMarkers] = nlohmann::json::array();
    for (uint32_t i = 0; i < umd_crash_data.events.size(); i++)
    {
        const RgdEvent& rgd_event = *umd_crash_data.events[i].rgd_event;

        //UmdEventId event_id = static_cast<UmdEventId>(rgd_event.header.eventId);
        switch (rgd_event.header.eventId)
        {
        case uint8_t(UmdEventId::RgdEventExecutionMarkerBegin):
        {
            const CrashAnalysisExecutionMarkerBegin& exec_marker_begin_event = static_cast<const CrashAnalysisExecutionMarkerBegin&>(rgd_event);
            json_[kJsonElemExecMarkers].push_back({
                {kJsonElemTimestampElement, umd_crash_data.events[i].event_time},
                {kJsonElemMarkerType, "begin"},
                {kJsonElemCmdBufferIdElement, exec_marker_begin_event.cmdBufferId},
                {kJsonElemMarkerValue, exec_marker_begin_event.markerValue & kMarkerValueMask},
                });
        }
        break;
        case uint8_t(UmdEventId::RgdEventExecutionMarkerEnd):
        {
            const CrashAnalysisExecutionMarkerEnd& exec_marker_end_event = static_cast<const CrashAnalysisExecutionMarkerEnd&>(rgd_event);
            json_[kJsonElemExecMarkers].push_back({
                {kJsonElemTimestampElement, umd_crash_data.events[i].event_time},
                {kJsonElemMarkerType, "end"},
                {kJsonElemCmdBufferIdElement, exec_marker_end_event.cmdBufferId},
                {kJsonElemMarkerValue, exec_marker_end_event.markerValue & kMarkerValueMask},
                });
        }
        break;
        case uint8_t(UmdEventId::RgdEventCrashDebugNopData):
        {
            const CrashDebugNopData& debug_nop_event = static_cast<const CrashDebugNopData&>(rgd_event);
            json_["debug_nop_events"].push_back({
                {kJsonElemTimestampElement, umd_crash_data.events[i].event_time},
                {kJsonElemCmdBufferIdElement, debug_nop_event.cmdBufferId},
                {"begin_timestamp", debug_nop_event.beginTimestampValue},
                {"end_timestamp", debug_nop_event.endTimestampValue}
                });
        }
        break;
        // These are ignored.
        case uint8_t(DDCommonEventId::RgdEventTimestamp):
        break;
        default:
            // Notify in case there is an unknown event type.
            assert(false);
        }
    }
}

void RgdSerializerJson::SetKmdCrashData(const CrashData& kmd_crash_data)
{
    for (uint32_t i = 0; i < kmd_crash_data.events.size(); i++)
    {
        const RgdEvent& rgd_event = *kmd_crash_data.events[i].rgd_event;

        switch (rgd_event.header.eventId)
        {
        case uint8_t(KmdEventId::RgdEventVmPageFault):
        {
            const VmPageFaultEvent& page_fault_event = static_cast<const VmPageFaultEvent&>(rgd_event);
            const char* process_name = (const char*)page_fault_event.processName;
            if (page_fault_event.processNameLength == 0)
            {
                process_name = kStrNotAvailable;
            }
            json_["page_fault_events"].push_back({
                {kJsonElemTimestampElement, kmd_crash_data.events[i].event_time},
                {"virtual_address", page_fault_event.faultVmAddress},
                {"process_id", page_fault_event.processId},
                {"process_name", process_name},
                {"vm_id", page_fault_event.vmId},
                {"process_name_length", page_fault_event.processNameLength}
                });
        }
        break;
        // These are ignored.
        case uint8_t(DDCommonEventId::RgdEventTimestamp):
        break;
        default:
            // Notify in case there is an unknown event type.
            assert(false);
        }
    }
}

void RgdSerializerJson::SetVaResourceData(RgdResourceInfoSerializer& resource_serializer,
    const Config& user_config,
    const uint64_t virtual_address)
{
    RgdUtils::PrintMessage("generating JSON representation of the page fault information...", RgdMessageType::kInfo, user_config.is_verbose);
    nlohmann::json resource_info_json;
    bool is_ok = resource_serializer.GetVirtualAddressHistoryInfo(user_config, virtual_address, resource_info_json);

    assert(is_ok);

    if (is_ok)
    {
        json_[kJsonElemPageFaultSummary].push_back(resource_info_json[kJsonElemPageFaultSummary]);

        if (!json_[kJsonElemPageFaultSummary].empty())
        {
            RgdUtils::PrintMessage("JSON representation of the page fault information generated successfully.",
                RgdMessageType::kInfo, user_config.is_verbose);
        }
    }
    else
    {
        RgdUtils::PrintMessage("failed to generate JSON representation of the page fault information.", RgdMessageType::kError, user_config.is_verbose);
    }
}

void RgdSerializerJson::SetExecutionMarkerTree(const Config& user_config, const CrashData& kmd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    ExecMarkerDataSerializer& exec_marker_serializer)
{
    RgdUtils::PrintMessage("generating JSON representation of the execution marker tree...", RgdMessageType::kInfo, user_config.is_verbose);
    nlohmann::json all_cmd_buffers_marker_tree_json;

    all_cmd_buffers_marker_tree_json[kJsonElemExecutionMarkerTree] = nlohmann::json::array();

    bool is_ok = exec_marker_serializer.GenerateExecutionMarkerTreeToJson(user_config,
        kmd_crash_data,
        cmd_buffer_events,
        all_cmd_buffers_marker_tree_json);

    assert(is_ok);

    if (is_ok)
    {
        json_[kJsonElemExecutionMarkerTree] = all_cmd_buffers_marker_tree_json[kJsonElemExecutionMarkerTree];

        if (!all_cmd_buffers_marker_tree_json[kJsonElemExecutionMarkerTree].empty())
        {
            RgdUtils::PrintMessage("JSON representation of the execution marker tree generated successfully.",
                RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            // In verbose mode, we would have already printed this message while trying to generate the marker summary.
            RgdUtils::PrintMessage(kStrInfoNoCmdBuffersInFlight, RgdMessageType::kInfo, user_config.is_verbose);
        }
    }
    else
    {
        RgdUtils::PrintMessage("failed to generate JSON representation of the execution tree.", RgdMessageType::kError, user_config.is_verbose);
    }
}

void RgdSerializerJson::SetExecutionMarkerSummaryList(const Config& user_config, const CrashData& kmd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    ExecMarkerDataSerializer& exec_marker_serializer)
{
    RgdUtils::PrintMessage("generating JSON representation of the list of markers in progress...", RgdMessageType::kInfo, user_config.is_verbose);
    nlohmann::json all_cmd_buffers_exec_marker_summary;
    bool is_ok = exec_marker_serializer.GenerateExecutionMarkerSummaryListJson(user_config,
        kmd_crash_data,
        cmd_buffer_events,
        all_cmd_buffers_exec_marker_summary);

    assert(is_ok);

    if (is_ok)
    {
        json_[kJsonElemMarkersInProgress] = all_cmd_buffers_exec_marker_summary[kJsonElemMarkersInProgress];

        if (!json_[kJsonElemMarkersInProgress].empty())
        {
            RgdUtils::PrintMessage("JSON representation of the list of markers in progress generated successfully.",
                RgdMessageType::kInfo, user_config.is_verbose);
        }
        else
        {
            RgdUtils::PrintMessage(kStrInfoNoCmdBuffersInFlight, RgdMessageType::kInfo, user_config.is_verbose);
        }
    }
    else
    {
        RgdUtils::PrintMessage("failed to generate JSON representation of the list of markers in progress.", RgdMessageType::kError, user_config.is_verbose);
    }
}

void RgdSerializerJson::SetDriverExperimentsInfoData(const nlohmann::json& driver_experiments_json)
{
    const char*       kJsonElemExperimentsRgdOutputJson               = "experiments";
    const char*       kJsonElemSettingNameRgdOutputJson               = "setting_name";
    const char*       kJsonElemUserOverrideRgdOutputJson              = "user_override";
    std::stringstream txt;

    // Add "experiments" array under the system_info -> driver_info.
    json_[kJsonElemSystemInfo][kJsonElemDriverInfo][kJsonElemExperimentsRgdOutputJson] = nlohmann::json::array();

    try
    {
        if (driver_experiments_json.find(kJsonElemComponentsDriverOverridesChunk) != driver_experiments_json.end() &&
            driver_experiments_json[kJsonElemComponentsDriverOverridesChunk].is_array() &&
            driver_experiments_json.find(kJsonElemIsDriverExperimentsDriverOverridesChunk) != driver_experiments_json.end() &&
            driver_experiments_json[kJsonElemIsDriverExperimentsDriverOverridesChunk].is_boolean())
        {
            const bool is_driver_experiments = driver_experiments_json[kJsonElemIsDriverExperimentsDriverOverridesChunk].get<bool>();

            if (is_driver_experiments)
            {
                const nlohmann::json& components_json = driver_experiments_json[kJsonElemComponentsDriverOverridesChunk];

                for (const auto& component : components_json)
                {
                    // Process "Experiments" component.
                    if (component[kJsonElemComponentDriverOverridesChunk] == kJsonElemExperimentsDriverOverridesChunk)
                    {
                        const nlohmann::json& structures = component[kJsonElemStructuresDriverOverridesChunk];
                        size_t                exp_seq_no = 1;
                        for (auto it = structures.begin(); it != structures.end(); ++it)
                        {
                            const nlohmann::json& experiments = it.value();
                            for (const auto& experiment : experiments)
                            {
                                // Check if the experiment was supported by the driver at the time of the crash.
                                bool is_supported_experiment = (experiment[kJsonElemWasSupportedDriverOverridesChunk].is_boolean() &&
                                                                experiment[kJsonElemWasSupportedDriverOverridesChunk]);
                                if (is_supported_experiment)
                                {
                                    if (experiment[kJsonElemUserOverrideDriverOverridesChunk].is_boolean() &&
                                        experiment[kJsonElemCurrentDriverOverridesChunk].is_boolean())
                                    {
                                        // The user override value.
                                        bool is_user_override = experiment[kJsonElemUserOverrideDriverOverridesChunk].get<bool>();

                                        // The value in the driver at the time of the crash.
                                        bool is_current = experiment[kJsonElemCurrentDriverOverridesChunk].get<bool>();

                                        if (is_user_override && is_current)
                                        {
                                            // Experiment is active only when both user override and current values are true.
                                            json_[kJsonElemSystemInfo][kJsonElemDriverInfo][kJsonElemExperimentsRgdOutputJson].push_back(
                                                {{kJsonElemSettingNameRgdOutputJson, experiment[kJsonElemSettingNameDriverOverridesChunk]},
                                                 {kJsonElemUserOverrideRgdOutputJson, experiment[kJsonElemCurrentDriverOverridesChunk]}});
                                        }
                                    }
                                    else
                                    {
                                        json_[kJsonElemSystemInfo][kJsonElemDriverInfo][kJsonElemExperimentsRgdOutputJson].push_back(
                                            {{kJsonElemSettingNameRgdOutputJson, experiment[kJsonElemSettingNameDriverOverridesChunk]},
                                             {kJsonElemUserOverrideRgdOutputJson, experiment[kJsonElemCurrentDriverOverridesChunk]}});
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            assert(false);
            RgdUtils::PrintMessage(kErrorMsgInvalidDriverOverridesJson, RgdMessageType::kError, true);
        }
    }
    catch (nlohmann::json::exception& e)
    {
        assert(false);
        std::stringstream error_msg;
        error_msg << kErrorMsgFailedToParseDriverExperimentsInfo << " (" << e.what() << ")";
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, true);
    } 
}

void RgdSerializerJson::SetShaderInfo(const Config& user_config, RgdEnhancedCrashInfoSerializer& enhanced_crash_info_serializer)
{
    RgdUtils::PrintMessage("generating JSON representation of the in-flight shader information...", RgdMessageType::kInfo, user_config.is_verbose);
    nlohmann::json shader_info_json;
    bool is_ok = enhanced_crash_info_serializer.GetInFlightShaderInfoJson(user_config, shader_info_json);

    if (is_ok)
    {
        json_[kJsonElemShaderInfo] = shader_info_json[kJsonElemShaderInfo];
        assert(!json_[kJsonElemShaderInfo].empty());
        RgdUtils::PrintMessage(
            "JSON representation of the in-flight shader information generated successfully.", RgdMessageType::kInfo, user_config.is_verbose);
    }
    else
    {
        RgdUtils::PrintMessage(kStrNoInFlightShaderInfo, RgdMessageType::kWarning, user_config.is_verbose);
    }
}

void RgdSerializerJson::SetGprData(const CrashData& kmd_crash_data)
{
    bool should_process = true;
    
    // Check if no events.
    if (kmd_crash_data.events.empty())
    {
        should_process = false;
    }

    const size_t total_events = should_process ? kmd_crash_data.events.size() : 0;
    const uint8_t gpr_event_id = uint8_t(KmdEventId::SgprVgprRegisters);
    
    // Two-pass algorithm to pre-count GPR events for exact allocation.
    size_t gpr_event_count = 0;
    if (should_process)
    {
        for (size_t i = 0; i < total_events; ++i)
        {
            const RgdEventOccurrence& current_event = kmd_crash_data.events[i];
            if (current_event.rgd_event != nullptr && 
                current_event.rgd_event->header.eventId == gpr_event_id)
            {
                gpr_event_count++;
            }
        }
    }
    
    // Check if no GPR events found.
    if (gpr_event_count == 0)
    {
        should_process = false;
    }
    
    nlohmann::json gpr_data_array;
    size_t gpr_events_processed = 0;
    
    if (should_process)
    {
        gpr_data_array = nlohmann::json::array();
        
        for (size_t i = 0; i < total_events; ++i)
        {
            const RgdEventOccurrence& current_event = kmd_crash_data.events[i];
            
            if (current_event.rgd_event != nullptr && 
                current_event.rgd_event->header.eventId == gpr_event_id)
            {
                gpr_events_processed++;
                const GprRegistersData& gpr_event = static_cast<const GprRegistersData&>(*current_event.rgd_event);
                
                const uint32_t shader_id = gpr_event.shaderId;
                const uint32_t wave_id = gpr_event.waveId;
                const uint32_t simd_id = gpr_event.simdId;
                const uint32_t wgp_id = gpr_event.wgpId;
                const uint32_t sa_id = gpr_event.saId;
                const uint32_t se_id = gpr_event.seId;
                
                nlohmann::json gpr_entry;
                gpr_entry[kJsonElemGprTimestamp] = current_event.event_time;
                gpr_entry[kJsonElemGprType] = gpr_event.isVgpr ? kJsonElemGprTypeVgpr : kJsonElemGprTypeSgpr;
                gpr_entry[kJsonElemGprShaderId] = shader_id;
                gpr_entry[kJsonElemGprSeId] = se_id;
                gpr_entry[kJsonElemGprSaId] = sa_id;
                gpr_entry[kJsonElemGprWgpId] = wgp_id;
                gpr_entry[kJsonElemGprSimdId] = simd_id;
                gpr_entry[kJsonElemGprWaveId] = wave_id;
                gpr_entry[kJsonElemGprWorkItem] = gpr_event.workItem;
                gpr_entry[kJsonElemGprRegistersToRead] = gpr_event.regToRead;

                // For VGPR and SGPR data, we first collect all register values in a vector rather than adding them individually to the JSON object.
                // This significantly reduces overhead by performing a single bulk operation instead of multiple JSON insertions.
                // This approach is especially important when handling thousands of registers across multiple waves.
                const uint32_t reg_count = gpr_event.regToRead;
                if (reg_count > 0)
                {
                    // Create std::vector first, then assign to JSON in one operation.
                    std::vector<uint32_t> register_values_vec(gpr_event.reg, gpr_event.reg + reg_count);
                    gpr_entry[kJsonElemGprRegisterValues] = std::move(register_values_vec);
                }
                else
                {
                    gpr_entry[kJsonElemGprRegisterValues] = nlohmann::json::array();
                }
                
                // Add entry to main array.
                gpr_data_array.emplace_back(std::move(gpr_entry));
            }
        }
    }

    // Assign the complete array to the main JSON object in one operation.
    if (!gpr_data_array.empty())
    {
        json_[kJsonElemGprRawData] = std::move(gpr_data_array);
        has_gpr_data_ = true;
    }
}

bool RgdSerializerJson::SaveToFile(const Config& user_config) const
{
    std::string contents;

    if (user_config.is_compact_json || user_config.is_raw_gpr_data)
    {
        // Use compact format when requested or raw VGPR and SGPR data is present.
        contents = json_.dump();
    }
    else
    {
        const int kIndent = 4;
        contents = json_.dump(kIndent);
    }
    return RgdUtils::WriteTextFile(user_config.output_file_json, contents);
}

void RgdSerializerJson::Clear()
{
    // Clear large GPR data first to minimize destructor time.
    if (has_gpr_data_ && json_.contains(kJsonElemGprRawData))
    {
        // Move GPR data out and let it destruct separately to avoid expensive copying.
        // temp_gpr will be automatically destructed here with move semantics.
        nlohmann::json temp_gpr = std::move(json_[kJsonElemGprRawData]);
        json_.erase(kJsonElemGprRawData);
    }
    
    // Clear the remaining JSON object.
    json_.clear();
    has_gpr_data_ = false;
}
