//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  execution marker tree serialization.
//=============================================================================

// local
#include "rgd_exec_marker_tree_serializer.h"

// C++.
#include <string>
#include <sstream>
#include <cassert>

// RGD
#include "rgd_utils.h"

// *** INTERNALLY-LINKED CONSTANTS - BEGIN ***

static const char* kJsonElemMarkers = "markers";
static const char* kJsonElemEvents = "events";

static const char* kExecTreeDummyRootString = "rgd_internal_dummy_exec_marker_tree_root_node";

static const char* kMarkerSrcApplication = "App";
static const char* kMarkerSrcAPILayer = "Driver-DX12";
static const char* kMarkerSrcPal = "Driver-PAL";
static const char* kMarkerSrcHw = "GPU HW";

// *** INTERNALLY-LINKED CONSTANTS - ENDS ***

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***
// Returns the string representation of a MarkerExecutionStatus.
static std::string MarkerExecStatusToString(MarkerExecutionStatus exec_status)
{
    const char* kExecStatusStrNotStarted = "not started";
    const char* kExecStatusStrInProgress = "in progress";
    const char* kExecStatusStrFinished = "finished";
    std::string ret;
    switch (exec_status)
    {
    case MarkerExecutionStatus::kNotStarted:
        ret = kExecStatusStrNotStarted;
        break;
    case MarkerExecutionStatus::kInProgress:
        ret = kExecStatusStrInProgress;
        break;
    case MarkerExecutionStatus::kFinished:
        ret = kExecStatusStrFinished;
        break;
    default:
        break;
    }
    return ret;
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

void ExecMarkerTreeSerializer::PushMarkerBegin(uint64_t timestamp, uint32_t marker_value, const std::string& str)
{
#ifdef _DEBUG
    UpdateAndValidateTimestamp(timestamp);
#endif

    MarkerNode new_marker_node(timestamp, marker_value, str);
    if (current_stack_.empty())
    {
        marker_nodes_.push_back(std::move(new_marker_node));
        current_stack_.push_back(&marker_nodes_.back());
    }
    else
    {
        MarkerNode* curr_marker_node = current_stack_.back();
        curr_marker_node->child_markers.push_back(std::move(new_marker_node));
        current_stack_.push_back(&curr_marker_node->child_markers.back());
    }
}

void ExecMarkerTreeSerializer::PushMarkerEnd(uint64_t timestamp, uint32_t marker_value)
{
    (void)marker_value;
#ifdef _DEBUG
    UpdateAndValidateTimestamp(timestamp);
#endif

    assert(!current_stack_.empty());
    if (!current_stack_.empty())
    {
        current_stack_.back()->timestamp_end_ = timestamp;
        assert(current_stack_.back()->marker_value == marker_value);
        current_stack_.pop_back();
    }
}

void ExecMarkerTreeSerializer::ValidateExecutionMarkers()
{
    if (!current_stack_.empty())
    {
        // current_stack_ holds the 'begin' markers when building the execution marker tree nodes in marker_nodes_.
        // 'end' marker is missing for each remaining 'begin' marker in the stack.
        std::stringstream txt;

        // Add a info message for each missing 'end' marker.
        while (!current_stack_.empty())
        {
            const MarkerNode& current_node = *current_stack_.back();
            if (GetItemStatus(current_node) == MarkerExecutionStatus::kInProgress)
            {
                std::stringstream console_msg;
                console_msg << "detected a missing 'End' (pop) event for marker named \"" << current_node.marker_str << "\". Marker hierarchy might be impacted.";
                txt << "Note: " << console_msg.str() << std::endl;
                RgdUtils::PrintMessage(console_msg.str().c_str(), RgdMessageType::kInfo, true);
            }
            current_stack_.pop_back();
        }

        missing_markers_info_txt_ = txt.str();
    }
}

std::string ExecMarkerTreeSerializer::SummaryListToString() const
{
    std::stringstream txt;

    // Add missing pop markers info for the summary list.
    if (!missing_markers_info_txt_.empty())
    {
        txt << std::endl << missing_markers_info_txt_ << std::endl;
    }

    std::vector<const MarkerNode*> marker_stack;
    for (const auto& marker_node : marker_nodes_)
    {
        bool is_atleast_one_child_in_progress_place_holdler = false;
        marker_stack.push_back(&marker_node);
        txt << GenerateSummaryString(marker_stack, is_atleast_one_child_in_progress_place_holdler);
        marker_stack.pop_back();
    }
    return txt.str();
}

void ExecMarkerTreeSerializer::SummaryListToJson(nlohmann::json& summary_list_json) const
{
    std::vector<const MarkerNode*> marker_stack;

    summary_list_json[kJsonElemMarkers] = nlohmann::json::array();
    for (const auto& marker_node : marker_nodes_)
    {
        bool is_atleast_one_child_in_progress_place_holdler = false;
        marker_stack.push_back(&marker_node);
        GenerateSummaryJson(marker_stack, summary_list_json, is_atleast_one_child_in_progress_place_holdler);
        marker_stack.pop_back();
    }
}

void ExecMarkerTreeSerializer::GenerateSummaryJson(std::vector<const MarkerNode*>& marker_stack,
    nlohmann::json& marker_summary_json, bool& is_atleast_one_child_in_progress) const
{
    const MarkerNode* const node = marker_stack.back();
    assert(node != nullptr);
    if (node != nullptr)
    {
        if (GetItemStatus(*node) == MarkerExecutionStatus::kInProgress)
        {
            is_atleast_one_child_in_progress = true;
            bool is_atleast_one_child_of_current_in_progress = false;

            for (const auto& child_marker : node->child_markers)
            {
                marker_stack.push_back(&child_marker);
                GenerateSummaryJson(marker_stack, marker_summary_json, is_atleast_one_child_of_current_in_progress);
                marker_stack.pop_back();
            }

            if (!is_atleast_one_child_of_current_in_progress)
            {
                // When none of the child nodes are "in progress" or there are no child marker nodes, current node is the last "in progress" node in the tree path.
                // Print the path from root until the current node.
                std::stringstream txt;

                for (size_t i = 0, count = marker_stack.size(); i + 1 < count; ++i)
                {
                    txt << marker_stack[i]->marker_str << '/';
                }
                txt << marker_stack.back()->marker_str;
                marker_summary_json[kJsonElemMarkers].push_back(txt.str());
            }
        }
    }
}

std::string ExecMarkerTreeSerializer::TreeToString(const Config& user_config) const
{
    std::vector<bool> is_last_on_level;
    std::stringstream txt;

    // Add missing pop markers info for execution marker tree.
    if (!missing_markers_info_txt_.empty())
    {
        txt << std::endl << missing_markers_info_txt_ << std::endl;
    }

    // Dummy parent/root node to hold top level markers list.
    MarkerNode dummy_root(0,0,kExecTreeDummyRootString);

    dummy_root.child_markers = marker_nodes_;

    txt << TreeNodeToString(is_last_on_level, dummy_root, user_config);
    return txt.str();
}

void ExecMarkerTreeSerializer::TreeToJson(const Config& user_config, nlohmann::json& marker_tree_json) const
{
    for (const auto& item : marker_nodes_)
    {
        nlohmann::json marker_node_json;
        TreeNodeToJson(item, marker_node_json, user_config);
        marker_tree_json[kJsonElemEvents].push_back(marker_node_json);
    }
}

void ExecMarkerTreeSerializer::TreeNodeToJson(const MarkerNode& node, nlohmann::json& marker_node_json, const Config& user_config) const
{
    const char* kJsonElemMarkerExecStatus = "execution_status";
    const char* kJsonElemMarkerSrc = "marker_source";

    marker_node_json["name"] = node.marker_str;

    // Execution status.
    const MarkerExecutionStatus status = GetItemStatus(node);
    switch (status)
    {
    case MarkerExecutionStatus::kNotStarted:
        marker_node_json[kJsonElemMarkerExecStatus] = "not_started";
        break;
    case MarkerExecutionStatus::kInProgress:
        marker_node_json[kJsonElemMarkerExecStatus] = "in_progress";
        break;
    case MarkerExecutionStatus::kFinished:
        marker_node_json[kJsonElemMarkerExecStatus] = "finished";
        break;
    default:
        assert(0);
    }

    // Add marker source information.
    if (user_config.is_marker_src)
    {
        switch ((node.marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen))
        {
        case (uint32_t)CrashAnalysisExecutionMarkerSource::Application:
            marker_node_json[kJsonElemMarkerSrc] = kMarkerSrcApplication;
            break;
        case (uint32_t)CrashAnalysisExecutionMarkerSource::APILayer:
            marker_node_json[kJsonElemMarkerSrc] = kMarkerSrcAPILayer;
            break;
        case (uint32_t)CrashAnalysisExecutionMarkerSource::PAL:
            marker_node_json[kJsonElemMarkerSrc] = kMarkerSrcPal;
            break;
        case (uint32_t)CrashAnalysisExecutionMarkerSource::Hardware:
            marker_node_json[kJsonElemMarkerSrc] = kMarkerSrcHw;
            break;
        case (uint32_t)CrashAnalysisExecutionMarkerSource::System:
            break;
        default:
            // Should not reach here.
            assert(false);
        }
    }

    if (status == MarkerExecutionStatus::kInProgress)
    {
        for (size_t i = 0; i < node.child_markers.size(); ++i)
        {
            nlohmann::json child_marker_node_json;
            TreeNodeToJson(node.child_markers[i], child_marker_node_json, user_config);
            marker_node_json[kJsonElemEvents].push_back(child_marker_node_json);
        }
    }
}

MarkerExecutionStatus ExecMarkerTreeSerializer::GetItemStatus(const MarkerNode& node) const
{
    MarkerExecutionStatus ret = MarkerExecutionStatus::kNotStarted;
    auto iter = cmd_buffer_exec_status_.find(node.marker_value);
    assert(iter != cmd_buffer_exec_status_.end());
    if (iter != cmd_buffer_exec_status_.end())
    {
        ret = iter->second;
    }
    return ret;
}

#ifdef _DEBUG
void ExecMarkerTreeSerializer::UpdateAndValidateTimestamp(uint64_t timestamp)
{
    assert(timestamp >= last_timestamp_ || last_timestamp_ == 0);
    last_timestamp_ = timestamp;
}
#endif

std::string ExecMarkerTreeSerializer::GenerateSummaryString(std::vector<const MarkerNode*>& marker_stack, bool& is_atleast_one_child_in_progress) const
{

    std::stringstream txt;
    const MarkerNode* const node = marker_stack.back();
    assert(node != nullptr);
    if (node != nullptr)
    {
        if (GetItemStatus(*node) == MarkerExecutionStatus::kInProgress)
        {
            is_atleast_one_child_in_progress = true;
            bool is_atleast_one_child_of_current_in_progress = false;

            for (const auto& child_marker : node->child_markers)
            {
                marker_stack.push_back(&child_marker);
                txt << GenerateSummaryString(marker_stack, is_atleast_one_child_of_current_in_progress);
                marker_stack.pop_back();
            }

            if (!is_atleast_one_child_of_current_in_progress)
            {
                // When none of the child nodes are "in progress" or there are no child marker nodes, current node is the last "in progress" node in the tree path.
                // Print the path from root until the current node.

                // Do not include the consecutive repeating occurrences of identical nodes in text summary list.
                if (node->is_include_node_in_text_summary_list)
                {
                    for (size_t i = 0, count = marker_stack.size(); i + 1 < count; ++i)
                    {
                        txt << marker_stack[i]->marker_str << '/';
                    }
                    txt << marker_stack.back()->marker_str;

                    if (node->consecutive_identical_nodes_count > 0)
                    {
                        txt << " [" << node->consecutive_identical_nodes_count + 1 << " repeating occurrences]";
                    }
                    txt << std::endl;
                }
            }
        }
    }

    return txt.str();
}

std::string ExecMarkerTreeSerializer::TreeNodeToString(std::vector<bool> is_last_on_level, const MarkerNode& item, const Config& user_config) const
{
    // In the tree level, the maximum consecutive nodes of same status to print.
    // When a tree level has more consecutive nodes of same status than KMaxNodesOfSameStatusToPrint,
    // first and last (kMaxNodesOfSameStatusToPrint - kMinNumberOfNodesToSquash) / 2 nodes will be printed,
    // And in between nodes will be squashed, with count of nodes squashed printed.
    const size_t kMaxNodesOfSameStatusToPrint = 33;
    const size_t kMinNumberOfNodesToSquash = 9;

    std::stringstream txt;
    const size_t kIndentationDepth = is_last_on_level.size();

    // An internal dummy root node is created as a parent to these top level marker nodes.
    // kIndentationDepth = 1, represents indentation depth for the top level marker nodes (from marker_node_ list).
    // Dummy root node is not printed. So kIndentationDepth = 1 is ignored.
    if (kIndentationDepth > 1)
    {
        for (size_t i = 1; i + 1 < kIndentationDepth; ++i)
        {
            if (is_last_on_level[i])
            {
                txt << (is_file_visualization_ ? "   " : "    ");
            }
            else
            {
                // |
                txt << (is_file_visualization_ ? " \xe2\x94\x82 " : " |  ");
            }
        }

        if (is_last_on_level.back())
        {
            // '--
            txt << (is_file_visualization_ ? " \xe2\x94\x94\xe2\x94\x80" : " '--");
        }
        else
        {
            // |--
            txt << (is_file_visualization_ ? " \xe2\x94\x9c\xe2\x94\x80" : " |--");
        }
    }
    MarkerExecutionStatus status = MarkerExecutionStatus::kInProgress;

    // Do not print if this is an internally added "dummy Exec marker tree root node".
    if (item.marker_str != kExecTreeDummyRootString)
    {
        // Execution status.
        status = GetItemStatus(item);
        switch (status)
        {
        case MarkerExecutionStatus::kNotStarted:
            txt << "[ ] ";
            break;
        case MarkerExecutionStatus::kInProgress:
            txt << "[>] ";
            break;
        case MarkerExecutionStatus::kFinished:
            txt << "[X] ";
            break;
        default:
            assert(0);
        }

        // Generate the string.
        // Wrap the marker string in double quotes if source is application.
        if (((item.marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen)) == (uint32_t)CrashAnalysisExecutionMarkerSource::Application)
        {
            txt << "\"" << item.marker_str << "\"";
        }
        else
        {
            txt << item.marker_str;
        }

        if (user_config.is_marker_src)
        {
            switch ((item.marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen))
            {
            case (uint32_t)CrashAnalysisExecutionMarkerSource::Application:
                txt << " [" << kMarkerSrcApplication <<"]";
                break;
            case (uint32_t)CrashAnalysisExecutionMarkerSource::APILayer:
                txt << " [" << kMarkerSrcAPILayer << "]";
                break;
            case (uint32_t)CrashAnalysisExecutionMarkerSource::PAL:
                txt << " [" << kMarkerSrcPal << "]";
                break;
            case (uint32_t)CrashAnalysisExecutionMarkerSource::Hardware:
                txt << " [" << kMarkerSrcHw << "]";
                break;
            case (uint32_t)CrashAnalysisExecutionMarkerSource::System:
                break;
            default:
                // Should not reach here.
                assert(false);
            }
        }

        txt << std::endl;
    }
    // if 'expand_markers' is specified, expand all marker nodes.
    // For default output, only expand 'in progress' nodes.
    if (user_config.is_expand_markers || status == MarkerExecutionStatus::kInProgress)
    {
        const size_t sub_item_count = item.child_markers.size();
        if (sub_item_count > 0)
        {
            size_t total_nodes_with_same_status = 0;
            size_t first_skip_idx = 0;
            size_t last_skip_idx = 0;
            is_last_on_level.push_back(false);
            for (size_t i = 0; i + 1 < sub_item_count; ++i)
            {
                if (!user_config.is_expand_markers && item.child_markers[i].repeating_same_status_count != 0 && total_nodes_with_same_status == 0)
                {
                    // For default output, squash the repeated nodes with same status on the same level.
                    total_nodes_with_same_status = item.child_markers[i].repeating_same_status_count + 1;
                    const size_t kNodesToPrint = kMaxNodesOfSameStatusToPrint - kMinNumberOfNodesToSquash;
                    first_skip_idx = i + (kNodesToPrint / 2);
                    last_skip_idx = first_skip_idx + (total_nodes_with_same_status - kNodesToPrint);
                }
                else if (item.child_markers[i].repeating_same_status_count == 0)
                {
                    total_nodes_with_same_status = 0;
                }

                if (total_nodes_with_same_status == 0 || i < first_skip_idx || i > last_skip_idx)
                {
                    txt << TreeNodeToString(is_last_on_level, item.child_markers[i], user_config);
                }
                else if (i == first_skip_idx)
                {
                    // Print occurrence count information of the squashed nodes.
                    for (size_t k = 0; k < 3; ++k)
                    {
                        for (size_t j = 0; j < kIndentationDepth + 1; ++j)
                        {
                            // Print ellipse for squashed nodes.
                            // is_last_on_level[0] represents the dummy root node level.
                            // For j = 0; print ellipse only when squashing is done at top level.
                            if(item.marker_str == kExecTreeDummyRootString || j > 0)
                            {
                                if (is_last_on_level[j])
                                {
                                    txt << (is_file_visualization_ ? "   " : "    ");
                                }
                                else if(k == 1)
                                {
                                    // Print vertical ellipsis '⋮'.
                                    txt << (is_file_visualization_ ? " \xe2\x81\x9e " : " |  ");
                                    if (j == kIndentationDepth)
                                    {
                                        size_t occurrences = last_skip_idx - first_skip_idx + 1;
                                        txt << "(" << occurrences << " consecutive occurrences of "
                                            << MarkerExecStatusToString(GetItemStatus(item.child_markers[first_skip_idx])) << " nodes)";
                                    }
                                }
                                else
                                {
                                    // |
                                    txt << (is_file_visualization_ ? " \xe2\x94\x82 " : " |  ");
                                }
                            }
                        }
                        txt << std::endl;
                    }
                }
            }
            is_last_on_level.back() = true;
            txt << TreeNodeToString(is_last_on_level, item.child_markers.back(), user_config);
            is_last_on_level.pop_back();
        }
    }

    return txt.str();
}

void ExecMarkerTreeSerializer::UpdateSameStatusMarkerNodesCount()
{
    for (intmax_t idx = static_cast<intmax_t>(marker_nodes_.size()) - 1; idx > 0; idx--)
    {
        // Process nodes n - 1 to 1.
        MarkerNode& current_node = marker_nodes_[idx];
        MarkerNode& previous_node = marker_nodes_[idx - 1];

        const MarkerExecutionStatus current_node_status = GetItemStatus(current_node);
        const MarkerExecutionStatus previous_node_status = GetItemStatus(previous_node);

        if (!current_node.child_markers.empty() && current_node_status == MarkerExecutionStatus::kInProgress)
        {
            // Update same status marker nodes count for this node.
            UpdateSameStatusMarkerNodesCountForThisNode(marker_nodes_[idx]);
        }

        if ((current_node_status != MarkerExecutionStatus::kInProgress || current_node.child_markers.empty())
            && (previous_node_status != MarkerExecutionStatus::kInProgress || previous_node.child_markers.empty()))
        {
            if (current_node_status == previous_node_status)
            {
                previous_node.repeating_same_status_count = current_node.repeating_same_status_count + 1;
            }
        }

        // Repeating identical nodes needs to be collapsed in a single node appended with a repeat count for marker summary list in text output.
        // Build the count of consecutive identical nodes and mark the subsequent nodes to be excluded from the summary list text output.
        if (current_node_status == MarkerExecutionStatus::kInProgress && previous_node_status == MarkerExecutionStatus::kInProgress)
        {
            if (previous_node.marker_str == current_node.marker_str)
            {
                previous_node.consecutive_identical_nodes_count = current_node.consecutive_identical_nodes_count + 1;
                current_node.is_include_node_in_text_summary_list = false;
            }
        }
    }

    if (!marker_nodes_.empty() && !marker_nodes_[0].child_markers.empty() && GetItemStatus(marker_nodes_[0]) == MarkerExecutionStatus::kInProgress)
    {
        // Update same status marker nodes count for this node.
        UpdateSameStatusMarkerNodesCountForThisNode(marker_nodes_[0]);
    }

}

void ExecMarkerTreeSerializer::UpdateSameStatusMarkerNodesCountForThisNode(MarkerNode& current_node)
{
    for (intmax_t idx = static_cast<intmax_t>(current_node.child_markers.size()) - 1; idx > 0; idx--)
    {
        // Process nodes n - 1 to 1.
        MarkerNode& current_child_node = current_node.child_markers[idx];
        MarkerNode& previous_child_node = current_node.child_markers[idx - 1];

        const MarkerExecutionStatus current_child_node_status = GetItemStatus(current_child_node);
        const MarkerExecutionStatus previous_child_node_status = GetItemStatus(previous_child_node);

        if (!current_child_node.child_markers.empty() && current_child_node_status == MarkerExecutionStatus::kInProgress)
        {
            UpdateSameStatusMarkerNodesCountForThisNode(current_child_node);
        }

        if ((current_child_node_status != MarkerExecutionStatus::kInProgress || current_child_node.child_markers.empty())
            && (previous_child_node_status != MarkerExecutionStatus::kInProgress || previous_child_node.child_markers.empty()))
        {
            if (current_child_node_status == previous_child_node_status)
            {
                previous_child_node.repeating_same_status_count = current_child_node.repeating_same_status_count + 1;
            }
        }

        // Repeating identical nodes needs to be collapsed in a single node appended with a repeat count for marker summary list in text output.
        // Build the count of consecutive identical nodes and mark the subsequent nodes to be excluded from the summary list text output.
        if (current_child_node_status == MarkerExecutionStatus::kInProgress && previous_child_node_status == MarkerExecutionStatus::kInProgress)
        {
            if (previous_child_node.marker_str == current_child_node.marker_str)
            {
                previous_child_node.consecutive_identical_nodes_count = current_child_node.consecutive_identical_nodes_count + 1;
                current_child_node.is_include_node_in_text_summary_list = false;
            }
        }
    }

    if (!current_node.child_markers.empty() && !current_node.child_markers[0].child_markers.empty())
    {
        // Process node 0.
        UpdateSameStatusMarkerNodesCountForThisNode(current_node.child_markers[0]);
    }
}