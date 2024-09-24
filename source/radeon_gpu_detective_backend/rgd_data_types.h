//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// RDF.
#include "rdf/rdf/inc/amdrdf.h"

// Dev driver.
#include "dev_driver/include/rgdevents.h"

// System info.
#include "system_info_utils/source/system_info_reader.h"

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

// Holds the parsed contents of a crash dump RDF file.
// For now RMT is not included as it is being parsed separately through a dedicated API.
struct RgdCrashDumpContents
{
    system_info_utils::SystemInfo system_info;
    TraceChunkApiInfo             api_info;
    CrashData umd_crash_data;
    CrashData kmd_crash_data;
    TraceProcessInfo              crashing_app_process_info;
    // Mapping between command buffer ID and the indices for umd_crash_data.events array of its relevant execution marker events.
    std::unordered_map<uint64_t, std::vector<size_t>> cmd_buffer_mapping;

    // Driver Experiments JSON
    nlohmann::json driver_experiments_json;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_DATA_TYPES_H_
