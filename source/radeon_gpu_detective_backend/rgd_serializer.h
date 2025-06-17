//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer different data elements.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_H_

// C++.
#include <string>
#include <unordered_map>

// System Info.
#include "system_info_utils/source/system_info_reader.h"
#include "dev_driver/include/rgdevents.h"

// Local.
#include "rgd_data_types.h"

class RgdSerializer
{
public:
    // Serializes the given SystemInfo structure into a string.
    // Returns true if the file exists and false otherwise.
    static bool ToString(const Config& user_config, const system_info_utils::SystemInfo& system_info, const nlohmann::json& driver_experiments_json, std::string& system_info_txt);

    // Serialize the input parameters information into a string.
    static void InputInfoToString(const Config&                   user_config,
                                  const RgdCrashDumpContents&     contents,
                                  const std::vector<std::string>& debug_info_files,
                                  std::string&                    input_info_str);

    // Generates a string representing the RgdEvent's header for Umd event, with each line being pushed with the offset tabs.
    static std::string RgdEventHeaderToStringUmd(const RgdEvent& rgd_event, const std::string& offset_tabs);

    // Generates a string representing the RgdEvent's header for Kmd event, with each line being pushed with the offset tabs.
    static std::string RgdEventHeaderToStringKmd(const RgdEvent& rgd_event, const std::string& offset_tabs);

    // Serializes the given RGD event into a string with offset being
    static std::string EventTimestampToString(const TimestampEvent& timestamp_event, const std::string& offset_tabs);
    static std::string EventExecMarkerBeginToString(const CrashAnalysisExecutionMarkerBegin& exec_marker_event, const std::string& offset_tabs);
    static std::string EventExecMarkerInfoToString(const CrashAnalysisExecutionMarkerInfo& exec_marker_event, const std::string& offset_tabs);
    static std::string EventExecMarkerEndToString(const CrashAnalysisExecutionMarkerEnd& exec_marker_event, const std::string& offset_tabs);
    static std::string EventDebugNopToString(const CrashDebugNopData& debug_nop_event, const std::string& offset_tabs);
    static std::string EventVmPageFaultToString(const VmPageFaultEvent& page_fault_event, const std::string& offset_tabs);
    static std::string EventShaderWaveToString(const ShaderWaves& shader_wave_event, const std::string& offset_tabs);
    static std::string EventMmrRegisterDataToString(const MmrRegistersData& mmr_register_data_event, const std::string& offset_tabs);
    static std::string EventWaveRegisterDataToString(const WaveRegistersData& wave_register_data_event, const std::string& offset_tabs);
    static std::string EventSeInfoToString(const SeInfo& se_info_event, const std::string& offset_tabs);

    // Serialize RGD Umd events in the given container into a string.
    static std::string SerializeUmdCrashEvents(const std::vector<RgdEventOccurrence>& umd_events);

    // Serialize RGD Kmd events in the given container into a string.
    static std::string SerializeKmdCrashEvents(const std::vector<RgdEventOccurrence>& kmd_events);

    // Serialize the crash analysis time info structure into a string.
    static std::string CrashAnalysisTimeInfoToString(const CrashAnalysisTimeInfo& time_info);

    // Serialize the Code Object Load Events info.
    static std::string CodeObjectLoadEventsToString(const std::vector<RgdCodeObjectLoadEvent>& code_object_load_events);

    // Serialize the Code Object Database info.
    static std::string CodeObjectsToString(const std::map<Rgd128bitHash, CodeObject>& code_objects_map);

    // Serialize the PSO Correlations info.
    static std::string PsoCorrelationsToString(const std::vector<RgdPsoCorrelation>& pso_correlations);

private:
    RgdSerializer() = delete;
    ~RgdSerializer() = delete;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_SERIALIZER_H_
