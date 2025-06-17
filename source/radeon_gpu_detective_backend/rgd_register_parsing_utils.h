//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers parsing utilities.
//============================================================================================

#ifndef RGD_REGISTER_PARSING_UTILS_H_
#define RGD_REGISTER_PARSING_UTILS_H_

// Local.
#include "rgd_data_types.h"

// Wave registers offsets for RDNA2, RDNA3 and STRIX1.
enum class WaveRegistersRdna2AndRdna3 : uint32_t
{
    kSqWaveStatus       = 0x0102,
    kSqWavePcHi         = 0x0109,
    kSqWavePcLo         = 0x0108,
    kSqWaveTrapsts      = 0x0103,
    kSqWaveIbSts        = 0x0107,
    kSqWaveIbSts2       = 0x011c,
    kSqWaveActive       = 0x000a,
    kSqWaveExecHi       = 0x027f,
    kSqWaveExecLo       = 0x027e,
    kSqWaveHwId1        = 0x0117,
    kSqWaveHwId2        = 0x0118,
    kSqWaveValidAndIdle = 0x000b
};

// Wave registers offsets for RDNA4.
enum class WaveRegistersRdna4 : uint32_t
{
    kSqWaveStatus       = 0x0102,
    kSqWaveStatePriv    = 0x0104,
    kSqWavePcHi         = 0x0141,
    kSqWavePcLo         = 0x0140,
    kSqWaveIbSts        = 0x0107,
    kSqWaveExcpFlagPriv = 0x0111,
    kSqWaveExcpFlagUser = 0x0112,   
    kSqWaveIbSts2       = 0x011c,
    kSqWaveActive       = 0x000a,
    kSqWaveExecHi       = 0x027f,
    kSqWaveExecLo       = 0x027e,
    kSqWaveHwId1        = 0x0117,
    kSqWaveHwId2        = 0x0118,
    kSqWaveValidAndIdle = 0x000b
};

class RegisterParsingUtils
{
public:
        /// @brief Parse wave info registers for Navi2x and Navi3x.
        /// @param [in]  kmd_crash_data The crash data.
        /// @param [out] wave_info_registers_map The wave info registers.
        /// @return True if the wave info registers are parsed successfully.
        static bool ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map);

private:
	RegisterParsingUtils() = delete;
	~RegisterParsingUtils() = delete;
};

#endif  // RGD_REGISTER_PARSING_UTILS_H_