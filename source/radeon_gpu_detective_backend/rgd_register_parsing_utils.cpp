//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers parsing utilities.
//============================================================================================

#include "rgd_register_parsing_utils.h"

bool RegisterParsingUtils::ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map)
{
    // Parse wave info registers events for RDNA2 and RDNA3.
    bool ret = !kmd_crash_data.events.empty();
    wave_info_registers_map.clear();

    for (uint32_t i = 0; i < kmd_crash_data.events.size(); i++)
    {
        const RgdEventOccurrence& curr_event = kmd_crash_data.events[i];

        assert(curr_event.rgd_event != nullptr);
        if (curr_event.rgd_event != nullptr)
        {
            if (static_cast<KmdEventId>(curr_event.rgd_event->header.eventId) == KmdEventId::RgdEventWaveRegisters)
            {
                const WaveRegistersData* wave_registers = static_cast<const WaveRegistersData*>(curr_event.rgd_event);
                for (uint32_t j = 0; j < wave_registers->numRegisters; j++)
                {
                    switch ((WaveRegistersRdna2AndRdna3)wave_registers->registerInfos[j].offset)
                    {
                    case WaveRegistersRdna2AndRdna3::kSqWaveActive:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_active = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveExecHi:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_exec_hi = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveExecLo:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_exec_lo = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveHwId1:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_hw_id1 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveHwId2:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_hw_id2 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveIbSts:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_ib_sts = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveIbSts2:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_ib_sts2 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWavePcHi:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_pc_hi = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWavePcLo:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_pc_lo = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveStatus:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_status = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveTrapsts:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_trapsts = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna2AndRdna3::kSqWaveValidAndIdle:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_valid_and_idle = wave_registers->registerInfos[j].data;
                        break;
                    default:
                        // Should not reach here.
                        assert(false);
                    }
                }
            }
        }
    }

    if (ret)
    {
        ret = !wave_info_registers_map.empty();
    }

    return ret;
}