//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer different data elements.
//=============================================================================
// Local.
#include "rgd_serializer.h"
#include "rgd_utils.h"
#include "rgd_parsing_utils.h"
#include "rgd_version_info.h"

// C++.
#include <string>
#include <sstream>
#include <cassert>
#include <iomanip>

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

bool RgdSerializer::ToString(const Config& user_config, const system_info_utils::SystemInfo& system_info, std::string& system_info_txt)
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
    txt << "Dev driver version: " << system_info.devdriver.tag << std::endl;
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
        txt << "\tDevice revision ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.e_rev << std::dec << std::endl;
        txt << "\tDevice family ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.family << std::dec << std::endl;
        txt << "\tDevice graphics engine ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.gfx_engine << std::dec << std::endl;
        txt << "\tDevice PCI revision ID: " << "0x" << std::hex << system_info.gpus[i].asic.id_info.revision << std::dec << std::endl;
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
                                      const TraceProcessInfo&              process_info,
                                      const system_info_utils::SystemInfo& system_info,
                                      const TraceChunkApiInfo& api_info,
                                      std::string& input_info_str)
{
    std::stringstream txt;

    txt << "===================" << std::endl;
    txt << "CRASH ANALYSIS FILE" << std::endl;
    txt << "===================" << std::endl << std::endl;

    txt << "Crash analysis file format version: 1.0" << std::endl;
    std::string rgd_version_string;
    RgdUtils::TrimLeadingAndTrailingWhitespace(RGD_TITLE, rgd_version_string);
    txt << "RGD CLI version used: " << rgd_version_string << std::endl;
    txt << "Input crash dump file creation time: " << RgdUtils::GetFileCreationTime(user_config.crash_dump_file) << std::endl;
    txt << "Input crash dump file name: " << user_config.crash_dump_file << std::endl;
    txt << "Crashing executable full path: " << (process_info.process_path.empty() ? kStrNotAvailable : process_info.process_path)
        << " (PID: " << process_info.process_id << ")" << std::endl;
    txt << "API: " << RgdUtils::GetApiString(api_info.apiType) << std::endl;
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