//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The interface class for parsing crash info registers data.
//============================================================================================

#ifndef RGD_CRASH_INFO_REGISTERS_PARSER_H_
#define RGD_CRASH_INFO_REGISTERS_PARSER_H_

#include "rgd_data_types.h"
#include "rgd_register_parsing_utils.h"

class ICrashInfoRegistersParser
{
public:
    /// @brief Destructor.
    virtual ~ICrashInfoRegistersParser() = default;
    virtual bool ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers) = 0;
};

#endif  // RGD_CRASH_INFO_REGISTERS_PARSER_H_