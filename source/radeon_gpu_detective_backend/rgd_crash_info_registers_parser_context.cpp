//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers context class.
//============================================================================================

#include "rgd_crash_info_registers_parser_context.h"

CrashInfoRegistersParserContext::CrashInfoRegistersParserContext(std::unique_ptr<ICrashInfoRegistersParser> crash_info_registers_parser)
    : crash_info_registers_parser_(std::move(crash_info_registers_parser))
{}

bool CrashInfoRegistersParserContext::ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map)
{
    return crash_info_registers_parser_->ParseWaveInfoRegisters(kmd_crash_data, wave_info_registers_map);
}