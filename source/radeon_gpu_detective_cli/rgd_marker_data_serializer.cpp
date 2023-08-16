//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  execution marker serialization.
//=============================================================================

// Local.
#include "rgd_marker_data_serializer.h"
#include "rgd_exec_marker_tree_serializer.h"

// C++.
#include <string>
#include <sstream>
#include <cassert>
#include <memory>

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

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
        // Generate status string.
        txt_tree << "Command Buffer ID: 0x" << std::hex << cmd_buffer_item.first << std::dec << std::endl;
        txt_tree << "======================";
        uint64_t cmd_buffer_id = cmd_buffer_item.first;
        while (cmd_buffer_id > 0xF)
        {
            txt_tree << "=";
            cmd_buffer_id /= 0xF;
        }
        txt_tree << std::endl;
        txt_tree << command_buffer_exec_tree_[cmd_buffer_item.first]->TreeToString(user_config) << std::endl;
    }

    marker_tree = txt_tree.str();
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
                            is_last_begin_found = (marker_value == debug_nop_event.beginTimestampValue);
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
                            is_last_end_found = (marker_value == debug_nop_event.endTimestampValue);
                            marker_status[marker_value].is_finished = true;
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

                            command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->PushMarkerBegin(curr_marker_event.event_time, marker_value, marker_name);
                        }
                        else if (marker_event_id == UmdEventId::RgdEventExecutionMarkerEnd)
                        {
                            const CrashAnalysisExecutionMarkerEnd& marker_end =
                                static_cast<const CrashAnalysisExecutionMarkerEnd&>(*curr_marker_event.rgd_event);
                            uint32_t marker_value = marker_end.markerValue;

                            command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->PushMarkerEnd(curr_marker_event.event_time, marker_value);
                        }
                    }

                    // Build the look ahead counter of consecutive same status nodes on the same level.
                    command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->UpdateSameStatusMarkerNodesCount();

                    // Validate if any 'Begin' marker is missing  a  matching 'End' marker and report the warnings to the user.
                    command_buffer_exec_tree_[debug_nop_event.cmdBufferId]->ValidateExecutionMarkers();
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

    return ret;
}
