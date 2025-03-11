//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief serializer for enhanced crash information.
//============================================================================================

#ifndef RGD_ENHANCED_CRASH_INFO_SERIALIZER_H_
#define RGD_ENHANCED_CRASH_INFO_SERIALIZER_H_

// C++.
#include<memory>

// Local.
#include "rgd_data_types.h"

static constexpr uint64_t kAddressHighWordOne = 0x0000000100000000;

class RgdEnhancedCrashInfoSerializer
{
public:
    /// @brief Constructor.
    RgdEnhancedCrashInfoSerializer();

    /// @brief Destructor.
    ~RgdEnhancedCrashInfoSerializer();

    /// @brief Initialize the enhanced crash info serializer.
    bool Initialize(RgdCrashDumpContents& rgd_crash_dump_contents, bool is_page_fault);

    /// @brief Serialize the code object information and get in-flight shader info.
    bool GetInFlightShaderInfo(const Config& user_config, std::string& out_text);

    /// @brief Serialize the enhanced crash information in JSON format.
    bool GetInFlightShaderInfoJson(const Config& user_config, nlohmann::json& out_json);

    /// @brief  Get the set of API PSO hashes for the shaders that were in flight at the time of the crash.
    bool GetInFlightShaderApiPsoHashes(std::unordered_map<uint64_t, RgdCrashingShaderInfo>& in_flight_shader_api_pso_hashes_to_shader_info_);

    /// @brief Get the complete disassembly of the shaders that were in flight at the time of the crash.
    bool GetCompleteDisassembly(const Config& user_config, std::string& out_text);

private:
    class Impl;
    std::unique_ptr<Impl> enhanced_crash_info_serializer_impl_;
};

#endif  // RGD_ENHANCED_CRASH_INFO_SERIALIZER_H_