//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer for memory resource information.
//=============================================================================
#ifndef RGD_RESOURCE_INFO_H_
#define RGD_RESOURCE_INFO_H_

#include<memory>
#include "rgd_data_types.h"

// JSON.
#include "json/single_include/nlohmann/json.hpp"

class RgdResourceInfoSerializer
{
public:
    /// @brief Constructor.
    RgdResourceInfoSerializer();

    /// @brief Destructor.
    ~RgdResourceInfoSerializer();

    /// @brief Serialize resource information for the virtual address.
    ///
    /// When virtual address provided is zero, information for all the resources
    /// from the input crash dump file is serialized.
    ///
    /// @param [in]  user_config     The user configuration.
    /// @param [in]  virtual_address The GPU Address.
    /// @param [out] out_text        The resource information string.
    ///
    /// @return true if resource history is built successfully; false otherwise.
    bool GetVirtualAddressHistoryInfo(const Config& user_config, const uint64_t virtual_address, std::string& out_text);

    /// @brief Serialize resource information for the virtual address.
    ///
    /// When virtual address provided is zero, information for all the resources
    /// from the input crash dump file is serialized.
    ///
    /// @param [in]  user_config     The user configuration.
    /// @param [in]  virtual_address The GPU Address.
    /// @param [out] out_json        The resource information json object.
    ///
    /// @return true if resource history is built successfully; false otherwise.
    bool GetVirtualAddressHistoryInfo(const Config& user_config, const uint64_t virtual_address, nlohmann::json& out_json);

    /// @brief Initialize resource info serializer handle with the Rmt dataset handle for the input trace file.
    ///
    /// @param [in] trace_file_path The path of the crash dump file.
    ///
    /// @return true if Rmt dataset handle initialization is successful; false otherwise.
    bool InitializeWithTraceFile(const std::string& trace_file_path);

private:

    class pImplResourceInfoSerializer;
    std::unique_ptr<pImplResourceInfoSerializer> resource_info_serializer_impl_;

};

#endif  // RGD_RESOURCE_INFO_H_