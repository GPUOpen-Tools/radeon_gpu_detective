//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  execution marker serialization.
//=============================================================================

// Local.
#include "rgd_marker_data_serializer.h"
#include "rgd_exec_marker_tree_serializer.h"
#include "rgd_utils.h"

// C++.
#include <string>
#include <sstream>
#include <cassert>
#include <memory>
#include <regex>

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

// Function to calculate the display width of a string, considering special characters.
// Used for aligning the Shader info in the execution marker tree. The execution marker tree string includes the special characters.
static size_t CalculateDisplayWidth(const std::string& str)
{
    size_t      width = 0;
    std::locale loc;
    for (size_t i = 0; i < str.length(); ++i)
    {
        // UTF-8 multi-byte character.
        if ((unsigned char)str[i] >= 0xC0)
        {
            if ((unsigned char)str[i] < 0xE0)
            {
                // 2-byte character.
                i += 1;
            }
            else if ((unsigned char)str[i] < 0xF0)
            {
                // 3-byte character.
                i += 2;
            }
            else
            {
                // 4-byte character.
                i += 3;
            }
            
            // Assume each multi-byte character takes 1 display width
            width += 1;
        }
        else
        {
            // Count printable characters.
            width += std::isprint(str[i], loc) ? 1 : 0;
        }
    }
    return width;
}

// Aligns the shader info added in the execution marker tree.
static std::string AlignShaderInfoinExecMarkerTree(const std::string& input_tree)
{
    std::istringstream       stream(input_tree);
    std::string              line;
    std::vector<std::string> lines;
    size_t                   max_length = 0;

    // Split lines and find the maximum length of the left part
    while (std::getline(stream, line))
    {
        size_t pos = line.find("<--");
        if (pos != std::string::npos)
        {
            max_length = std::max(max_length, CalculateDisplayWidth(line.substr(0, pos)));
        }
        else
        {
            max_length = std::max(max_length, CalculateDisplayWidth(line));
        }
        lines.push_back(line);
    }

    // Reconstruct the lines with aligned comments
    std::ostringstream result;
    for (const auto& l : lines)
    {
        size_t pos = l.find("<--");
        if (pos != std::string::npos)
        {
            size_t display_width = CalculateDisplayWidth(l.substr(0, pos));
            result << l.substr(0, pos);
            result << std::string(max_length - display_width, ' ');
            result << l.substr(pos) << "\n";
        }
        else
        {
            result << l << "\n";
        }
    }

    return result.str();
}

// Converts started/finished status flags to the relevant Execution status.
static MarkerExecutionStatus MarkerStatusFlagsToExecutionStatus(MarkerExecutionStatusFlags flags)
{
    MarkerExecutionStatus exec_status = MarkerExecutionStatus::kNotStarted;
    if (flags.is_started)
    {
        exec_status = (flags.is_finished) ? MarkerExecutionStatus::kFinished : MarkerExecutionStatus::kInProgress;
    }
    return exec_status;
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

bool ExecMarkerDataSerializer::GenerateExecutionMarkerTree(const Config& user_config, const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    std::string& marker_tree)
{
    bool ret = false;
    std::stringstream txt_tree;
    if (command_buffer_exec_tree_.empty())
    {
        ret = BuildCmdBufferExecutionMarkerTreeNodes(user_config, umd_crash_data, cmd_buffer_events);
        assert(ret);
    }

    for (const std::pair<const uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>>& cmd_buffer_item : command_buffer_exec_tree_)
    {
        assert(cmd_buffer_item.second != nullptr);
        if (cmd_buffer_item.second != nullptr && !cmd_buffer_item.second->IsNestedCmdBuffer())
        {
            // Generate status string.
            txt_tree << "Command Buffer ID: 0x" << std::hex << cmd_buffer_item.first << std::dec;

            std::stringstream txt_cmd_buffer_info;
            if (cmd_buffer_info_map_.find(cmd_buffer_item.first) != cmd_buffer_info_map_.end())
            {
                // Print command buffer info. Example: (Queue type: Direct)
                const CmdBufferInfo& cmd_buffer_info = cmd_buffer_info_map_[cmd_buffer_item.first];
                txt_cmd_buffer_info << " (Queue type: " << RgdUtils::GetCmdBufferQueueTypeString((CmdBufferQueueType)cmd_buffer_info.queue) << ")";
                txt_tree << txt_cmd_buffer_info.str();
            }
            txt_tree << std::endl;
            txt_tree << "======================";
            uint64_t cmd_buffer_id = cmd_buffer_item.first;
            while (cmd_buffer_id > 0xF)
            {
                txt_tree << "=";
                cmd_buffer_id /= 0xF;
            }

            for (size_t cmd_buffer_info_text_length = txt_cmd_buffer_info.str().length(); cmd_buffer_info_text_length > 0; --cmd_buffer_info_text_length)
            {
                txt_tree << "=";
            }

            txt_tree << std::endl;
            txt_tree << command_buffer_exec_tree_[cmd_buffer_item.first]->TreeToString(user_config) << std::endl;
        }
    }

    marker_tree = txt_tree.str();

    // Post process the execution marker tree output to align the shader info in same column.
    marker_tree = AlignShaderInfoinExecMarkerTree(marker_tree);
    if (marker_tree.empty() && ret)
    {
        marker_tree = "INFO: execution marker tree is empty since no command buffers were in flight during the crash.";
    }
    else if (marker_tree.empty())
    {
        marker_tree = "ERROR: failed to parse the crash dump data.";
    }
    ret = !marker_tree.empty();

    return ret;
}

bool ExecMarkerDataSerializer::GenerateExecutionMarkerTreeToJson(const Config& user_config, const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    nlohmann::json& all_cmd_buffers_marker_tree_json)
{
    bool ret = true;
    
    nlohmann::json marker_tree_json;

    if (command_buffer_exec_tree_.empty())
    {
        ret = BuildCmdBufferExecutionMarkerTreeNodes(user_config, umd_crash_data, cmd_buffer_events);
        assert(ret);
    }

    for (const std::pair<const uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>>& cmd_buffer_item : command_buffer_exec_tree_)
    {
        marker_tree_json[kJsonElemCmdBufferIdElement] = cmd_buffer_item.first;

        std::string cmd_buffer_queue_type_str = kStrNotAvailable;
        if (cmd_buffer_info_map_.find(cmd_buffer_item.first) != cmd_buffer_info_map_.end())
        {
            // Add command buffer info element. Example: queue_type: "Direct"
            const CmdBufferInfo& cmd_buffer_info = cmd_buffer_info_map_[cmd_buffer_item.first];
            cmd_buffer_queue_type_str = RgdUtils::GetCmdBufferQueueTypeString((CmdBufferQueueType)cmd_buffer_info.queue);
        }
        marker_tree_json["cmd_buffer_queue_type"] = cmd_buffer_queue_type_str;
        command_buffer_exec_tree_[cmd_buffer_item.first]->TreeToJson(user_config, marker_tree_json);
        all_cmd_buffers_marker_tree_json[kJsonElemExecutionMarkerTree].push_back(marker_tree_json);
    }

    return ret;
}

bool ExecMarkerDataSerializer::GenerateExecutionMarkerSummaryList(const Config& user_config, const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    std::string& marker_summary_list_txt)
{
    bool ret = true;
    std::stringstream txt;

    if (command_buffer_exec_tree_.empty())
    {
        ret = BuildCmdBufferExecutionMarkerTreeNodes(user_config, umd_crash_data, cmd_buffer_events);
        assert(ret);
    }

    for (const std::pair<const uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>>& cmd_buffer_item : command_buffer_exec_tree_)
    {
        assert(cmd_buffer_item.second != nullptr);
        if (cmd_buffer_item.second != nullptr && !cmd_buffer_item.second->IsNestedCmdBuffer())
        {
            txt << "Command Buffer ID: 0x" << std::hex << cmd_buffer_item.first << std::dec << std::endl;
            txt << "======================";
            uint64_t cmd_buffer_id = cmd_buffer_item.first;
            while (cmd_buffer_id > 0xF)
            {
                txt << "=";
                cmd_buffer_id /= 0xF;
            }
            txt << std::endl;
            txt << command_buffer_exec_tree_[cmd_buffer_item.first]->SummaryListToString() << std::endl;
        }
    }

    marker_summary_list_txt = txt.str();
    if (marker_summary_list_txt.empty() && ret)
    {
        marker_summary_list_txt = "INFO: no markers in progress since no command buffers were in flight during the crash.";
    }
    else if(marker_summary_list_txt.empty())
    {
        marker_summary_list_txt = "ERROR: failed to parse the crash dump data.";
    }
    ret = !marker_summary_list_txt.empty();

    return ret;
}

bool ExecMarkerDataSerializer::GenerateExecutionMarkerSummaryListJson(const Config& user_config, const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
    nlohmann::json& marker_summary_list_json)
{
    bool ret = true;

    nlohmann::json summary_list_json;

    if (command_buffer_exec_tree_.empty())
    {
        ret = BuildCmdBufferExecutionMarkerTreeNodes(user_config, umd_crash_data, cmd_buffer_events);
        assert(ret);
    }

    for (const std::pair<const uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>>& cmd_buffer_item : command_buffer_exec_tree_)
    {
        summary_list_json[kJsonElemCmdBufferIdElement] = cmd_buffer_item.first;
        command_buffer_exec_tree_[cmd_buffer_item.first]->SummaryListToJson(summary_list_json);
        marker_summary_list_json[kJsonElemMarkersInProgress].push_back(summary_list_json);
    }

    return ret;
}

bool ExecMarkerDataSerializer::BuildCmdBufferMarkerStatus(const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events)
{
    bool ret = true;

    command_buffer_marker_status_.clear();

    for (uint32_t i = 0; i < umd_crash_data.events.size(); i++)
    {
        assert(umd_crash_data.events[i].rgd_event != nullptr);
        const RgdEventOccurrence& curr_event_occurrence = umd_crash_data.events[i];
        const RgdEvent& curr_event = *curr_event_occurrence.rgd_event;
        UmdEventId event_id = static_cast<UmdEventId>(curr_event_occurrence.rgd_event->header.eventId);
        if (event_id == UmdEventId::RgdEventCrashDebugNopData)
        {
            // Retrieve the execution markers for the relevant command buffer ID.
            const CrashDebugNopData& debug_nop_event = static_cast<const CrashDebugNopData&>(curr_event);

            if (cmd_buffer_events.find(debug_nop_event.cmdBufferId) != cmd_buffer_events.end()
                && debug_nop_event.beginTimestampValue != InitialExecutionMarkerValue
                && debug_nop_event.beginTimestampValue != FinalExecutionMarkerValue)
            {
                // Command buffer mapping is build only for command buffers which are in flight.
                assert(debug_nop_event.beginTimestampValue == InitialExecutionMarkerValue
                    || debug_nop_event.beginTimestampValue == FinalExecutionMarkerValue
                    || cmd_buffer_events.find(debug_nop_event.cmdBufferId) != cmd_buffer_events.end());

                std::vector<size_t> markers = cmd_buffer_events.at(debug_nop_event.cmdBufferId);
                std::unordered_map<uint32_t, MarkerExecutionStatusFlags> marker_status;

                // True if we found the marker which is the last to begin/end according to the KMD event data.
                bool is_last_begin_found = false;
                bool is_last_end_found = false;

                for (uint32_t m = 0; m < markers.size(); m++)
                {
                    const size_t curr_marker_index = markers[m];
                    const RgdEventOccurrence& curr_marker_event = umd_crash_data.events[curr_marker_index];
                    UmdEventId marker_event_id = static_cast<UmdEventId>(curr_marker_event.rgd_event->header.eventId);
                    if (marker_event_id == UmdEventId::RgdEventExecutionMarkerBegin)
                    {
                        const CrashAnalysisExecutionMarkerBegin& marker_begin =
                            static_cast<const CrashAnalysisExecutionMarkerBegin&>(*curr_marker_event.rgd_event);
                        uint32_t marker_value = marker_begin.markerValue;
                        if (is_last_begin_found)
                        {
                            // We already found the last marker that began execution. Since the markers are sorted
                            // by time, this means that this marker hasn't begun execution.
                            marker_status[marker_value].is_started = false;
                        }
                        else
                        {
                            marker_status[marker_value].is_started = true;

                            // Internal driver barrier markers are filtered out during the marker command buffer mapping.
                            // It is possible for debug_nop_event.beginTimestampValue to be equal to one of the filtered out marker values.
                            // So if check if marker_value is greater or equal than debug_nop_event.beginTimestampValue to set the last begin marker found flag.
                            // Compare marker values without the leading source bits as marker values are unique regardless of their source value.
                            is_last_begin_found = ((marker_value & kMarkerValueMask) >= (debug_nop_event.beginTimestampValue & kMarkerValueMask));

                            if ((marker_value & kMarkerValueMask) > (debug_nop_event.beginTimestampValue & kMarkerValueMask))
                            {
                                // If the marker value is greater than the beginTimeStampValue and control reaches here, it implies that the matching marker with the beginTimeStamp value is filtered out.
                                // Since the current marker is after the last begin marker (beginTimestampValue), set the status as "Not started".
                                marker_status[marker_value].is_started = false;
                            }
                        }
                    }
                    // If marker last to enter is found, ignore ExecutionMarkerEnd events after that point in time.
                    // As the end markers after the marker last to enter are not executed.
                    // Begin markers after the marker last to enter are processed and marked as not started.
                    else if (marker_event_id == UmdEventId::RgdEventExecutionMarkerEnd)
                    {
                        const CrashAnalysisExecutionMarkerEnd& marker_end =
                            static_cast<const CrashAnalysisExecutionMarkerEnd&>(*curr_marker_event.rgd_event);
                        uint32_t marker_value = marker_end.markerValue;
                        if (is_last_end_found)
                        {
                            // We already found the last marker that finished execution. Since the markers are sorted
                            // by time, this means that this marker hasn't finished execution.
                            marker_status[marker_value].is_finished = false;
                        }
                        else if (debug_nop_event.endTimestampValue == InitialExecutionMarkerValue)
                        {
                            marker_status[marker_value].is_finished = false;
                            is_last_end_found = true;
                        }
                        else
                        {
                            // Internal driver barrier markers are filtered out during the marker command buffer mapping.
                            // It is possible for debug_nop_event.endTimestampValue to be equal to one of the filtered out marker values.
                            // So if check if marker_value is greater or equal than debug_nop_event.endTimestampValue to set the last end marker found flag.
                            // Compare marker values without the leading source bits as marker values are unique regardless of their source value.
                            is_last_end_found = ((marker_value & kMarkerValueMask) >= (debug_nop_event.endTimestampValue & kMarkerValueMask));
                            marker_status[marker_value].is_finished = true;

                            if ((marker_value & kMarkerValueMask) > (debug_nop_event.endTimestampValue & kMarkerValueMask))
                            {
                                // If the marker value is greater than the endTimestampValue and control reaches here, it implies that the matching marker with the endTimestampValue value is filtered out.
                                // Since the current marker is after the last end marker (endTimestampValue), set the status as "Not finished".
                                marker_status[marker_value].is_finished = false;
                            }
                        }
                    }
                }

                for (auto iter = marker_status.begin(); iter != marker_status.end(); iter++)
                {
                    // Get the execution status tristate and assign it to the relevant command buffer's
                    // [value ---> status] map.
                    MarkerExecutionStatus exec_status = MarkerStatusFlagsToExecutionStatus(iter->second);
                    command_buffer_marker_status_[debug_nop_event.cmdBufferId][iter->first] = exec_status;
                }
            }
            else if (debug_nop_event.beginTimestampValue != FinalExecutionMarkerValue && debug_nop_event.beginTimestampValue != InitialExecutionMarkerValue)
            {
                ret = false;
            }
        }
    }
    return ret;
}

bool ExecMarkerDataSerializer::BuildCmdBufferExecutionMarkerTreeNodes(const Config& user_config, const CrashData& umd_crash_data,
    const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events)
{
    bool ret = true;
    bool should_abort = false;

    if (command_buffer_marker_status_.empty())
    {
        ret = BuildCmdBufferMarkerStatus(umd_crash_data, cmd_buffer_events);
        assert(ret);
    }

    for (uint32_t i = 0; !should_abort && i < umd_crash_data.events.size(); i++)
    {
        assert(umd_crash_data.events[i].rgd_event != nullptr);
        const RgdEventOccurrence& curr_event_occurrence = umd_crash_data.events[i];
        const RgdEvent& curr_event = *curr_event_occurrence.rgd_event;
        UmdEventId event_id = static_cast<UmdEventId>(curr_event_occurrence.rgd_event->header.eventId);

        uint64_t pipeline_api_pso_hash = 0;
        bool     is_shader_in_flight   = false;

        if (event_id == UmdEventId::RgdEventCrashDebugNopData)
        {
            // Retrieve the execution markers for the relevant command buffer ID.
            const CrashDebugNopData& debug_nop_event = static_cast<const CrashDebugNopData&>(curr_event);

            // Retrieve the status map for this command buffer ID.
            auto cmd_buffer_status_iter = command_buffer_marker_status_.find(debug_nop_event.cmdBufferId);

            if (debug_nop_event.beginTimestampValue != InitialExecutionMarkerValue
                && debug_nop_event.beginTimestampValue != FinalExecutionMarkerValue
                && cmd_buffer_status_iter != command_buffer_marker_status_.end())
            {
                // Command buffer mapping is build only for command buffers which are in flight.
                assert(debug_nop_event.beginTimestampValue == InitialExecutionMarkerValue
                    || debug_nop_event.beginTimestampValue == FinalExecutionMarkerValue
                    || cmd_buffer_status_iter != command_buffer_marker_status_.end());

                std::unordered_map<uint32_t, MarkerExecutionStatus> cmd_buffer_exec_status_map;
                if (cmd_buffer_status_iter != command_buffer_marker_status_.end())
                {
                    cmd_buffer_exec_status_map = cmd_buffer_status_iter->second;
                }

                command_buffer_exec_tree_[debug_nop_event.cmdBufferId] = std::make_unique<ExecMarkerTreeSerializer>(user_config, cmd_buffer_exec_status_map,
                    debug_nop_event.beginTimestampValue, debug_nop_event.endTimestampValue);

                cmd_buffer_ids_create_ordered_.push_back(debug_nop_event.cmdBufferId);

                if (nested_cmd_buffer_ids_set.find(debug_nop_event.cmdBufferId) != nested_cmd_buffer_ids_set.end())
                {
                    // This command buffer is a nested command buffer.
                    command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->SetIsNestedCmdBuffer(true);
                }

                auto iter = cmd_buffer_events.find(debug_nop_event.cmdBufferId);
                assert(iter != cmd_buffer_events.end());
                if (iter != cmd_buffer_events.end())
                {
                    const std::vector<size_t>& cmd_buffer_markers = iter->second;
                    for (uint32_t m = 0; !should_abort && m < cmd_buffer_markers.size(); m++)
                    {
                        const size_t curr_marker_index = cmd_buffer_markers[m];
                        const RgdEventOccurrence& curr_marker_event = umd_crash_data.events[curr_marker_index];
                        UmdEventId marker_event_id = static_cast<UmdEventId>(curr_marker_event.rgd_event->header.eventId);
                        if (marker_event_id == UmdEventId::RgdEventExecutionMarkerBegin)
                        {
                            const CrashAnalysisExecutionMarkerBegin& marker_begin =
                                static_cast<const CrashAnalysisExecutionMarkerBegin&>(*curr_marker_event.rgd_event);
                            uint32_t marker_value = marker_begin.markerValue;
                            std::string marker_name = (marker_begin.markerStringSize > 0)
                                    ? std::string(reinterpret_cast<const char*>(marker_begin.markerName), marker_begin.markerStringSize)
                                    : std::string(kStrNotAvailable);

                            RgdCrashingShaderInfo crashing_shader_info;
                            if (is_shader_in_flight)
                            {
                                // If shader is in flight, update the Marker Node with related shader info.
                                assert(in_flight_shader_api_pso_hashes_to_shader_info_.find(pipeline_api_pso_hash) !=
                                       in_flight_shader_api_pso_hashes_to_shader_info_.end());
                                if (in_flight_shader_api_pso_hashes_to_shader_info_.find(pipeline_api_pso_hash) !=
                                    in_flight_shader_api_pso_hashes_to_shader_info_.end())
                                {
                                    crashing_shader_info.crashing_shader_ids =
                                        in_flight_shader_api_pso_hashes_to_shader_info_[pipeline_api_pso_hash].crashing_shader_ids;
                                    crashing_shader_info.api_stages =
                                        in_flight_shader_api_pso_hashes_to_shader_info_[pipeline_api_pso_hash].api_stages;
                                    crashing_shader_info.source_file_names =
                                        in_flight_shader_api_pso_hashes_to_shader_info_[pipeline_api_pso_hash].source_file_names;
                                    crashing_shader_info.source_entry_point_names =
                                        in_flight_shader_api_pso_hashes_to_shader_info_[pipeline_api_pso_hash].source_entry_point_names;
                                }
                            }
                            command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->PushMarkerBegin(
                                curr_marker_event.event_time, marker_value, pipeline_api_pso_hash, is_shader_in_flight, marker_name, crashing_shader_info);
                        }
                        else if (marker_event_id == UmdEventId::RgdEventExecutionMarkerInfo)
                        {
                            const CrashAnalysisExecutionMarkerInfo& exec_marker_info_event =
                                static_cast<const CrashAnalysisExecutionMarkerInfo&>(*curr_marker_event.rgd_event);
                            uint32_t marker_value = exec_marker_info_event.marker;

                            uint8_t* marker_info = const_cast<uint8_t*>(exec_marker_info_event.markerInfo);
                            ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);

                            if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::CmdBufStart)
                            {
                                // Update command buffer info.
                                CmdBufferInfo* cmd_buffer_info = reinterpret_cast<CmdBufferInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
                                cmd_buffer_info_map_[debug_nop_event.cmdBufferId] = *cmd_buffer_info;
                            }
                            else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::Draw
                                || exec_marker_info_header->infoType == ExecutionMarkerInfoType::Dispatch)
                            {
                                // Update Draw or Dispatch MarkerNode with additional info.
                                command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->UpdateMarkerInfo(marker_value, exec_marker_info_event.markerInfo);
                            }
                            else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::BarrierBegin)
                            {
                                // Update the Barrier MarkerNode with additional info.
                                // Currently MarkerNode holds space for one CrashAnalysisExecutionMarkerInfo (marker_info[64]).
                                // Barrier type marker is the only marker which has two "CrashAnalysisExecutionMarkerInfo" events followed by the CrashAnalysisExecutionMarkerBegin marker.
                                // These two MarkerInfo event types are - BarrierBegin and BarrierEnd.
                                // Since information provided with BarrierEnd Info marker is not in use, the available marker_info[64] is used to store the information from BarrierBegin Info marker and BarrierEnd Info marker is ignored.
                                // If info provided with BarrierEnd is needed in future, update the MarkerNode struct to hold additional marker_info[64] from the BarrierEnd event.
                                command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->UpdateMarkerInfo(marker_value, exec_marker_info_event.markerInfo);
                            }
                            else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::PipelineBind)
                            {
                                PipelineInfo* pipeline_info = reinterpret_cast<PipelineInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
                                pipeline_api_pso_hash       = pipeline_info->apiPsoHash;
                                is_shader_in_flight         = IsShaderInFlight(pipeline_api_pso_hash);
                            }
                            else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::NestedCmdBuffer)
                            {
                                // Update Nested Command Buffer MarkerNode with additional info.
                                command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->UpdateMarkerInfo(marker_value, exec_marker_info_event.markerInfo);

                                // Mark the relevant command buffer as a nested command buffer.
                                NestedCmdBufferInfo* nested_cmd_buffer_info = reinterpret_cast<NestedCmdBufferInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
                                nested_cmd_buffer_ids_set.insert(nested_cmd_buffer_info->nestedCmdBufferId);

                                // Mark that current command buffer executes another nested command buffer.
                                command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->SetIsExecutesNestedCmdBuffer(true);
                            }
                        }
                        else if (marker_event_id == UmdEventId::RgdEventExecutionMarkerEnd)
                        {
                            const CrashAnalysisExecutionMarkerEnd& marker_end =
                                static_cast<const CrashAnalysisExecutionMarkerEnd&>(*curr_marker_event.rgd_event);
                            uint32_t marker_value = marker_end.markerValue;

                            command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->PushMarkerEnd(curr_marker_event.event_time, marker_value);
                        }
                    }
                }
                else
                {
                    should_abort = true;
                }
            }
        }
    }

    if (should_abort)
    {
        ret = false;
    }
    else
    {
        // Check if any of the command buffers execute nested command buffers.
        // If yes, move the Marker Nodes from nested command buffer to the relevant Marker Node of the parent command buffer.
        // As this process may update the tree structure, it should be completed before calling UpdateSameStatusMarkerNodesCount() function.
        UpdateMarkerTreeNodesForNestedCmdBuffer(user_config);

        for (const std::pair<const uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>>& pair : command_buffer_exec_tree_)
        {
            // 
            ExecMarkerTreeSerializer& cmd_buffer_tree_serializer = *pair.second;

            // Build the look ahead counter of consecutive same status nodes on the same level.
            // UpdateSameStatusMarkerNodesCount() should be called after the complete Marker Nodes tree is built and all the required update operations are performaed.
            // For example UpdateMarkerTreeNodesForNestedCmdBuffer() may update the tree structure, so it is called before UpdateSameStatusMarkerNodesCount().
            cmd_buffer_tree_serializer.UpdateSameStatusMarkerNodesCount();

            // Validate if any 'Begin' marker is missing  a  matching 'End' marker and report the warnings to the user.
            cmd_buffer_tree_serializer.ValidateExecutionMarkers();
        }
    }

    return ret;
}

void ExecMarkerDataSerializer::UpdateMarkerTreeNodesForNestedCmdBuffer(const Config& user_config)
{
    // Iterate through each command buffer in the reverse order of events as received by RGD.
    for (auto it = cmd_buffer_ids_create_ordered_.rbegin(); it != cmd_buffer_ids_create_ordered_.rend(); ++it)
    {
        uint64_t cmd_buffer_id = *it;
        ExecMarkerTreeSerializer* parent_cmd_buffer_tree_ptr = command_buffer_exec_tree_[cmd_buffer_id].get();
        assert(parent_cmd_buffer_tree_ptr != nullptr);
        if (parent_cmd_buffer_tree_ptr != nullptr && parent_cmd_buffer_tree_ptr->IsExecutesNestedCmdBuffer())
        {
            // If the current command buffer (parent) executes another command buffer (child) as a nested command buffer and it is in flight at the time of the crash
            // Move the marker nodes from the nested command buffer as child nodes to the relevant marker node of the parent command buffer.

            // Get the list of nested command buffer IDs for the Execution Marker Tree.
            std::vector<uint64_t> nested_cmd_buffer_ids_for_exec_tree = parent_cmd_buffer_tree_ptr->GetNestedCmdBufferIdsForExecTree();

            // For each nested command buffer ID, update the marker tree nodes.
            for (uint64_t nested_cmd_buffer_id : nested_cmd_buffer_ids_for_exec_tree)
            {
                auto nested_cmd_buffer_tree_iter = command_buffer_exec_tree_.find(nested_cmd_buffer_id);
                if (nested_cmd_buffer_tree_iter != command_buffer_exec_tree_.end())
                {
                    // Get the nested command buffer queue type.
                    uint8_t nested_command_buffer_queue_type = 0xf;
                    assert(cmd_buffer_info_map_.find(nested_cmd_buffer_id) != cmd_buffer_info_map_.end());
                    if (cmd_buffer_info_map_.find(nested_cmd_buffer_id) != cmd_buffer_info_map_.end())
                    {
                        nested_command_buffer_queue_type = cmd_buffer_info_map_[nested_cmd_buffer_id].queue;
                    }
                    // Update the marker tree nodes for the nested command buffer.
                    parent_cmd_buffer_tree_ptr->UpdateNestedCmdBufferMarkerNodes(
                        nested_cmd_buffer_id, *nested_cmd_buffer_tree_iter->second, nested_command_buffer_queue_type);
                }
            }
        }
    }
}

// Returns true if the shader with the given API PSO hash was in flight at the time of the crash.
bool ExecMarkerDataSerializer::IsShaderInFlight(uint64_t api_pso_hash)
{
    return in_flight_shader_api_pso_hashes_to_shader_info_.find(api_pso_hash) != in_flight_shader_api_pso_hashes_to_shader_info_.end();
}
