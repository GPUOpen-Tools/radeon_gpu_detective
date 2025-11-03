//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  global data types.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_DATA_TYPES_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_DATA_TYPES_H_

// C++.
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// RDF.
#include "rdf/rdf/inc/amdrdf.h"

// Dev driver.
#pragma warning(push)
#pragma warning(disable : 4201)  // nonstandard extension used: nameless struct/union.
#include "dev_driver/include/rgdevents.h"
#pragma warning(pop)

// System info.
#pragma warning(push)
#pragma warning(disable : 4201)  // nonstandard extension used: nameless struct/union.
#include "system_info_utils/source/system_info_reader.h"
#pragma warning(pop)

// Local.
#include "rgd_asic_info.h"

// Marker value is in bits [0:27] while marker source is in bits [28:31].
static const uint32_t kMarkerValueMask = 0x0FFFFFFF;
static const uint32_t kMarkerSrcMask = 0xF0000000;
static const uint32_t kMarkerSrcBitLen = 4;
static const uint32_t kUint32Bits = 32;

// Reserved virtual address. Used when option 'all-resources' is specified.
static const uint64_t kVaReserved = 0x0;

// Special virtual address constants.
static const uint64_t kVaDeadBeef = 0xbeefde000000;

// Output json element string name for offending va information.
static const char* kJsonElemPageFaultSummary = "page_fault_summary";

// String not available.
static const char* kStrNotAvailable = "N/A";
static const char* kStrUnknown      = "Unknown";
static const char* kStrNone         = "None";

// Heap type strings.
static const char* kStrHeapTypeLocal = "Local (GPU memory, CPU-visible)";
static const char* kStrHeapTypeInvisible = "Invisible (GPU memory, invisible to CPU)";
static const char* kStrHeapTypeHost = "Host (CPU memory)";

// Driver Marker strings.
static const char* kStrDraw = "Draw";
static const char* kStrDispatch = "Dispatch";

// Marker strings for Barriers.
static const char* kBarrierStandard = "Barrier";
static const char* kBarrierRelease = "Release";
static const char* kBarrierAcquire = "Acquire";
static const char* kBarrierReleaseEvent = "ReleaseEvent";
static const char* kBarrierAcquireEvent = "AcquireEvent";
static const char* kBarrierReleaseThenAcquire = "ReleaseThenAcquire";
static const std::unordered_set<std::string> kBarrierMarkerStrings = { kBarrierStandard, kBarrierRelease, kBarrierAcquire, kBarrierReleaseEvent,kBarrierAcquireEvent, kBarrierReleaseThenAcquire};

static const char* kChunkIdTraceProcessInfo = "TraceProcessInfo";
static const uint32_t kChunkMaxSupportedVersionTraceProcessInfo = 1;

static const char* kChunkIdDriverOverrides = "DriverOverrides";

// DriverOverrides chunk version constants.
static const uint32_t kChunkMaxSupportedVersionDriverOverrides = 3;

static const char* kChunkIdCodeObject = "CodeObject";
static const uint32_t kChunkMaxSupportedVersionCodeObject = 2;

static const char* kChunkIdCOLoadEvent = "COLoadEvent";
static const uint32_t kChunkMaxSupportedVersionCOLoadEvent = 3;

static const char* kChunkIdPsoCorrelation = "PsoCorrelation";
static const uint32_t kChunkMaxSupportedVersionPsoCorrelation = 3;

static const char* kChunkIdRgdExtendedInfo = "RgdExtendedInfo";
static const uint32_t kChunkMaxSupportedVersionRgdExtendedInfo = 1;

// DriverOverrides chunk JSON element name constants.
static const char* kJsonElemComponentsDriverOverridesChunk          = "Components";
static const char* kJsonElemComponentDriverOverridesChunk           = "Component";
static const char* kJsonElemStructuresDriverOverridesChunk          = "Structures";
static const char* kJsonElemExperimentsDriverOverridesChunk         = "Experiments";
static const char* kJsonElemSettingNameDriverOverridesChunk         = "SettingName";
static const char* kJsonElemUserOverrideDriverOverridesChunk        = "UserOverride";
static const char* kJsonElemWasSupportedDriverOverridesChunk        = "Supported";
static const char* kJsonElemCurrentDriverOverridesChunk             = "Current";
static const char* kJsonElemIsDriverExperimentsDriverOverridesChunk = "IsDriverExperiments";
static const char* kErrorMsgInvalidDriverOverridesJson              = "invalid DriverOverrides JSON";
static const char* kErrorMsgFailedToParseDriverExperimentsInfo      = "failed to parse Driver Experiments info";

// RgdExtendedInfo chunk JSON element name constants.
static const char* kJsonElemHcaEnabled        = "hcaEnabled";
static const char* kJsonElemHcaFlags          = "hcaFlags";
static const char* kJsonElemCaptureWaveData   = "captureWaveData";
static const char* kJsonElemEnableSingleAluOp = "enableSingleAluOp";
static const char* kJsonElemEnableSingleMemOp = "enableSingleMemOp";
static const char* kJsonElemCaptureSgprVgprData = "captureSgprVgprData";
static const char* kJsonElemPdbSearchPaths    = "pdbSearchPaths";
static const char* kErrorMsgInvalidRgdExtendedInfoJson = "invalid RgdExtendedInfo JSON";

// RGD command line options constants.
static const char* kStrRawGprData = "raw-gpr-data";

// Enhanced Crash Info JSON element name constants.
static const char* kJsonElemShaders                     = "shaders";
static const char* kJsonElemShaderInfo                  = "shader_info";
static const char* kJsonElemShaderInfoId                = "shader_info_id";
static const char* kJsonElemShaderInfoIds               = "shader_info_ids";
static const char* kJsonElemSourceFileName              = "source_file_name";
static const char* kJsonElemEntryPointName              = "source_entry_point_name";
static const char* kJsonElemSourceCode                  = "high_level_source_code";
static const char* kJsonElemShaderIoAndResourceBindings = "shader_io_and_resource_bindings";
static const char* kJsonElemLinesHidden                 = "lines_hidden";
static const char* KJsonElemSourceLine                  = "source_line";
static const char* kJsonElemApiPsoHash                  = "api_pso_hash";
static const char* kJsonElemApiShaderHashHi             = "api_shader_hash_hi";
static const char* kJsonElemApiShaderHashLo             = "api_shader_hash_lo";
static const char* kJsonElemApiStage                    = "api_stage";
static const char* kJsonElemDisassembly                 = "disassembly";
static const char* kJsonElemInstructionOffset           = "instruction_offset";
static const char* kJsonElemInstr                       = "instr";
static const char* kJsonElemInstructionsDisassembly     = "instructions_disassembly";
static const char* kJsonElemWaveCount                   = "wave_count";
static const char* kJsonElemInstructionsHidden          = "instructions_hidden";

// ID prefixes
static const char* kStrPrefixShaderInfoId = "ShaderInfoID";
static const char* kStrPrefixCodeObjectId = "CodeObjectID";

// Execute nested command buffers string.
static const char* kStrExecuteNestedCmdBuffers = "ExecuteNestedCmdBuffers";

static const char* kStrEnabled = "Enabled";
static const char* kStrDisabled = "Disabled";

// Represents the execution status of an execution marker.
// A marker can be in a one of 3 states:
// 1. Hasn't started executing
// 2. In progress
// 3. Finished
enum class MarkerExecutionStatus
{
    kNotStarted,
    kInProgress,
    kFinished
};

// Configuration dictated by the user's command line arguments.
struct Config
{
    // Full path to the crash dump input file.
    std::string crash_dump_file;

    // Full path to the analysis output text file.
    std::string output_file_txt;

    // Full path to the analysis output json file.
    std::string output_file_json;

    // Full directory paths to the pdb files.
    std::vector<std::string> pdb_dir;

    // True for higher level of details for console output.
    bool is_verbose = false;

    // Serialize all the resources from the input crash dump file.
    bool is_all_resources = false;

    // Include timeline of the memory event for the virtual address.
    bool is_va_timeline = false;

    // Include raw Umd and Kmd crash data in the output.
    bool is_raw_event_data = false;

    // Include marker source information in execution marker tree visualization.
    bool is_marker_src = false;

    // Expand all marker nodes in the execution marker tree visualization.
    bool is_expand_markers = false;

    // Print raw timestamp in the text output.
    bool is_raw_time = false;

    // Output compact JSON.
    bool is_compact_json = false;

    // Print additional system information.
    bool is_extended_sysinfo = false;

    // Include implicit resources in summary output.
    bool is_include_implicit_resources = false;

    // Include internal barriers in Execution Marker Tree. Internal barriers are filtered out by default.
    bool is_include_internal_barriers = false;

    // Include full code object disassembly.
    bool is_all_disassembly = false;

    // Include full high level shader code if available.
    bool is_full_source = false;

    // Include more detailed information in text and JSON output.
    bool is_extended_output = false;
    // Save code object binaries from the crash dump file.
    bool is_save_code_object_binaries = false;

    // Include raw GPR data in the text output.
    bool is_raw_gpr_data = false;
};

// Stores time information about the crash analysis session.
struct CrashAnalysisTimeInfo
{
    // Base time: initial timestamp where analysis started.
    uint64_t start_time = 0;

    // Frequency of timestamps in chunk.
    uint64_t frequency = 0;
};

// An occurrence of an RGD event.
struct RgdEventOccurrence
{
    RgdEventOccurrence(RgdEvent* event, uint64_t event_timing) :
        rgd_event(event), event_time(event_timing){}

    // The event itself.
    RgdEvent* rgd_event = nullptr;

    // The time when the event happened.
    uint64_t event_time = 0;
};

// Stores the crash data which was read from the crash dump file's UmdCrashData chunk.
struct CrashData
{
    // The header of the first chunk that was used to encode the crash data.
    // This header can be used to retrieve the base time information for the crash data events.
    DDEventProviderHeader chunk_header;

    // Crash data events.
    // We store all events by value and access them during execution using their
    // indices in order to optimize runtime performance. Indices are used instead
    // of pointers, to avoid invalidated pointers in case that the underlying
    // std::vector storage is relocated by the runtime.
    std::vector<RgdEventOccurrence> events;

    // The payload where the raw event data is stored.
    std::vector<std::uint8_t> chunk_payload;

    // Time information about the crash analysis session.
    CrashAnalysisTimeInfo time_info;
};

// Holds the execution status of a marker: information on if the marker started and finished execution.
struct MarkerExecutionStatusFlags
{
    // True if the marker started execution.
    bool is_started = false;

    // True if the marker finished execution.
    bool is_finished = false;
};

// Holds the code object chunk info.
struct CodeObject
{
    RgdCodeObjectHeader chunk_header;

    // The payload where the code object binary blob data is stored.
    std::vector<std::uint8_t> chunk_payload;
};

struct RgdExtendedInfo
{
    std::vector<std::string> pdb_search_paths;
    bool                     is_hca_enabled{false};
    bool                     is_capture_wave_data{false};
    bool                     is_enable_single_alu_op{false};
    bool                     is_enable_single_memory_op{false};
    bool                     is_capture_sgpr_vgpr_data{false};
};

// Holds the parsed contents of a crash dump RDF file.
// For now RMT is not included as it is being parsed separately through a dedicated API.
struct RgdCrashDumpContents
{
    system_info_utils::SystemInfo system_info;
    ecitrace::GpuSeries           gpu_series{0};
    TraceChunkApiInfo             api_info;
    CrashData umd_crash_data;
    CrashData kmd_crash_data;
    TraceProcessInfo              crashing_app_process_info;
    // Mapping between command buffer ID and the indices for umd_crash_data.events array of its relevant execution marker events.
    std::unordered_map<uint64_t, std::vector<size_t>> cmd_buffer_mapping;

    // Driver Experiments JSON.
    nlohmann::json driver_experiments_json;

    // Rgd Extended Info.
    RgdExtendedInfo rgd_extended_info;

    // CodeObject Database.
    std::map<Rgd128bitHash, CodeObject> code_objects_map;

    // CodeObject Loader Events.
    std::vector<RgdCodeObjectLoadEvent> code_object_load_events;

    // PSO Correlations.
    std::vector<RgdPsoCorrelation> pso_correlations;
};

// Holds the information about the in-flight shader for correlation with the execution markers nodes.
struct RgdCrashingShaderInfo
{
    std::vector<std::string> crashing_shader_ids;
    std::vector<std::string> api_stages;
    std::vector<std::string> source_file_names;
    std::vector<std::string> source_entry_point_names;
};

struct WaveInfoRegisters
{
    uint32_t sq_wave_status{0};
    uint32_t sq_wave_pc_hi{0};
    uint32_t sq_wave_pc_lo{0};

    //sq_wave_trapsts is available only for RDNA2 and RDNA3.
    uint32_t sq_wave_trapsts{0};
    uint32_t sq_wave_ib_sts{0};
    uint32_t sq_wave_ib_sts2{0};
    uint32_t sq_wave_active{0};
    uint32_t sq_wave_exec_hi{0};
    uint32_t sq_wave_exec_lo{0};
    uint32_t sq_wave_hw_id1{0};
    uint32_t sq_wave_hw_id2{0};
    uint32_t sq_wave_valid_and_idle{0};

    // RDNA4 specific registers.
    uint32_t sq_wave_state_priv{0};
    uint32_t sq_wave_excp_flag_priv{0};
    uint32_t sq_wave_excp_flag_user{0};
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_DATA_TYPES_H_
