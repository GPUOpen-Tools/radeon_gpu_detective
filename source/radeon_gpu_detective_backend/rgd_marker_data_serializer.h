//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  execution marker serialization.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_MARKER_DATA_SERIALIZER_H_
#define RADEON_GPU_DETECTIVE_MARKER_DATA_SERIALIZER_H_

// Local.
#include "rgd_exec_marker_tree_serializer.h"

// Dev driver.
#include "dev_driver/include/rgdevents.h"

static const char* kJsonElemExecutionMarkerTree = "execution_marker_tree";
static const char* kJsonElemMarkersInProgress = "markers_in_progress";
static const char* kJsonElemExecutionMarkerStatusReport = "execution_marker_status_report";
static const char* kJsonElemCmdBufferIdElement = "cmd_buffer_id";

class ExecMarkerDataSerializer
{
public:
    ExecMarkerDataSerializer(std::unordered_map<uint64_t, RgdCrashingShaderInfo> in_flight_shader_api_pso_hashes_to_shader_info_)
        : in_flight_shader_api_pso_hashes_to_shader_info_(std::move(in_flight_shader_api_pso_hashes_to_shader_info_))
    {
    }
    ~ExecMarkerDataSerializer() = default;

    // Generate a textual tree that represents the status of the execution markers.
    // cmd_buffer_events maps between each command buffer ID and the indices of its execution marker events.
    // cmd_buffer_marker_status maps between each command buffer ID to a mapping between [marker_value --> execution status].
    bool GenerateExecutionMarkerTree(const Config& user_config, const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        std::string& marker_tree);

    // Generate a json representation of the status of the execution markers.
    // cmd_buffer_events maps between each command buffer ID and the indices of its execution marker events.
    // cmd_buffer_marker_status maps between each command buffer ID to a mapping between [marker_value --> execution status].
    bool GenerateExecutionMarkerTreeToJson(const Config& user_config, const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        nlohmann::json& all_cmd_buffers_marker_tree_json);

    // Generate a textual summary of the execution markers.
    bool GenerateExecutionMarkerSummaryList(const Config& user_config, const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        std::string& marker_summary_list_txt);

    // Generate a json representation of the summary of the execution markers.
    bool GenerateExecutionMarkerSummaryListJson(const Config& user_config, const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        nlohmann::json& all_cmd_buffers_marker_summary_json);

    // Returns true if any of the shaders that are associated with the pipeline that is represented by the given API PSO hash is in flight.
    bool IsShaderInFlight(uint64_t api_pso_hash);

private:

    // Build command buffer marker status map.
    bool BuildCmdBufferMarkerStatus(const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events);

    // Build execution marker tree nodes.
    bool BuildCmdBufferExecutionMarkerTreeNodes(const Config& user_config, const CrashData& umd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events);

    void UpdateMarkerTreeNodesForNestedCmdBuffer(const Config& user_config);

    // Map to store command buffer id -> marker value -> marker execution status.
    std::unordered_map <uint64_t, std::unordered_map<uint32_t, MarkerExecutionStatus>> command_buffer_marker_status_;

    // Map to store command buffer id -> ExecMarkerTreeSerializer mapping.
    std::unordered_map <uint64_t, std::unique_ptr<ExecMarkerTreeSerializer>> command_buffer_exec_tree_;

    std::vector<uint64_t> cmd_buffer_ids_create_ordered_;

    // Set to store nested command buffer ids.
    std::unordered_set<uint64_t> nested_cmd_buffer_ids_set;

    // Map to store command buffer info.
    std::unordered_map<uint64_t, CmdBufferInfo> cmd_buffer_info_map_;

    // For each in-flight shader, this is a mapping between the shader's pipeline's PSO hash to the pipeline's shader information.
    std::unordered_map<uint64_t, RgdCrashingShaderInfo> in_flight_shader_api_pso_hashes_to_shader_info_;
};

#endif // RADEON_GPU_DETECTIVE_MARKER_DATA_SERIALIZER_H_
