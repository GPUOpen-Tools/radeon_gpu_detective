//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - BEGIN ***

static const char* kJsonElemTimestampElement = "timestamp";

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

void RgdSerializerJson::SetInputInfo(const Config& user_config, const uint64_t crashing_process_id, const system_info_utils::SystemInfo& system_info)
{
    json_["crash_analysis_file"]["input_crash_dump_file_name"] = user_config.crash_dump_file;
    json_["crash_analysis_file"]["input_crash_dump_file_creation_time"] = RgdUtils::GetFileCreationTime(user_config.crash_dump_file);
    std::string rgd_version_string;
    RgdUtils::TrimLeadingAndTrailingWhitespace(RGD_TITLE, rgd_version_string);
    json_["crash_analysis_file"]["rgd_cli_version"] = rgd_version_string;
    json_["crash_analysis_file"]["json_schema_version"] = RGD_JSON_SCHEMA_VERSION;
    json_["crash_analysis_file"]["crashing_process_id"] = crashing_process_id;
    json_["crash_analysis_file"]["crashing_process_path"] = RgdUtils::GetFullPathStringForPid((uint32_t)crashing_process_id, system_info);
}

void RgdSerializerJson::SetSystemInfoData(const Config& user_config, const system_info_utils::SystemInfo& system_info)
{
    if (user_config.is_extended_sysinfo)
    {
        // Version.
        json_["system_info"]["system_info_version"]["major"] = system_info.version.major;
        json_["system_info"]["system_info_version"]["minor"] = system_info.version.minor;
        json_["system_info"]["system_info_version"]["patch"] = system_info.version.patch;
        json_["system_info"]["system_info_version"]["build"] = system_info.version.build;
    }

    // Driver info.
    json_["system_info"]["driver_info"]["packaging_version"] = system_info.driver.packaging_version;
    json_["system_info"]["driver_info"]["software_version"] = system_info.driver.software_version;
    json_["system_info"]["driver_info"]["dev_driver_version"] = system_info.devdriver.tag;

    // Operating system info.
    json_["system_info"]["os"]["name"] = system_info.os.name;
    json_["system_info"]["os"]["description"] = system_info.os.desc;
    json_["system_info"]["os"]["hostname"] = system_info.os.hostname;
    json_["system_info"]["os"]["memory"] = nlohmann::json::array();
    json_["system_info"]["os"]["memory"].push_back({
        {"physical_bytes", system_info.os.memory.physical},
        {"swap_bytes", system_info.os.memory.swap}
        });

    // CPU info.
    json_["system_info"]["cpu"] = nlohmann::json::array();
    for (uint32_t i = 0; i < system_info.cpus.size(); i++)
    {
        std::string cpu_name;
        RgdUtils::TrimLeadingAndTrailingWhitespace(system_info.cpus[i].name, cpu_name);
        if (user_config.is_extended_sysinfo)
        {
            json_["system_info"]["cpu"].push_back({
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
            json_["system_info"]["cpu"].push_back({
                {"name", cpu_name },
                {"architecture", system_info.cpus[i].architecture },
                {"cpu_id", system_info.cpus[i].cpu_id },
                {"virtualization", system_info.cpus[i].virtualization },
                });
        }
    }

    // GPU info.
    json_["system_info"]["gpu"] = nlohmann::json::array();
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

            json_["system_info"]["gpu"].push_back({
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
            json_["system_info"]["gpu"].push_back({
                {"name", system_info.gpus[g].name },
                {"device_id", system_info.gpus[g].asic.id_info.device },
                {"device_revision_id", system_info.gpus[g].asic.id_info.e_rev },
                {"device_family_id", system_info.gpus[g].asic.id_info.family },
                {"device_graphics_engine_id", system_info.gpus[g].asic.id_info.gfx_engine },
                {"device_pci_revision_id", system_info.gpus[g].asic.id_info.revision },
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

bool RgdSerializerJson::SaveToFile(const Config& user_config) const
{
    std::string contents;

    if (user_config.is_compact_json)
    {
        contents = json_.dump();
    }
    else
    {
        const int kIndent = 4;
        contents = json_.dump(kIndent);
    }
    return RgdUtils::WriteTextFile(user_config.output_file_json, contents);
}
