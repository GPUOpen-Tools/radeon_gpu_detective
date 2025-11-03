//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "system_info_utils/source/system_info_reader.h"

// Local.
#include "rgd_data_types.h"
#include "rgd_resource_info_serializer.h"
#include "rgd_marker_data_serializer.h"
#include "rgd_enhanced_crash_info_serializer.h"

// JSON Schema version
#define STRINGIFY_JSON_SCHEMA_VERSION(major, minor) STRINGIFY_MACRO(major) "." STRINGIFY_MACRO(minor)
#define RGD_JSON_SCHEMA_VERSION_MAJOR 1
#define RGD_JSON_SCHEMA_VERSION_MINOR 3
#define RGD_JSON_SCHEMA_VERSION STRINGIFY_JSON_SCHEMA_VERSION(RGD_JSON_SCHEMA_VERSION_MAJOR, RGD_JSON_SCHEMA_VERSION_MINOR)

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - BEGIN ***

static const char* kStrInfoNoCmdBuffersInFlight = "no command buffers were in flight during crash.";
static const char* kStrNoInFlightShaderInfo     = "in flight shader info not available.";

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - END ***

class RgdSerializerJson
{
public:
    RgdSerializerJson() = default;
    ~RgdSerializerJson() = default;

    // Set input information.
    void SetInputInfo(const Config& user_config, const RgdCrashDumpContents& contents, const std::vector<std::string>& debug_info_files);

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

    // Set Driver Experiments info.
    void SetDriverExperimentsInfoData(const nlohmann::json& driver_experiments_json);

    // Set shader info.
    void SetShaderInfo(const Config& user_config, RgdEnhancedCrashInfoSerializer& enhanced_crash_info_serializer);

    // Set raw GPR (VGPR and SGPR) data.
    void SetGprData(const CrashData& kmd_crash_data);

    // Saves the JSON contents to a file.
    bool SaveToFile(const Config& user_config) const;

    // Clear JSON data to reduce destructor time.
    void Clear();

private:
    nlohmann::json json_;
    bool has_gpr_data_ = false;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_JSON_H_
