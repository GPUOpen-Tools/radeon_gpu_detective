//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer different data elements.
//=============================================================================
// Local.
#include "rgd_parsing_utils.h"
#include "rgd_serializer.h"
#include "rgd_utils.h"
#include "rgd_version_info.h"

// C++.
#include <cassert>
#include <iomanip>
#include <string>
#include <sstream>

// Clock speed units.
enum class ClockSpeedUnit {kHz, kKHz, kMHz, kGHz};

// Convert and return the value in GHz.
static double GetValueInHz(double val, ClockSpeedUnit val_unit = ClockSpeedUnit::kHz)
{
    double result = 0;
    if (val_unit == ClockSpeedUnit::kHz)
    {
        result = val / 1000000000;
    }
    else if (val_unit == ClockSpeedUnit::kMHz)
    {
        result = val / 1000;
    }

    return result;
}

// Returns the list of active Driver Experiments.
static std::string GetDriverExperimentsString(const nlohmann::json& driver_experiments_json)
{
    std::stringstream txt;
    size_t            active_experiments_count                  = 0;

    try
    {
        if (driver_experiments_json.find(kJsonElemComponentsDriverOverridesChunk) != driver_experiments_json.end()
            && driver_experiments_json[kJsonElemComponentsDriverOverridesChunk].is_array()
            && driver_experiments_json.find(kJsonElemIsDriverExperimentsDriverOverridesChunk) != driver_experiments_json.end()
            && driver_experiments_json[kJsonElemIsDriverExperimentsDriverOverridesChunk].is_boolean())
        {
            const bool is_driver_experiments = driver_experiments_json[kJsonElemIsDriverExperimentsDriverOverridesChunk].get<bool>();
            
            if (is_driver_experiments)
            {
                const nlohmann::json& components_json             = driver_experiments_json[kJsonElemComponentsDriverOverridesChunk];
                
                for (const auto& component : components_json)
                {
                    // Process "Experiments" component.
                    if (component[kJsonElemComponentDriverOverridesChunk] == kJsonElemExperimentsDriverOverridesChunk)
                    {
                        const nlohmann::json& structures = component[kJsonElemStructuresDriverOverridesChunk];
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
                                            txt << "\t" << ++active_experiments_count << ". "
                                                << experiment[kJsonElemSettingNameDriverOverridesChunk].get<std::string>() << std::endl;
                                        }
                                    }
                                    else
                                    {
                                        txt << "\t" << ++active_experiments_count << ". "
                                            << experiment[kJsonElemSettingNameDriverOverridesChunk].get<std::string>() << ": "
                                            << experiment[kJsonElemCurrentDriverOverridesChunk].get<std::string>() << std::endl;
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
    catch (nlohmann::json::exception e)
    {
        assert(false);
        std::stringstream error_msg;
        error_msg << kErrorMsgFailedToParseDriverExperimentsInfo << " (" << e.what() << ")";
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, true);
    }

    const char* kDriverExperimentsSectionStr     = "Experiments: ";
    const char* kDriverExperimentsActiveMsgPart1 = "total of ";
    const char* kDriverExperimentsActiveMsgPart2 = " Driver Experiments were active while capturing the AMD GPU crash dump:";
    const char* kDriverExperimentsNotActiveMsg   = "no driver experiments were enabled.";

    std::stringstream driver_experiments_txt;
    if (active_experiments_count > 0)
    {
        driver_experiments_txt << kDriverExperimentsSectionStr << kDriverExperimentsActiveMsgPart1 << active_experiments_count << kDriverExperimentsActiveMsgPart2
                               << std::endl;
        driver_experiments_txt << txt.str();
    }
    else
    {
        driver_experiments_txt << kDriverExperimentsSectionStr << kDriverExperimentsNotActiveMsg << std::endl;
    }

    return driver_experiments_txt.str();
}

static uint64_t CombineRegisterValue(uint32_t high_val, uint32_t low_val)
{
    uint64_t combined_value = ((uint64_t)high_val << 32) | low_val;
    return combined_value;
}

bool RgdSerializer::ToString(const Config& user_config, const system_info_utils::SystemInfo& system_info, const nlohmann::json& driver_experiments_json, std::string& system_info_txt)
{
    bool ret = true;
    std::stringstream txt;
    txt << "===========" << std::endl;
    txt << "SYSTEM INFO" << std::endl;
    txt << "===========" << std::endl;
    txt << std::endl;

    if (user_config.is_extended_sysinfo)
    {
        txt << "System Info version: " << system_info.version.major << "." << system_info.version.minor << "." << system_info.version.patch << "." << system_info.version.build << std::endl;
        txt << std::endl;
    }

    // Driver info.
    txt << "Driver info" << std::endl;
    txt << "===========" << std::endl;
    txt << "Driver packaging version: " << system_info.driver.packaging_version << std::endl;
    txt << "Driver software version: " << system_info.driver.software_version << std::endl;
    txt << "Dev driver version: " << (system_info.devdriver.tag.empty() ? kStrNotAvailable : system_info.devdriver.tag) << std::endl;
    txt << GetDriverExperimentsString(driver_experiments_json);
    txt << std::endl;

    // Operating system info.
    txt << "Operating system info" << std::endl;
    txt << "=====================" << std::endl;
    txt << "Name: " << system_info.os.name << std::endl;
    txt << "Description: " << system_info.os.desc << std::endl;
    txt << "Hostname: " << system_info.os.hostname << std::endl;
    txt << "Memory size (physical bytes): " << RgdParsingUtils::GetFormattedSizeString(system_info.os.memory.physical) << std::endl;
    txt << "Memory size (swap bytes): " << RgdParsingUtils::GetFormattedSizeString(system_info.os.memory.swap) << std::endl;
    txt << std::endl;

    // CPU info.
    txt << "CPU info" << std::endl;
    txt << "========" << std::endl;
    txt << "CPU count: " << system_info.cpus.size() << std::endl;
    for (uint32_t i = 0; i < system_info.cpus.size(); i++)
    {
        std::string cpu_name;
        RgdUtils::TrimLeadingAndTrailingWhitespace(system_info.cpus[i].name, cpu_name);
        txt << "CPU #" << (i + 1) << ":" << std::endl;
        txt << "\tName: " << cpu_name << std::endl;
        txt << "\tArchitecture: " << system_info.cpus[i].architecture << std::endl;
        txt << "\tCPU ID: " << system_info.cpus[i].cpu_id << std::endl;

        if (user_config.is_extended_sysinfo)
        {
            txt << "\tDevice ID: " << system_info.cpus[i].device_id << std::endl;
            txt << "\tMax clock speed: " << std::setprecision(4) << GetValueInHz((double)system_info.cpus[i].max_clock_speed, ClockSpeedUnit::kMHz) << " GHz" << std::endl;
            txt << "\tLogical core count: " << system_info.cpus[i].num_logical_cores << std::endl;
            txt << "\tPhysical core count: " << system_info.cpus[i].num_physical_cores << std::endl;
            txt << "\tVendor ID: " << system_info.cpus[i].vendor_id << std::endl;
        }

        txt << "\tVirtualization: " << system_info.cpus[i].virtualization << std::endl;
        txt << std::endl;
    }

    // GPU info.
    txt << "GPU info" << std::endl;
    txt << "========" << std::endl;
    txt << "GPU count: " << system_info.gpus.size() << std::endl;
    for (uint32_t i = 0; i < system_info.gpus.size(); i++)
    {
        txt << "GPU #" << (i + 1) << ":" << std::endl;
        txt << "\tName: " << system_info.gpus[i].name << std::endl;
        if (user_config.is_extended_sysinfo)
        {
            txt << "\tEngine clock max: " << std::setprecision(4) << GetValueInHz((double)system_info.gpus[i].asic.engine_clock_hz.max) << " GHz" << std::endl;
            txt << "\tEngine clock min: " << std::setprecision(4) << GetValueInHz((double)system_info.gpus[i].asic.engine_clock_hz.min) << " GHz" << std::endl;
            txt << "\tGPU index (order as seen by system): " << system_info.gpus[i].asic.gpu_index << std::endl;
        }
        txt << "\tDevice ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.device << std::dec << std::endl;
        txt << "\teRev: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.e_rev << std::dec << std::endl;
        txt << "\tDevice family ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.family << std::dec << std::endl;
        txt << "\tDevice graphics engine ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.gfx_engine << std::dec << std::endl;
        txt << "\tRevision: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.revision << std::dec << std::endl;
        txt << "\tBig SW version: " << system_info.gpus[i].big_sw.major << "." << system_info.gpus[i].big_sw.minor << "." << system_info.gpus[i].big_sw.misc << std::endl;
        txt << "\tMemory type: " << system_info.gpus[i].memory.type << std::endl;
        if (user_config.is_extended_sysinfo)
        {
            txt << "\tMemory clock max: " << std::setprecision(4) << GetValueInHz((double)system_info.gpus[i].memory.mem_clock_hz.max) << " GHz" << std::endl;
            txt << "\tMemory clock min: " << std::setprecision(4) << GetValueInHz((double)system_info.gpus[i].memory.mem_clock_hz.min) << " GHz" << std::endl;
            txt << "\tMemory ops per clock: " << system_info.gpus[i].memory.mem_ops_per_clock << std::endl;
            txt << "\tMemory bandwidth: " << RgdParsingUtils::GetFormattedSizeString(system_info.gpus[i].memory.bandwidth, "B/s") << std::endl;
            txt << "\tMemory bus width (bits): " << system_info.gpus[i].memory.bus_bit_width << std::endl;
        }

        // Non zero heap indices.
        std::vector<uint32_t> memory_heap_indices;

        // For the default output, print non zero heaps only.
        for (uint32_t k = 0; k < system_info.gpus[i].memory.heaps.size(); k++)
        {
            if (system_info.gpus[i].memory.heaps[k].size != 0 || user_config.is_extended_sysinfo)
            {
                memory_heap_indices.push_back(k);
            }
        }

        txt << "\tMemory heap count: " << memory_heap_indices.size() << std::endl;
        for (const uint32_t& k : memory_heap_indices)
        {
            txt << "\t\tMemory heap #" << (&k - &memory_heap_indices[0] + 1) << ":" << std::endl;
            txt << "\t\t\tHeap type: " << RgdUtils::ToHeapTypeString(system_info.gpus[i].memory.heaps[k].heap_type) << std::endl;
            txt << "\t\t\tHeap size: " << RgdParsingUtils::GetFormattedSizeString(system_info.gpus[i].memory.heaps[k].size) << std::endl;
            if (user_config.is_extended_sysinfo)
            {
                txt << "\t\t\tHeap physical location offset: " << RgdParsingUtils::GetFormattedSizeString(system_info.gpus[i].memory.heaps[k].phys_addr) << std::endl;
            }
        }

        if (user_config.is_extended_sysinfo)
        {
            txt << "\tMemory excluded virtual address range count: " << system_info.gpus[i].memory.excluded_va_ranges.size() << std::endl;
            for (uint32_t k = 0; k < system_info.gpus[i].memory.excluded_va_ranges.size(); k++)
            {
                txt << "\t\tExcluded VA range #" << (k + 1) << ":" << std::endl;
                txt << "\t\t\tBase address: 0x" << std::hex << system_info.gpus[i].memory.excluded_va_ranges[k].base << std::dec << std::endl;
                txt << "\t\t\tSize: " << RgdParsingUtils::GetFormattedSizeString(system_info.gpus[i].memory.excluded_va_ranges[k].size) << std::endl;
            }
        }
    }

    system_info_txt = txt.str();
    return ret;
}

void RgdSerializer::InputInfoToString(const Config&                        user_config,
                                      const RgdCrashDumpContents& contents,
                                      const std::vector<std::string>&            debug_info_files,
                                      std::string& input_info_str)
{
    std::stringstream txt;

    txt << "===================" << std::endl;
    txt << "CRASH ANALYSIS FILE" << std::endl;
    txt << "===================" << std::endl;

    txt << "Crash analysis file format version: 1.0" << std::endl;
    std::string rgd_version_string;
    RgdUtils::TrimLeadingAndTrailingWhitespace(RGD_TITLE, rgd_version_string);
    txt << "RGD CLI version used: " << rgd_version_string << std::endl;
    txt << "Input crash dump file creation time: " << RgdUtils::GetFileCreationTime(user_config.crash_dump_file) << std::endl;
    txt << "Input crash dump file name: " << user_config.crash_dump_file << std::endl;
    txt << "Crashing executable full path: "
        << (contents.crashing_app_process_info.process_path.empty() ? kStrNotAvailable : contents.crashing_app_process_info.process_path)
        << " (PID: " << contents.crashing_app_process_info.process_id << ")" << std::endl;
    txt << "API: " << RgdUtils::GetApiString(contents.api_info.apiType) << std::endl;
    txt << "PDB files used: ";
    
    if (contents.api_info.apiType != TraceApiType::DIRECTX_12)
    {
        txt << kStrNotAvailable << std::endl;
    }
    else if(debug_info_files.empty())
    {
        txt << "no PDB files found." << std::endl;
    }
    else
    {
        
        for (const auto& debug_info_file : debug_info_files)
        {
            txt << debug_info_file;
            if(debug_info_file != debug_info_files.back())
            {
                txt << ", ";
            }
        }
        txt << std::endl;
    }
    if (user_config.is_extended_output)
    { 
        txt << "PDB search paths (.rgd file):";
        if (contents.rgd_extended_info.pdb_search_paths.empty())
        {
            txt << " " << kStrNone;
        }
        else
        {
            for (const std::string& path : contents.rgd_extended_info.pdb_search_paths)
            {
                txt << std::endl << "\t" << path;
            }
        }
        txt << std::endl;
        txt << "PDB search paths (CLI):";
        if (user_config.pdb_dir.empty())
        {
            txt << " " << kStrNone;
        }
        else
        {
            for (const std::string& path : user_config.pdb_dir)
            {
                txt << std::endl << "\t" << path;
            }
        }
        txt << std::endl;
    }
    txt << "Hardware Crash Analysis: " << (contents.rgd_extended_info.is_hca_enabled ? kStrEnabled : kStrDisabled) << std::endl;
    txt << std::endl;

    input_info_str = txt.str();
}

std::string RgdSerializer::RgdEventHeaderToStringUmd(const RgdEvent& rgd_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << offset_tabs << "Event ID: " << RgdParsingUtils::UmdRgdEventIdToString(rgd_event.header.eventId) << std::endl;
    ret << offset_tabs << "Delta: " << (uint32_t)rgd_event.header.delta << std::endl;
    ret << offset_tabs << "Event data size: " << rgd_event.header.eventSize;
    return ret.str();
}

std::string RgdSerializer::RgdEventHeaderToStringKmd(const RgdEvent& rgd_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << offset_tabs << "Event ID: " << RgdParsingUtils::KmdRgdEventIdToString(rgd_event.header.eventId) << std::endl;
    ret << offset_tabs << "Delta: " << (uint32_t)rgd_event.header.delta << std::endl;
    ret << offset_tabs << "Event data size: " << rgd_event.header.eventSize;
    return ret.str();
}

std::string RgdSerializer::EventTimestampToString(const TimestampEvent& timestamp_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringUmd(timestamp_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Timestamp: " << timestamp_event.timestamp;
    return ret.str();
}

std::string RgdSerializer::EventExecMarkerBeginToString(const CrashAnalysisExecutionMarkerBegin& exec_marker_begin_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringUmd(exec_marker_begin_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Marker source: " << RgdParsingUtils::ExtractMarkerSource(exec_marker_begin_event.markerValue) << std::endl;
    ret << offset_tabs << "Command buffer ID: 0x" << std::hex << exec_marker_begin_event.cmdBufferId << std::dec << std::endl;
    ret << offset_tabs << "Marker value: 0x" << std::hex << exec_marker_begin_event.markerValue << std::dec << std::endl;
    std::string marker_name = (exec_marker_begin_event.markerStringSize > 0)
        ? std::string(reinterpret_cast<const char*>(exec_marker_begin_event.markerName), exec_marker_begin_event.markerStringSize)
        : std::string(kStrNotAvailable);
    ret << offset_tabs << "Marker string name: " << marker_name << std::endl;
    ret << offset_tabs << "Marker string length: " << exec_marker_begin_event.markerStringSize;
    return ret.str();
}

std::string RgdSerializer::EventExecMarkerInfoToString(const CrashAnalysisExecutionMarkerInfo& exec_marker_info_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringUmd(exec_marker_info_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Command buffer ID: 0x" << std::hex << exec_marker_info_event.cmdBufferId << std::dec << std::endl;
    ret << offset_tabs << "Marker value: 0x" << std::hex << exec_marker_info_event.marker << std::dec << std::endl;
    uint8_t* marker_info = const_cast<uint8_t*>(exec_marker_info_event.markerInfo);
    ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);

    switch (exec_marker_info_header->infoType)
    {
    case ExecutionMarkerInfoType::CmdBufStart:
    {
        CmdBufferInfo* cmd_buffer_info = reinterpret_cast<CmdBufferInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Command buffer start" << std::endl;
        ret << offset_tabs << "Queue: " << uint32_t(cmd_buffer_info->queue) << std::endl;
        ret << offset_tabs << "Queue type string: " << RgdUtils::GetCmdBufferQueueTypeString((CmdBufferQueueType)cmd_buffer_info->queue) << std::endl;
        ret << offset_tabs << "Device ID: " << cmd_buffer_info->deviceId << std::endl;
        ret << offset_tabs << "Queue flags: 0x" << std::hex << cmd_buffer_info->queueFlags << std::dec;
    }
        break;
    case ExecutionMarkerInfoType::PipelineBind:
    {
        PipelineInfo* pipeline_info = reinterpret_cast<PipelineInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Pipeline bind" << std::endl;
        ret << offset_tabs << "Bind point: " << pipeline_info->bindPoint << std::endl;
        ret << offset_tabs << "Api PSO hash: 0x" << std::hex << pipeline_info->apiPsoHash << std::dec;
    }
        break;
    case ExecutionMarkerInfoType::Draw:
    {
        DrawInfo* draw_info = reinterpret_cast<DrawInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << kStrDraw << std::endl;
        ret << offset_tabs << "Draw type: " << RgdUtils::GetExecMarkerApiTypeString((CrashAnalysisExecutionMarkerApiType)draw_info->drawType) << std::endl;
        ret << offset_tabs << "Vertex ID count: " << draw_info->vtxIdxCount << std::endl;
        ret << offset_tabs << "Instance count: " << draw_info->instanceCount << std::endl;
        ret << offset_tabs << "Start index: " << draw_info->startIndex << std::endl;
        ret << offset_tabs << "Vertex offset: " << draw_info->userData.vertexOffset << std::endl;
        ret << offset_tabs << "Instance offset: " << draw_info->userData.instanceOffset << std::endl;
        ret << offset_tabs << "Draw ID: " << draw_info->userData.drawId;
    }
        break;
    case ExecutionMarkerInfoType::DrawUserData:
    {
        DrawUserData* draw_user_data = reinterpret_cast<DrawUserData*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Draw user data" << std::endl;
        ret << offset_tabs << "Vertex offset: " << draw_user_data->vertexOffset << std::endl;
        ret << offset_tabs << "Instance offset: " << draw_user_data->instanceOffset << std::endl;
        ret << offset_tabs << "Draw ID: " << draw_user_data->drawId;
    }
        break;
    case ExecutionMarkerInfoType::Dispatch:
    {
        DispatchInfo* dispatch_info = reinterpret_cast<DispatchInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << kStrDispatch << std::endl;
        ret << offset_tabs << "Dispatch type: " << RgdUtils::GetExecMarkerApiTypeString((CrashAnalysisExecutionMarkerApiType)dispatch_info->dispatchType) << std::endl;
        ret << offset_tabs << "X: " << dispatch_info->threadX << std::endl;
        ret << offset_tabs << "Y: " << dispatch_info->threadY << std::endl;
        ret << offset_tabs << "Z: " << dispatch_info->threadZ;
    }
        break;
    case ExecutionMarkerInfoType::BarrierBegin:
    {
        BarrierBeginInfo* barrier_begin = reinterpret_cast<BarrierBeginInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Barrier begin" << std::endl;
        ret << offset_tabs << "Type: " << barrier_begin->type << std::endl;
        ret << offset_tabs << "Reason: " << barrier_begin->reason;
    }
        break;
    case ExecutionMarkerInfoType::BarrierEnd:
    {
        BarrierEndInfo* barrier_end = reinterpret_cast<BarrierEndInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Barrier end" << std::endl;
        ret << offset_tabs << "Pipeline stalls: " << barrier_end->pipelineStalls << std::endl;
        ret << offset_tabs << "Layout transition: " << barrier_end->layoutTransitions << std::endl;
        ret << offset_tabs << "Caches: " << barrier_end->caches;
    }
        break;
    case ExecutionMarkerInfoType::NestedCmdBuffer:
    {
        NestedCmdBufferInfo* nested_cmd_buffer = reinterpret_cast<NestedCmdBufferInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
        ret << offset_tabs << "Info type: " << "Nested command buffer" << std::endl;
        ret << offset_tabs << "Command buffer ID: 0x" << std::hex << nested_cmd_buffer->nestedCmdBufferId << std::dec;
    }
        break;
    default:
        assert(false);
        break;
    }
    return ret.str();
}

std::string RgdSerializer::EventExecMarkerEndToString(const CrashAnalysisExecutionMarkerEnd& exec_marker_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringUmd(exec_marker_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Marker source: " << RgdParsingUtils::ExtractMarkerSource(exec_marker_event.markerValue) << std::endl;
    ret << offset_tabs << "Command buffer ID: 0x" << std::hex << exec_marker_event.cmdBufferId << std::dec << std::endl;
    ret << offset_tabs << "Marker value: 0x" << std::hex << exec_marker_event.markerValue << std::dec;
    return ret.str();
}

std::string RgdSerializer::EventDebugNopToString(const CrashDebugNopData& debug_nop_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringUmd(debug_nop_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Command buffer ID: " << debug_nop_event.cmdBufferId << " (0x" << std::hex << debug_nop_event.cmdBufferId << ")" << std::dec << std::endl;
    ret << offset_tabs << "Timestamp value - begin: " << debug_nop_event.beginTimestampValue << " (0x" << std::hex << debug_nop_event.beginTimestampValue << ")" << std::dec << std::endl;
    ret << offset_tabs << "Timestamp value - end: " << debug_nop_event.endTimestampValue << " (0x" << std::hex << debug_nop_event.endTimestampValue << ")" << std::dec;
    return ret.str();
}

std::string RgdSerializer::EventVmPageFaultToString(const VmPageFaultEvent& page_fault_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    const char* process_name = (const char*)page_fault_event.processName;
    if (page_fault_event.processNameLength == 0)
    {
        process_name = kStrNotAvailable;
    }
    ret << RgdEventHeaderToStringKmd(page_fault_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Faulting VA: 0x" << std::hex << page_fault_event.faultVmAddress << std::dec << std::endl;
    ret << offset_tabs << "VM ID: 0x" << std::hex << page_fault_event.vmId << std::dec << std::endl;
    ret << offset_tabs << "Process ID: 0x" << std::hex << page_fault_event.processId << std::dec << std::endl;
    ret << offset_tabs << "Process name: " << process_name << std::endl;
    return ret.str();
}

std::string RgdSerializer::EventShaderWaveToString(const ShaderWaves& shader_wave_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << RgdEventHeaderToStringKmd(shader_wave_event, offset_tabs) << std::endl;
    ret << offset_tabs << "Version: " << shader_wave_event.version << std::endl;
    ret << offset_tabs << "GPU ID: " << shader_wave_event.gpuId << std::endl;
    ret << offset_tabs << "Type of Hang: " << RgdUtils::GetHangTypeString(shader_wave_event.typeOfHang) << std::endl;
    ret << offset_tabs << "GrbmStatusSeRegs version: " << shader_wave_event.grbmStatusSeRegs.version << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe0: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe0 << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe1: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe1 << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe2: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe2 << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe3: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe3 << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe4: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe4 << std::endl;
    ret << offset_tabs << offset_tabs << "GrbmStatusSe5: " << shader_wave_event.grbmStatusSeRegs.grbmStatusSe5 << std::endl;
    ret << offset_tabs << "Number of hung waves: " << shader_wave_event.numberOfHungWaves << std::endl;
    ret << offset_tabs << "Number of active waves: " << shader_wave_event.numberOfActiveWaves << std::endl;

    for (size_t wave_idx = 0; wave_idx < shader_wave_event.numberOfActiveWaves + shader_wave_event.numberOfHungWaves; ++wave_idx)
    {
        if (wave_idx < shader_wave_event.numberOfHungWaves)
        {
            ret << offset_tabs << "Hung wave info " << wave_idx << ":" << std::endl;
        }
        else
        {
            ret << offset_tabs << "Active wave info " << wave_idx << ":" << std::endl;
        }
        const WaveInfo& current_wave_info = shader_wave_event.waveInfos[wave_idx];
        ret << offset_tabs << offset_tabs << "Version: " << current_wave_info.version << std::endl;
        ret << offset_tabs << offset_tabs << "Shader id: 0x" << std::hex << current_wave_info.shaderId << std::dec << std::endl;
    }
    return ret.str();
}

std::string RgdSerializer::EventMmrRegisterDataToString(const MmrRegistersData& mmr_register_data_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << offset_tabs << "Version: " << mmr_register_data_event.version << std::endl;
    ret << offset_tabs << "GPU ID: " << mmr_register_data_event.gpuId << std::endl;
    ret << offset_tabs << "Number of registers: " << mmr_register_data_event.numRegisters << std::endl;
    for (size_t reg_info_idx = 0; reg_info_idx < mmr_register_data_event.numRegisters; ++reg_info_idx)
    {
        ret << offset_tabs << "Mmr Register info " << reg_info_idx << ":" << std::endl;
        ret << offset_tabs << offset_tabs << "Offset: 0x" << std::hex << mmr_register_data_event.registerInfos[reg_info_idx].offset << std::dec << std::endl;
        ret << offset_tabs << offset_tabs << "Data  : 0x" << std::hex << mmr_register_data_event.registerInfos[reg_info_idx].data << std::dec << std::endl;
    }

    return ret.str();
}

std::string RgdSerializer::EventWaveRegisterDataToString(const WaveRegistersData& wave_register_data_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << offset_tabs << "Version: " << wave_register_data_event.version << std::endl;
    ret << offset_tabs << "Shader ID: 0x" << std::hex << wave_register_data_event.shaderId << std::dec << std::endl;
    ret << offset_tabs << "Number of registers: " << wave_register_data_event.numRegisters << std::endl;
    for (size_t reg_info_idx = 0; reg_info_idx < wave_register_data_event.numRegisters; ++reg_info_idx)
    {
        ret << offset_tabs << "Wave Register info " << reg_info_idx << ":" << std::endl;
        ret << offset_tabs << offset_tabs << "Offset: 0x" << std::hex << wave_register_data_event.registerInfos[reg_info_idx].offset << std::dec << std::endl;
        ret << offset_tabs << offset_tabs << "Data  : 0x" << std::hex << wave_register_data_event.registerInfos[reg_info_idx].data << std::dec << std::endl;
    }

    return ret.str();

}

std::string RgdSerializer::EventSeInfoToString(const SeInfo& se_info_event, const std::string& offset_tabs)
{
    std::stringstream ret;
    ret << offset_tabs << "Version: " << se_info_event.version << std::endl;
    ret << offset_tabs << "GPU ID: " << se_info_event.gpuId << std::endl;
    ret << offset_tabs << "Number of SE registers: " << se_info_event.numSe << std::endl;
    for (size_t reg_info_idx = 0; reg_info_idx < se_info_event.numSe; ++reg_info_idx)
    {
        ret << offset_tabs << "SE Register info " << reg_info_idx << ":" << std::endl;
        ret << offset_tabs << offset_tabs << "Version: " << se_info_event.seRegsInfos[reg_info_idx].version << std::endl;
        ret << offset_tabs << offset_tabs << "spiDebugBusy      : 0x" << std::hex << se_info_event.seRegsInfos[reg_info_idx].spiDebugBusy << std::dec << std::endl;
        ret << offset_tabs << offset_tabs << "sqDebugStsGlobal  : 0x" << std::hex << se_info_event.seRegsInfos[reg_info_idx].sqDebugStsGlobal << std::dec
            << std::endl;
        ret << offset_tabs << offset_tabs << "sqDebugStsGlobal2 : 0x" << std::hex << se_info_event.seRegsInfos[reg_info_idx].sqDebugStsGlobal2 << std::dec
            << std::endl;
    }

    return ret.str();
}

static std::string SerializeRgdEventOccurrenceUmd(const RgdEventOccurrence& curr_event)
{
    // Serialize the event based on its type.
    std::stringstream txt;
    txt << "\tTime: " << curr_event.event_time << std::endl;
    const RgdEvent& rgd_event = *curr_event.rgd_event;
    switch (curr_event.rgd_event->header.eventId)
    {
    case (uint8_t)DDCommonEventId::RgdEventTimestamp:
    {
        const TimestampEvent& timestamp_event = static_cast<const TimestampEvent&>(rgd_event);
        txt << RgdSerializer::EventTimestampToString(timestamp_event, "\t") << std::endl;
    }
    break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerBegin:
    {
        const CrashAnalysisExecutionMarkerBegin& exec_marker_begin_event = static_cast<const CrashAnalysisExecutionMarkerBegin&>(rgd_event);
        txt << RgdSerializer::EventExecMarkerBeginToString(exec_marker_begin_event, "\t") << std::endl;
    }
    break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerInfo:
    {
        const CrashAnalysisExecutionMarkerInfo& exec_marker_info_event = static_cast<const CrashAnalysisExecutionMarkerInfo&>(rgd_event);
        txt << RgdSerializer::EventExecMarkerInfoToString(exec_marker_info_event, "\t") << std::endl;
    }
    break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerEnd:
    {
        const CrashAnalysisExecutionMarkerEnd& exec_marker_end_event = static_cast<const CrashAnalysisExecutionMarkerEnd&>(rgd_event);
        txt << RgdSerializer::EventExecMarkerEndToString(exec_marker_end_event, "\t") << std::endl;
    }
    break;
    case (uint8_t)UmdEventId::RgdEventCrashDebugNopData:
    {
        const CrashDebugNopData& debug_nop_event = static_cast<const CrashDebugNopData&>(rgd_event);
        txt << RgdSerializer::EventDebugNopToString(debug_nop_event, "\t") << std::endl;
    }
    break;
    break;
    default:
        // Notify in case there is an unknown event type.
        assert(false);
    }

    return txt.str();
}

static std::string SerializeRgdEventOccurrenceKmd(const RgdEventOccurrence& curr_event)
{
    // Serialize the event based on its type.
    std::stringstream txt;
    txt << "\tTime: " << curr_event.event_time << std::endl;
    const RgdEvent& rgd_event = *curr_event.rgd_event;
    switch (curr_event.rgd_event->header.eventId)
    {
    case uint8_t(DDCommonEventId::RgdEventTimestamp):
    {
        const TimestampEvent& timestamp_event = static_cast<const TimestampEvent&>(rgd_event);
        txt << RgdSerializer::EventTimestampToString(timestamp_event, "\t") << std::endl;
    }
    break;
    case uint8_t(KmdEventId::RgdEventVmPageFault):
    {
        const VmPageFaultEvent& page_fault_event = static_cast<const VmPageFaultEvent&>(rgd_event);
        txt << RgdSerializer::EventVmPageFaultToString(page_fault_event, "\t") << std::endl;
    }
    break;
    case uint8_t(KmdEventId::RgdEventShaderWaves):
    {
        const ShaderWaves& shader_waves_event = static_cast<const ShaderWaves&>(rgd_event);
        txt << RgdSerializer::EventShaderWaveToString(shader_waves_event, "\t") << std::endl;
    }
    break;
    case uint8_t(KmdEventId::RgdEventMmrRegisters):
    {
        const MmrRegistersData& mmr_register_data_event = static_cast<const MmrRegistersData&>(rgd_event);
        txt << RgdSerializer::EventMmrRegisterDataToString(mmr_register_data_event, "\t") << std::endl;
    }
    break;
    case uint8_t(KmdEventId::RgdEventSeInfo):
    {
        const SeInfo& se_info_event = static_cast<const SeInfo&>(rgd_event);
        txt << RgdSerializer::EventSeInfoToString(se_info_event, "\t") << std::endl;
    }
    break;
    case uint8_t(KmdEventId::RgdEventWaveRegisters):
    {
        const WaveRegistersData& wave_register_data_event = static_cast<const WaveRegistersData&>(rgd_event);
        txt << RgdSerializer::EventWaveRegisterDataToString(wave_register_data_event, "\t") << std::endl;
    }
    break;
    default:
        // Notify in case there is an unknown event type.
        assert(false);
    }

    return txt.str();
}

std::string RgdSerializer::SerializeUmdCrashEvents(const std::vector<RgdEventOccurrence>& umd_events)
{
    std::stringstream txt;
    for (uint32_t i = 0; i < umd_events.size(); i++)
    {
        txt << "Event #" << (i + 1) << ":" << std::endl;
        const RgdEventOccurrence& curr_event = umd_events[i];
        txt << SerializeRgdEventOccurrenceUmd(curr_event);
    }
    return txt.str();
}

std::string RgdSerializer::SerializeKmdCrashEvents(const std::vector<RgdEventOccurrence>& kmd_events)
{
    std::stringstream txt;
    for (uint32_t i = 0; i < kmd_events.size(); i++)
    {
        txt << "Event #" << (i + 1) << ":" << std::endl;
        const RgdEventOccurrence& curr_event = kmd_events[i];
        txt << SerializeRgdEventOccurrenceKmd(curr_event);
    }
    return txt.str();
}

std::string RgdSerializer::CrashAnalysisTimeInfoToString(const CrashAnalysisTimeInfo& time_info)
{
    std::stringstream txt;
    txt << "Time info:" << std::endl;
    txt << "\tStart time: " << time_info.start_time << std::endl;
    txt << "\tFrequency (Hz): " << time_info.frequency << std::endl;
    return txt.str();
}

std::string RgdSerializer::CodeObjectLoadEventsToString(const std::vector<RgdCodeObjectLoadEvent>& code_object_load_events)
{
    std::stringstream txt;

    for (uint32_t i = 0; i < code_object_load_events.size(); i++)
    {
        txt << "Event #" << (i + 1) << ":" << std::endl;
        txt << "\tPCI ID: " << code_object_load_events[i].pci_id << std::endl;
        txt << "\tLoader event type: "
            << (code_object_load_events[i].loader_event_type == RgdCodeObjectLoadEventType::kCodeObjectLoadToGpuMemory ? "Code object load to GPU memory"
                                                                                                                       : "Code object load from GPU memory")
            << std::endl;
        txt << "\tBase address: 0x" << std::hex << code_object_load_events[i].base_address << std::dec << std::endl;
        txt << "\tCode object hash high: 0x" << std::hex << code_object_load_events[i].code_object_hash.high << std::dec << std::endl;
        txt << "\tCode object hash low: 0x" << std::hex << code_object_load_events[i].code_object_hash.low << std::dec << std::endl;
        txt << "\tTimestamp: " << code_object_load_events[i].timestamp << std::endl;
    }
    return txt.str();
}

std::string RgdSerializer::CodeObjectsToString(const std::map<Rgd128bitHash, CodeObject>& code_objects_map)
{
    std::stringstream txt;
    size_t            seq_no = 1;
    for (const std::pair<Rgd128bitHash, CodeObject>& code_object_item : code_objects_map)
    {
        txt << "Code object #" << seq_no++ << ":" << std::endl;
        txt << "\tPCI ID: " << code_object_item.second.chunk_header.pci_id << std::endl;
        txt << "\tCode object hash high: 0x" << std::hex << code_object_item.second.chunk_header.code_object_hash.high << std::dec << std::endl;
        txt << "\tCode object hash low: 0x" << std::hex << code_object_item.second.chunk_header.code_object_hash.low << std::dec << std::endl;
    }
    return txt.str();
}

std::string RgdSerializer::PsoCorrelationsToString(const std::vector<RgdPsoCorrelation>& pso_correlations)
{
    std::stringstream txt;
    for (uint32_t i = 0; i < pso_correlations.size(); i++)
    {
        txt << "Correlation #" << (i + 1) << ":" << std::endl;
        txt << "\tAPI PSO hash: 0x" << std::hex << pso_correlations[i].api_pso_hash << std::dec << std::endl;
        txt << "\tInternal pipeline hash high: 0x" << std::hex << pso_correlations[i].internal_pipeline_hash.high << std::dec << std::endl;
        txt << "\tInternal pipeline hash low: 0x" << std::hex << pso_correlations[i].internal_pipeline_hash.low << std::dec << std::endl;
        txt << "\tAPI level object name: " << pso_correlations[i].api_level_object_name << std::endl;
    }
    return txt.str();
}
