//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include <string_view>

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

static std::string GenerateBarrierString()
{
    // Surround "Barrier" strings with a line to clearly separate the markers before and after the "Barrier".
    const char* kBarrierSymbol = "----------";
    std::stringstream ret_txt;
    
    ret_txt << kBarrierSymbol;
    ret_txt << kBarrierStandard;
    ret_txt << kBarrierSymbol;

    return ret_txt.str();
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

void ExecMarkerTreeSerializer::PushMarkerBegin(uint64_t               timestamp,
                                               uint32_t               marker_value,
                                               uint64_t               pipeline_api_pso_hash,
                                               bool                   is_shader_in_flight,
                                               const std::string&     str,
                                               RgdCrashingShaderInfo& rgd_crashing_shader_info)
{
#ifdef _DEBUG
    UpdateAndValidateTimestamp(timestamp);
#endif

    std::shared_ptr<MarkerNode> new_marker_node = std::make_shared<MarkerNode>(timestamp, marker_value, pipeline_api_pso_hash, is_shader_in_flight, str);
    new_marker_node->exec_status                = GetItemStatus(*new_marker_node);
    if (is_shader_in_flight)
    {
        new_marker_node->crashing_shader_info = std::move(rgd_crashing_shader_info);
    }
    if (current_stack_.empty())
    {
        marker_nodes_.push_back(std::move(new_marker_node));
        current_stack_.push_back(marker_nodes_.back().get());
    }
    else
    {
        MarkerNode* curr_marker_node = current_stack_.back();
        curr_marker_node->child_markers.push_back(std::move(new_marker_node));
        current_stack_.push_back(curr_marker_node->child_markers.back().get());
    }
}

// Update marker info event for DrawInfo, DispatchInfo BarrierBeginInfo and BarrierEndInfo.
void ExecMarkerTreeSerializer::UpdateMarkerInfo(uint32_t marker_value, const uint8_t info[])
{
    bool is_invalid_info_marker = false;
    assert(!current_stack_.empty());
    if (!current_stack_.empty())
    {
        // Marker Info event for Draw, Dispatch and Barrier is always preceded by Marker Begin event for the same marker value.
        MarkerNode& node = *current_stack_.back();
        assert(node.marker_value == marker_value);
        if (node.marker_value == marker_value)
        {
            std::memcpy(node.marker_info, info, sizeof(node.marker_info));
        }
        else
        {
            is_invalid_info_marker = true;
        }

        uint8_t*                   marker_info             = const_cast<uint8_t*>(info);
        ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);
        if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::NestedCmdBuffer)
        {
            NestedCmdBufferInfo* nested_cmd_buffer_info = reinterpret_cast<NestedCmdBufferInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));

            // Check if the marker node points another nested cmd buffer.
            if (node.marker_str == kStrExecuteNestedCmdBuffers)
            {
                // One marker may have multiple 'Nested Command Buffers Info' events. Store each command buffer ID in a vector for current exec marker node.
                node.nested_cmd_buffer_ids.push_back(nested_cmd_buffer_info->nestedCmdBufferId);

                // Store the root marker node of the nested command buffer in a map with the nested command buffer ID as the key.
                assert(nested_cmd_buffer_nodes_map_.find(nested_cmd_buffer_info->nestedCmdBufferId) == nested_cmd_buffer_nodes_map_.end());
                if (nested_cmd_buffer_nodes_map_.find(nested_cmd_buffer_info->nestedCmdBufferId) == nested_cmd_buffer_nodes_map_.end())
                {
                    nested_cmd_buffer_nodes_map_[nested_cmd_buffer_info->nestedCmdBufferId] = &node;
                }
            }
            else
            {
                // Should not reach here.
                assert(false);
            }
        }
    }
    else
    {
        is_invalid_info_marker = true;
    }

    if (is_invalid_info_marker)
    {
        // There is no matching 'ExecutionMarkerBegin' for this 'ExecutionMarkerInfo' event.
        std::stringstream warning_msg;
        warning_msg << "detected an 'ExecutionMarkerInfo' event with no matching 'ExecutionMarkerBegin' event for the marker value=0x" << std::hex
                    << marker_value << std::dec << ".";
        RgdUtils::PrintMessage(warning_msg.str().c_str(), RgdMessageType::kWarning, true);
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
            if (GetMarkerNodeStatus(current_node) == MarkerExecutionStatus::kInProgress)
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

void ExecMarkerTreeSerializer::SetIsNestedCmdBuffer(bool is_nested_cmd_buffer)
{
    is_nested_cmd_buffer_ = is_nested_cmd_buffer;
}

bool ExecMarkerTreeSerializer::IsNestedCmdBuffer() const
{
    return is_nested_cmd_buffer_;
}

void ExecMarkerTreeSerializer::SetIsExecutesNestedCmdBuffer(bool is_execute_nested_cmd_buffer)
{
    is_executes_nested_cmd_buffer_ = is_execute_nested_cmd_buffer;
}

bool ExecMarkerTreeSerializer::IsExecutesNestedCmdBuffer() const
{
    return is_executes_nested_cmd_buffer_;
}

std::vector<uint64_t> ExecMarkerTreeSerializer::GetNestedCmdBufferIdsForExecTree() const
{
    std::vector<uint64_t> nested_cmd_buffer_ids;
    for (const std::pair<uint64_t, MarkerNode*>& nested_cmd_buffer_node_pair : nested_cmd_buffer_nodes_map_)
    {
        nested_cmd_buffer_ids.push_back(nested_cmd_buffer_node_pair.first);
    }

    return nested_cmd_buffer_ids;
}

bool ExecMarkerTreeSerializer::UpdateNestedCmdBufferMarkerNodes(uint64_t                  cmd_buffer_id,
                                                                ExecMarkerTreeSerializer& nested_cmd_buffer_tree,
                                                                uint8_t                   nested_command_buffer_queue_type)
{
    bool ret = false;

    auto iter = nested_cmd_buffer_nodes_map_.find(cmd_buffer_id);
    if (iter != nested_cmd_buffer_nodes_map_.end())
    {
        MarkerNode* parent_cmd_buffer_node = iter->second;
        assert(parent_cmd_buffer_node != nullptr);
        if (parent_cmd_buffer_node != nullptr)
        {
            // Move the root marker node from nested command buffer to the relevant marker node of the parent command buffer.
            for (std::shared_ptr<MarkerNode>& node_ptr : nested_cmd_buffer_tree.marker_nodes_)
            {
                parent_cmd_buffer_node->child_markers.push_back(node_ptr);
            }

            parent_cmd_buffer_node->nested_cmd_buffer_queue_type = nested_command_buffer_queue_type;
            ret                                                  = true;
        }
        else
        {
            // Should not reach here.
            assert(false);
        }
    }

    return ret;
}

MarkerExecutionStatus ExecMarkerTreeSerializer::GetMarkerNodeStatus(const MarkerNode& node) const
{
    return node.exec_status;
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
    for (const std::shared_ptr<MarkerNode>& marker_node : marker_nodes_)
    {
        bool is_atleast_one_child_in_progress_place_holdler = false;
        marker_stack.push_back(marker_node.get());
        txt << GenerateSummaryString(marker_stack, is_atleast_one_child_in_progress_place_holdler);
        marker_stack.pop_back();
    }
    return txt.str();
}

void ExecMarkerTreeSerializer::SummaryListToJson(nlohmann::json& summary_list_json) const
{
    std::vector<const MarkerNode*> marker_stack;

    summary_list_json[kJsonElemMarkers] = nlohmann::json::array();
    for (const std::shared_ptr<MarkerNode>& marker_node : marker_nodes_)
    {
        bool is_atleast_one_child_in_progress_place_holdler = false;
        marker_stack.push_back(marker_node.get());
        GenerateSummaryJson(marker_stack, summary_list_json, is_atleast_one_child_in_progress_place_holdler);
        marker_stack.pop_back();
    }
}

void ExecMarkerTreeSerializer::GenerateSummaryJson(std::vector<const MarkerNode*>& marker_stack,
    nlohmann::json& marker_summary_json, bool& is_atleast_one_child_in_progress) const
{
    const MarkerNode* node = marker_stack.back();
    assert(node != nullptr);
    if (node != nullptr)
    {
        if (GetMarkerNodeStatus(*node) == MarkerExecutionStatus::kInProgress)
        {
            is_atleast_one_child_in_progress = true;
            bool is_atleast_one_child_of_current_in_progress = false;

            for (const std::shared_ptr<MarkerNode>& child_marker : node->child_markers)
            {
                marker_stack.push_back(child_marker.get());
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
                if (kBarrierMarkerStrings.find(marker_stack.back()->marker_str) != kBarrierMarkerStrings.end())
                {
                    // Replace barrier marker strings with standard string for the barrier marker.
                    txt << kBarrierStandard;
                }
                else
                {
                    txt << marker_stack.back()->marker_str;
                }
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
    MarkerNode dummy_root(0, 0, 0, 0, kExecTreeDummyRootString);

    dummy_root.child_markers = marker_nodes_;

    txt << TreeNodeToString(is_last_on_level, dummy_root, user_config);
    return txt.str();
}

void ExecMarkerTreeSerializer::TreeToJson(const Config& user_config, nlohmann::json& marker_tree_json) const
{
    for (const std::shared_ptr<MarkerNode>& item : marker_nodes_)
    {
        nlohmann::json marker_node_json;
        TreeNodeToJson(*item, marker_node_json, user_config);
        marker_tree_json[kJsonElemEvents].push_back(marker_node_json);
    }
}

void ExecMarkerTreeSerializer::TreeNodeToJson(const MarkerNode& node, nlohmann::json& marker_node_json, const Config& user_config) const
{
    const char* kJsonElemMarkerExecStatus = "execution_status";
    const char* kJsonElemMarkerSrc        = "marker_source";
    const char* KJsonElemName             = "name";
    const char* kJsonElemIndexCount       = "index_count";
    const char* kJsonElemVertexCount      = "vertex_count";
    try
        {
        if (kBarrierMarkerStrings.find(node.marker_str) != kBarrierMarkerStrings.end())
        {
            marker_node_json[KJsonElemName] = kBarrierStandard;
        }
        else
        {
            marker_node_json[KJsonElemName] = node.marker_str;
        }

        // Flag for the barrier marker. Enhanced crash info - shader in flight correlation info is not printed for barrier markers.
        bool is_barrier_marker = false;

        if (kBarrierMarkerStrings.find(node.marker_str) != kBarrierMarkerStrings.end())
        {
            is_barrier_marker = true;
        }

        // Flag for the application marker. Enhanced crash info - shader in flight correlation info is not printed for application markers.
        // As pipeline bind events are correctly correlated with the driver markers only.
        bool is_application_marker =
            ((node.marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen)) == (uint32_t)CrashAnalysisExecutionMarkerSource::Application;

        uint8_t*                   marker_info             = const_cast<uint8_t*>(node.marker_info);
        ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);
        if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::Dispatch)
        {
            // Add thread dimensions from the dispatch info event.
            DispatchInfo* dispatch_info = reinterpret_cast<DispatchInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
            marker_node_json["thread_group_count"].push_back(
                {{"thread_x", dispatch_info->threadX}, {"thread_y", dispatch_info->threadY}, {"thread_z", dispatch_info->threadZ}});
        }
        else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::Draw)
        {
            // Add the index count and instance count.
            DrawInfo* draw_info = reinterpret_cast<DrawInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));

            // Vertex count field 'vtxIdxCount' is used for both storing vertex count when it's a non-indexed draw call and index count when it's an indexed draw call.
            if (draw_info->drawType == CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INDEXED_INSTANCED)
            {
                marker_node_json[kJsonElemIndexCount] = draw_info->vtxIdxCount;
            }
            else
            {
                marker_node_json[kJsonElemVertexCount] = draw_info->vtxIdxCount;
            }

            marker_node_json["instance_count"] = draw_info->instanceCount;
        }

        // Execution status.
        const MarkerExecutionStatus status = GetMarkerNodeStatus(node);
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

        // Update marker node json element with nested command buffer IDs and queue type if applicable.
        if (node.nested_cmd_buffer_ids.size() > 0)
        {
            static const char* kJsonElemNestedCmdBufferIds = "nested_command_buffer_ids";
            marker_node_json[kJsonElemNestedCmdBufferIds]  = nlohmann::json::array();

            for (size_t i = 0; i < node.nested_cmd_buffer_ids.size(); ++i)
            {
                marker_node_json[kJsonElemNestedCmdBufferIds].push_back(node.nested_cmd_buffer_ids[i]);
            }
            marker_node_json["nested_cmd_buffer_queue_type"] = RgdUtils::GetCmdBufferQueueTypeString((CmdBufferQueueType)node.nested_cmd_buffer_queue_type);
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

        // Add in-flight shader information.
        if (node.is_shader_in_flight && status == MarkerExecutionStatus::kInProgress && !is_barrier_marker && !is_application_marker)
        {
            marker_node_json["has_correlated_running_wave"]         = true;
            marker_node_json[kJsonElemShaderInfo][kJsonElemShaders] = nlohmann::json::array();

            const bool is_multiple_ids = node.crashing_shader_info.crashing_shader_ids.size() > 1;
            if (is_multiple_ids)
            {
                // When there are multiple shaders associated with a single marker node, print only the Shader Info Section IDs.
                marker_node_json[kJsonElemShaderInfo][kJsonElemShaders][kJsonElemShaderInfoIds] = nlohmann::json::array();
                for (size_t i = 0; i < node.crashing_shader_info.crashing_shader_ids.size(); ++i)
                {
                    marker_node_json[kJsonElemShaderInfo][kJsonElemShaders][kJsonElemShaderInfoIds].push_back(node.crashing_shader_info.crashing_shader_ids[i]);
                }
            }
            else
            {
                // When there is only one shader associated with a single marker node, print the Shader Info Section ID, API PSO Hash and API stage.
                marker_node_json[kJsonElemShaderInfo][kJsonElemApiPsoHash] = node.api_pso_hash;

                assert(node.crashing_shader_info.api_stages.size() == node.crashing_shader_info.crashing_shader_ids.size());
                const size_t min_of_shader_ids_stages = std::min(node.crashing_shader_info.api_stages.size(), node.crashing_shader_info.crashing_shader_ids.size());
                for (size_t i = 0; i < min_of_shader_ids_stages; ++i)
                {
                    marker_node_json[kJsonElemShaderInfo][kJsonElemShaders].push_back(
                        {{kJsonElemShaderInfoId, node.crashing_shader_info.crashing_shader_ids[i]}, {kJsonElemApiStage, node.crashing_shader_info.api_stages[i]}});
                }

                // Print source file name and entry point name if debug information is available.
                if (!node.crashing_shader_info.source_file_names.empty() && !node.crashing_shader_info.source_entry_point_names.empty() &&
                    node.crashing_shader_info.source_file_names.size() == 1 && node.crashing_shader_info.source_entry_point_names.size() == 1)
                {
                    marker_node_json[kJsonElemShaderInfo][kJsonElemShaders].back()[kJsonElemSourceFileName] =
                        (node.crashing_shader_info.source_file_names[0].empty() ? kStrNotAvailable : node.crashing_shader_info.source_file_names[0]);
                    marker_node_json[kJsonElemShaderInfo][kJsonElemShaders].back()[kJsonElemEntryPointName] =
                        (node.crashing_shader_info.source_entry_point_names.empty() ? kStrNotAvailable : node.crashing_shader_info.source_entry_point_names[0]);
                }
            }
        }

        if (status == MarkerExecutionStatus::kInProgress)
        {
            for (size_t i = 0; i < node.child_markers.size(); ++i)
            {
                nlohmann::json child_marker_node_json;
                TreeNodeToJson(*node.child_markers[i], child_marker_node_json, user_config);
                marker_node_json[kJsonElemEvents].push_back(child_marker_node_json);
            }
        }
    }
    catch (nlohmann::json::exception& e)
    {
        RgdUtils::PrintMessage(e.what(), RgdMessageType::kError, true);
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
        if (GetMarkerNodeStatus(*node) == MarkerExecutionStatus::kInProgress)
        {
            is_atleast_one_child_in_progress = true;
            bool is_atleast_one_child_of_current_in_progress = false;

            for (const std::shared_ptr<MarkerNode>& child_marker : node->child_markers)
            {
                marker_stack.push_back(child_marker.get());
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
                    if (kBarrierMarkerStrings.find(marker_stack.back()->marker_str) != kBarrierMarkerStrings.end())
                    {
                        // Replace barrier marker strings with standard string for the barrier marker.
                        txt << kBarrierStandard;
                    }
                    else
                    {
                        txt << marker_stack.back()->marker_str;
                    }

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

    const char* kMarkerNodeHasACorrelatedRunningWave = "<-- has a correlated running wave";
    const char* kShaderInfoSectionId                = "SHADER INFO section ID";

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

    // Flag for the barrier marker. Enhanced crash info - shader in flight correlation info is not printed for barrier markers.
    bool is_barrier_marker = false;

    if (kBarrierMarkerStrings.find(item.marker_str) != kBarrierMarkerStrings.end())
    {
        is_barrier_marker = true;
    }

    // Flag for the application marker. Enhanced crash info - shader in flight correlation info is not printed for application markers.
    // As pipeline bind events are correctly correlated with the driver markers only.
    bool is_application_marker =
        ((item.marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen)) == (uint32_t)CrashAnalysisExecutionMarkerSource::Application;

    MarkerExecutionStatus status = MarkerExecutionStatus::kInProgress;

    // Do not print if this is an internally added "dummy Exec marker tree root node".
    if (item.marker_str != kExecTreeDummyRootString)
    {
        // Execution status.
        status = GetMarkerNodeStatus(item);
        switch (status)
        {
        case MarkerExecutionStatus::kNotStarted:
            txt << "[ ] ";
            break;
        case MarkerExecutionStatus::kInProgress:
            if (item.is_shader_in_flight && !is_barrier_marker && !is_application_marker)
            {
                txt << "[#] ";
            }
            else
            {
                txt << "[>] ";
            }
            break;
        case MarkerExecutionStatus::kFinished:
            txt << "[X] ";
            break;
        default:
            assert(0);
        }

        // Generate the string.
        // Wrap the marker string in double quotes if source is application.
        if (is_application_marker)
        {
            txt << "\"" << item.marker_str << "\"";
        }
        else if (is_barrier_marker)
        {
            // Following marker strings from driver are replaced with standard string "Barrier" in Execution Marker Tree output.
            // 'Release', 'Acquire', 'ReleaseEvent', 'AcquireEvent' and 'ReleaseThenAcquire'.
            // Example - marker string 'ReleaseThenAcquire' is replace with '----------Barrier----------'.
            txt << GenerateBarrierString();
        }
        else
        {
            txt << item.marker_str;
        }

        uint8_t* marker_info = const_cast<uint8_t*>(item.marker_info);
        ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);
        if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::Dispatch)
        {
            // Print thread dimensions from the dispatch info event.
            DispatchInfo* dispatch_info = reinterpret_cast<DispatchInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));
            txt << "(ThreadGroupCount=[" << dispatch_info->threadX << "," << dispatch_info->threadY << "," << dispatch_info->threadZ << "])";
        }
        else if (exec_marker_info_header->infoType == ExecutionMarkerInfoType::Draw)
        {
            // Print the index count and instance count.
            const char* kIndexCountStr = "IndexCount";
            const char* kInstanceCountStr = "InstanceCount";
            const char* kVertexCountStr   = "VertexCount";
            std::string count_type_string = kVertexCountStr;

            DrawInfo* draw_info = reinterpret_cast<DrawInfo*>(marker_info + sizeof(ExecutionMarkerInfoHeader));

            // Vertex count field 'vtxIdxCount' is used for both storing vertex count when it's a non-indexed draw call and index count when it's an indexed draw call.
            if (draw_info->drawType == CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INDEXED_INSTANCED)
            {
                count_type_string = kIndexCountStr;
            }
                
            txt << "(" << count_type_string << "=" << draw_info->vtxIdxCount << ", " << kInstanceCountStr << "=" << draw_info->instanceCount << ")";
        }

        if (item.nested_cmd_buffer_ids.size() > 0)
        {
            const bool is_multiple_ids = item.nested_cmd_buffer_ids.size() > 1;
            txt << "(Nested Command Buffer IDs: {0x";
            
            for (size_t i = 0; i < item.nested_cmd_buffer_ids.size(); ++i)
            {
                txt << "0x" << std::hex << item.nested_cmd_buffer_ids[i] << std::dec;
                if (is_multiple_ids && i + 1 < item.nested_cmd_buffer_ids.size())
                {
                    txt << ", ";
                }
            }
            txt << "}, " << "(Queue type: " << RgdUtils::GetCmdBufferQueueTypeString((CmdBufferQueueType)item.nested_cmd_buffer_queue_type) << "))";
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

        if (item.is_shader_in_flight && status == MarkerExecutionStatus::kInProgress && !is_barrier_marker && !is_application_marker)
        {
            txt << " " << kMarkerNodeHasACorrelatedRunningWave << " <" << kShaderInfoSectionId;
            
            const bool is_multiple_ids = item.crashing_shader_info.crashing_shader_ids.size() > 1;
            if (is_multiple_ids)
            {
                // When there are multiple shaders associated with a single marker node, print only the Shader Info Section IDs. 
                txt << "s: {";
                for (size_t i = 0; i < item.crashing_shader_info.crashing_shader_ids.size(); ++i)
                {
                    txt << item.crashing_shader_info.crashing_shader_ids[i];
                    if (i + 1 < item.crashing_shader_info.crashing_shader_ids.size())
                    {
                        txt << ", ";
                    }
                }
                txt << "}";
            }
            else if (item.crashing_shader_info.crashing_shader_ids.size() == 1
                && item.crashing_shader_info.api_stages.size() == 1)
            {
                // When there is only one shader associated with a single marker node, print the Shader Info Section ID, API PSO Hash and API stage.
                // Print source file name and entry point name if debug information is available.
                txt << ": " << item.crashing_shader_info.crashing_shader_ids[0];  
                if (!item.crashing_shader_info.source_file_names.empty() && !item.crashing_shader_info.source_entry_point_names.empty() &&
                    item.crashing_shader_info.source_file_names.size() == 1 && item.crashing_shader_info.source_entry_point_names.size() == 1)
                {
                    txt << ", Entry point: " << item.crashing_shader_info.source_entry_point_names[0];
                    txt << ", Source file: " << item.crashing_shader_info.source_file_names[0];
                }
                txt << ", API stage: " << item.crashing_shader_info.api_stages[0];
                txt << ", API PSO hash: 0x" << std::hex << item.api_pso_hash << std::dec;
            }
            else
            {
                // Should not reach here.
                assert(false);
            }
            txt << ">";
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
                if (!user_config.is_expand_markers && item.child_markers[i]->repeating_same_status_count != 0 && total_nodes_with_same_status == 0)
                {
                    // For default output, squash the repeated nodes with same status on the same level.
                    total_nodes_with_same_status = item.child_markers[i]->repeating_same_status_count + 1;
                    const size_t kNodesToPrint = kMaxNodesOfSameStatusToPrint - kMinNumberOfNodesToSquash;
                    first_skip_idx = i + (kNodesToPrint / 2);
                    last_skip_idx = first_skip_idx + (total_nodes_with_same_status - kNodesToPrint);
                }
                else if (item.child_markers[i]->repeating_same_status_count == 0)
                {
                    total_nodes_with_same_status = 0;
                }

                if (total_nodes_with_same_status == 0 || i < first_skip_idx || i > last_skip_idx)
                {
                    txt << TreeNodeToString(is_last_on_level, *item.child_markers[i], user_config);
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
                                            << MarkerExecStatusToString(GetMarkerNodeStatus(*item.child_markers[first_skip_idx])) << " nodes)";
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
            txt << TreeNodeToString(is_last_on_level, *item.child_markers.back(), user_config);
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
        MarkerNode& current_node = *marker_nodes_[idx];
        MarkerNode& previous_node = *marker_nodes_[idx - 1];

        const MarkerExecutionStatus current_node_status = GetMarkerNodeStatus(current_node);
        const MarkerExecutionStatus previous_node_status = GetMarkerNodeStatus(previous_node);

        if (!current_node.child_markers.empty() && current_node_status == MarkerExecutionStatus::kInProgress)
        {
            // Update same status marker nodes count for this node.
            UpdateSameStatusMarkerNodesCountForThisNode(*marker_nodes_[idx]);
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

    if (!marker_nodes_.empty() && !marker_nodes_[0]->child_markers.empty() && GetMarkerNodeStatus(*marker_nodes_[0]) == MarkerExecutionStatus::kInProgress)
    {
        // Update same status marker nodes count for this node.
        UpdateSameStatusMarkerNodesCountForThisNode(*marker_nodes_[0]);
    }

}

void ExecMarkerTreeSerializer::UpdateSameStatusMarkerNodesCountForThisNode(MarkerNode& current_node)
{
    for (intmax_t idx = static_cast<intmax_t>(current_node.child_markers.size()) - 1; idx > 0; idx--)
    {
        // Process nodes n - 1 to 1.
        MarkerNode& current_child_node = *current_node.child_markers[idx];
        MarkerNode& previous_child_node = *current_node.child_markers[idx - 1];

        const MarkerExecutionStatus current_child_node_status = GetMarkerNodeStatus(current_child_node);
        const MarkerExecutionStatus previous_child_node_status = GetMarkerNodeStatus(previous_child_node);

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

    if (!current_node.child_markers.empty() && !current_node.child_markers[0]->child_markers.empty())
    {
        // Process node 0.
        UpdateSameStatusMarkerNodesCountForThisNode(*current_node.child_markers[0]);
    }
}