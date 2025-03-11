//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers parser for RDNA 2.
//============================================================================================

#include "rgd_crash_info_registers_parser_rdna2.h"
#include "rgd_register_parsing_utils.h"

bool CrashInfoRegistersParserRdna2::ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map)
{
    // Parse wave info registers events.
    bool ret = RegisterParsingUtils::ParseWaveInfoRegisters(kmd_crash_data, wave_info_registers_map);

    return ret;
}