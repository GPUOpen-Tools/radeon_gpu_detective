//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  execution marker tree serialization.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_EXEC_MARKER_TREE_SERIALIZER_H_
#define RADEON_GPU_DETECTIVE_EXEC_MARKER_TREE_SERIALIZER_H_

// C++.
#include <string>
#include <unordered_map>

// Dev driver.
#include "dev_driver/include/rgdevents.h"

// Local.
#include "rgd_data_types.h"

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// Node in the execution marker tree.
struct MarkerNode
{
    MarkerNode(uint64_t begin_timestamp, uint32_t value, uint64_t api_pso_hash, bool is_shader_in_flight, const std::string& str)
        : timestamp_begin_(begin_timestamp)
        , marker_str(str)
        , marker_value(value)
        , api_pso_hash(api_pso_hash)
        , is_shader_in_flight(is_shader_in_flight)
        , marker_info{}
    {
    }
    uint64_t timestamp_begin_ = 0;
    uint64_t timestamp_end_ = 0;

    // Pipeline bind API PSO hash.
    uint64_t api_pso_hash = 0;

    // User string.
    std::string marker_str;

    // Marker value.
    uint32_t marker_value = 0;

    // Execution status.
    MarkerExecutionStatus exec_status = MarkerExecutionStatus::kNotStarted;

    /// Used as a buffer to host additional structural data for DrawInfo, DispatchInfo, BarrierBeginInfo and BarrierEndInfo. It should start with ExecutionMarkerInfoHeader followed
    /// by a data structure that ExecutionMarkerInfoHeader.infoType dictates. All the structures are tightly packed
    /// (no paddings).
    uint8_t marker_info[MarkerInfoBufferSize] = { 0 };

    // Child markers (markers that started after this marker began and before it ended).
    std::vector<std::shared_ptr<MarkerNode>> child_markers;

    // One marker can have multiple 'Nested Command Buffers Info' events. Store each command buffer ID in a vector and marker_info will only store the most recent command buffer info event data.
    std::vector<uint32_t> nested_cmd_buffer_ids;

    // Counter to store no of consecutive nodes with same status on its level.
    uint64_t repeating_same_status_count = 0;

    // The count of consecutive identical nodes. Nodes are considered identical if they have the same status, same parent node and their marker string value is same.
    uint64_t consecutive_identical_nodes_count = 0;

    // For more than one consecutive identical nodes, Marker Summary List in text output includes path once for this node appended with repeat count.
    // This will be set to false for all identical occurrences of the nodes after the first node (nodes 2 to n) and the nodes 2 to n are excluded from the marker summary list in text output.
    bool is_include_node_in_text_summary_list = true;

    // Set to true if this marker is associated with a crashing shader.
    bool is_shader_in_flight                 = false;

    // Crashing shader information for this marker. Updated only when the marker is associated with a crashing shader.
    RgdCrashingShaderInfo crashing_shader_info;

    // Nested command buffer queue type.
    uint8_t nested_cmd_buffer_queue_type = 0;
};

// Builds a tree representation of the execution markers for a single command buffer.
class ExecMarkerTreeSerializer
{
public:
    ExecMarkerTreeSerializer(const Config& user_config, const std::unordered_map<uint32_t, MarkerExecutionStatus>& cmd_buffer_exec_buffer,
        uint64_t kmd_crash_value_begin, uint64_t kmd_crash_value_end) :
        cmd_buffer_exec_status_{ cmd_buffer_exec_buffer },
        kmd_crash_value_begin_{ kmd_crash_value_begin },
        kmd_crash_value_end_{ kmd_crash_value_end },
        is_file_visualization_(!user_config.output_file_txt.empty()) {}

    // Insert a Begin marker into the tree.
    void PushMarkerBegin(uint64_t              timestamp,
                         uint32_t              marker_value,
                         uint64_t              pipeline_api_pso_hash,
                         bool                  is_shader_in_flight,
                         const std::string&    str,
                         RgdCrashingShaderInfo& rgd_crashing_shader_info);

    // Update Begin marker info.
    void UpdateMarkerInfo(uint32_t marker_value, const uint8_t info[]);

    // Insert an End marker into the tree.
    void PushMarkerEnd(uint64_t timestamp, uint32_t marker_value);

    // Serialize the summary list into a string.
    std::string SummaryListToString() const;

    // Serialize the summary list into a json representation.
    void SummaryListToJson(nlohmann::json& summary_list_json) const;

    // Serialize the tree into a string.
    std::string TreeToString(const Config& user_config) const;

    // Serialize the tree into a json.
    void TreeToJson(const Config& user_config, nlohmann::json& marker_tree_json) const;

    // Build the look ahead counter of consecutive same status marker nodes.
    void UpdateSameStatusMarkerNodesCount();

    // Validate the 'begin' execution markers for matching 'end' marker and report the missing 'end' markers.
    void ValidateExecutionMarkers();

    // Update nested command buffer flag to true if the command buffer is a nested command buffer; false otherwise.
    void SetIsNestedCmdBuffer(bool is_nested_cmd_buffer);

    // Get the flag to check if the command buffer is a nested command buffer.
    bool IsNestedCmdBuffer() const;
    
    // Update the flag to true if the command buffer executes one or more nested command buffers; false otherwise.
    void SetIsExecutesNestedCmdBuffer(bool is_execute_nested_cmd_buffer);

    // Get the flag to check if the command buffer executes one or more nested command buffers.
    bool IsExecutesNestedCmdBuffer() const;

    // Get the list of nested command buffer IDs for this command buffer.
    std::vector<uint64_t> GetNestedCmdBufferIdsForExecTree() const;

    // Update the nested command buffer marker nodes map.
    bool UpdateNestedCmdBufferMarkerNodes(uint64_t cmd_buffer_id, ExecMarkerTreeSerializer& nested_cmd_buffer_tree, uint8_t nested_command_buffer_queue_type);

    // Return the status of the given marker node.
    MarkerExecutionStatus GetMarkerNodeStatus(const MarkerNode& node) const;

private:
    const uint64_t kmd_crash_value_begin_ = 0;
    const uint64_t kmd_crash_value_end_ = 0;
    std::vector<std::shared_ptr<MarkerNode>> marker_nodes_;
    std::vector<MarkerNode*> current_stack_;
    std::unordered_map<uint64_t, MarkerNode*> nested_cmd_buffer_nodes_map_;
    std::unordered_map<uint32_t, MarkerExecutionStatus> cmd_buffer_exec_status_;
    std::string missing_markers_info_txt_;

    // True if the text visualization is for a file - we use special characters that
    // render more nicely in files, but are not rendered nicely on Windows console.
    bool is_file_visualization_ = false;

    // Set to True, when the command buffer is a nested command buffer.
    bool is_nested_cmd_buffer_ = false;

    // Set to True, when this command buffer executes one or more nested command buffers.
    bool is_executes_nested_cmd_buffer_ = false;

#ifdef _DEBUG
    // Tracking the most recent timestamp for input validation.
    uint64_t last_timestamp_ = 0;
    void UpdateAndValidateTimestamp(uint64_t timestamp);
#endif

    // Return the status of the given marker node.
    MarkerExecutionStatus GetItemStatus(const MarkerNode& node) const;

    // Generates a string representation of the execution marker tree.
    std::string TreeNodeToString(std::vector<bool> is_last_on_level, const MarkerNode& node, const Config& user_config) const;

    // Generates a json object for the execution marker tree node.
    void TreeNodeToJson(const MarkerNode& node, nlohmann::json& marker_node_json, const Config& user_config) const;

    // Generates a summary for markers that are in progress.
    std::string GenerateSummaryString(std::vector<const MarkerNode*>& node_stack, bool& is_atleast_one_child_in_progress) const;

    // Generates a json representation of a summary for markers that are in progress.
    void GenerateSummaryJson(std::vector<const MarkerNode*>& node_stack,
        nlohmann::json& marker_summary_json, bool& is_atleast_one_child_in_progress) const;

    // Build the look ahead counter of consecutive same status child marker nodes.
    void UpdateSameStatusMarkerNodesCountForThisNode(MarkerNode& current_node);
};

#endif // RADEON_GPU_DETECTIVE_EXEC_MARKER_TREE_SERIALIZER_H_