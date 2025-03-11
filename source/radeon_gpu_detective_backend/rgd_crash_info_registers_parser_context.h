//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers context class.
//============================================================================================

#ifndef RGD_CRASH_INFO_REGISTERS_PARSER_CONTEXT_H_
#define RGD_CRASH_INFO_REGISTERS_PARSER_CONTEXT_H_

#include "rgd_crash_info_registers_parser.h"
#include "rgd_data_types.h"
#include <memory>

class CrashInfoRegistersParserContext
{
public:
    explicit CrashInfoRegistersParserContext(std::unique_ptr<ICrashInfoRegistersParser> crash_info_registers_parser);

    bool ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map);
    
private:
    std::unique_ptr<ICrashInfoRegistersParser> crash_info_registers_parser_;
};

#endif  // RGD_CRASH_INFO_REGISTERS_PARSER_CONTEXT_H_