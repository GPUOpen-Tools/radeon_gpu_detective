//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers parser for STRIX1.
//============================================================================================

#ifndef RGD_CRASH_INFO_REGISTERS_PARSER_STRIX1_H_
#define RGD_CRASH_INFO_REGISTERS_PARSER_STRIX1_H_

#include "rgd_crash_info_registers_parser.h"

class CrashInfoRegistersParserStrix1 : public ICrashInfoRegistersParser
{
public:
        /// @brief Destructor.
        virtual ~CrashInfoRegistersParserStrix1() = default;

        /// @brief Parse wave info registers.
        /// @param [in]  kmd_crash_data The crash data.
        /// @param [out] wave_info_registers_map The wave info registers.
        /// @return True if the wave info registers are parsed successfully.
        bool ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map) override;
};

#endif  // RGD_CRASH_INFO_REGISTER_PARSER_STRIX1_H_