//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The crash info registers parser for RDNA 4.
//============================================================================================

#include "rgd_crash_info_registers_parser_rdna4.h"
#include "rgd_register_parsing_utils.h"

bool CrashInfoRegistersParserRdna4::ParseWaveInfoRegisters(const CrashData& kmd_crash_data, std::unordered_map<uint32_t, WaveInfoRegisters>& wave_info_registers_map)
{
    // Parse wave info registers events for Navi4x.
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
                    switch ((WaveRegistersRdna4)wave_registers->registerInfos[j].offset)
                    {
                    case WaveRegistersRdna4::kSqWaveActive:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_active = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveExecHi:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_exec_hi = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveExecLo:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_exec_lo = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveHwId1:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_hw_id1 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveHwId2:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_hw_id2 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveIbSts:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_ib_sts = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveIbSts2:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_ib_sts2 = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWavePcHi:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_pc_hi = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWavePcLo:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_pc_lo = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveStatus:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_status = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveValidAndIdle:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_valid_and_idle = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveStatePriv:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_state_priv = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveExcpFlagPriv:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_excp_flag_priv = wave_registers->registerInfos[j].data;
                        break;
                    case WaveRegistersRdna4::kSqWaveExcpFlagUser:
                        wave_info_registers_map[wave_registers->shaderId].sq_wave_excp_flag_user = wave_registers->registerInfos[j].data;
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