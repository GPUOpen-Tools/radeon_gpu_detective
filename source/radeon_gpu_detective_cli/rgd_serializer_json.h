//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer to JSON format.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_JSON_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_JSON_H_

// C++.
#include <string>

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// System Info.
#include "system_info_reader.h"

// Local.
#include "rgd_data_types.h"
#include "rgd_resource_info_serializer.h"
#include "rgd_marker_data_serializer.h"

// JSON Schema version
#define STRINGIFY_JSON_SCHEMA_VERSION(major, minor) STRINGIFY_MACRO(major) "." STRINGIFY_MACRO(minor)
#define RGD_JSON_SCHEMA_VERSION_MAJOR 1
#define RGD_JSON_SCHEMA_VERSION_MINOR 0
#define RGD_JSON_SCHEMA_VERSION STRINGIFY_JSON_SCHEMA_VERSION(RGD_JSON_SCHEMA_VERSION_MAJOR, RGD_JSON_SCHEMA_VERSION_MINOR)

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - BEGIN ***

static const char* kStrInfoNoCmdBuffersInFlight = "no command buffers were in flight during crash.";

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - END ***

class RgdSerializerJson
{
public:
    RgdSerializerJson() = default;
    ~RgdSerializerJson() = default;

    // Set input information.
    void SetInputInfo(const Config& user_config, const uint64_t crashing_process_id, const system_info_utils::SystemInfo& system_info);

    // Serializes and sets relevant system info.
    void SetSystemInfoData(const Config& user_config, const system_info_utils::SystemInfo& system_info);

    // Serializes and sets the execution marker events and other relevant UMD crash data.
    void SetUmdCrashData(const CrashData& umd_crash_data);

    // Serializes and sets relevant KMD events, such as page fault and debug nop.
    void SetKmdCrashData(const CrashData& kmd_crash_data);

    // Set virtual address memory residency timeline and relevent resource memory event history.
    void SetVaResourceData(RgdResourceInfoSerializer& resource_serializer,
        const Config& user_config,
        const uint64_t virtual_address);

    // Set command buffer execution markers status tree.
    void SetExecutionMarkerTree(const Config& user_config, const CrashData& kmd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        ExecMarkerDataSerializer& exec_marker_serializer);

    // Set command buffer executiona marker summary list.
    void SetExecutionMarkerSummaryList(const Config& user_config, const CrashData& kmd_crash_data,
        const std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_events,
        ExecMarkerDataSerializer& exec_marker_serializer);

    // Saves the JSON contents to a file.
    bool SaveToFile(const Config& user_config) const;

private:
    nlohmann::json json_;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_JSON_H_
