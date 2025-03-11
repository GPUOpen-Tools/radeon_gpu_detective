//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Structures for crash dump parsing.
//=============================================================================

#pragma once
#pragma pack(push,1)

// RDF.
#include "rdf/rdf/inc/amdrdf.h"
#include "rgd_hash.h"

/// Generic event header for Rdf Chunks
struct DDEventProviderHeader
{
    /// Major version number that dictates the format of data in this file.
    uint16_t metaVersionMajor;

    /// Minor version number that dictates the format of data in this file.
    uint16_t metaVersionMinor;

    /// reserved
    uint32_t reserved;

    /// Number uniquely identifying an event provider.
    uint32_t providerId;

    /// Time unit indicates the precision of timestamp delta. A timestamp delta
    /// is always a multiple of `timeUnit`. To calculate timestamp:
    /// `currentTimestamp = lastTimestamp + delta * timeUnit`
    uint32_t timeUnit;

    /// First timestamp counter before any other events. Used to calibrate the
    /// timing of all subsequent events.
    uint64_t baseTimestamp;

    /// The frequency of counter, in counts per second. Used to convert
    /// counters to seconds.
    uint64_t baseTimestampFrequency;
};

constexpr uint32_t DDEventMetaVersionMajor = 0;
constexpr uint32_t DDEventMetaVersionMinor = 1;

struct DDEventMetaVersion
{
    uint16_t major;
    uint16_t minor;
};
static_assert(sizeof(DDEventMetaVersion) == 4, "DDEventMetaVersion has incorrect size");

/// Ids for events that are common for all event providers.
enum DDCommonEventId : uint8_t
{
    RgdEventTimestamp = 0,

    /// Individual provider's event id starts at this value.
    FirstEventIdForIndividualProvider = 16,
};

/// A marker that matches this value indicates the associated command buffer is scheduled but hasn't seen by the GPU.
constexpr uint32_t UninitializedExecutionMarkerValue = 0x0;

/// A marker that matches this value indicates the associated command buffer hasn't started.
constexpr uint32_t InitialExecutionMarkerValue = 0xFFFFAAAA;

/// A marker that matches this value indicates the associated command buffer has completed.
constexpr uint32_t FinalExecutionMarkerValue = 0xFFFFBBBB;

/// markerInfo size as defined in devdriver/apis/events/inc/dd_event/gpu_detective/umd_crash_analysis.h.
constexpr uint32_t MarkerInfoBufferSize = 64;

/// Unique id representing each event. Each variable name of the enum value corresponds to the
/// struct with the same name.
enum class UmdEventId : uint8_t
{
    RgdEventExecutionMarkerBegin = DDCommonEventId::FirstEventIdForIndividualProvider + 0,
    RgdEventExecutionMarkerEnd = DDCommonEventId::FirstEventIdForIndividualProvider + 1,
    RgdEventCrashDebugNopData = DDCommonEventId::FirstEventIdForIndividualProvider + 2,
    RgdEventExecutionMarkerInfo = DDCommonEventId::FirstEventIdForIndividualProvider + 3
};

enum class ExecutionMarkerInfoType : uint8_t
{
    // Invalid
    InvalidInfo = 0,

    // Indicate that the header precedes a CmdBufferInfo struct.
    CmdBufStart = 1,

    // Indicate that the header precedes a PipelineInfo struct.
    PipelineBind = 2,

    // Indicate that the header precedes a DrawInfo struct.
    Draw = 3,

    // Indicate that the header precedes a DrawUserData struct.
    DrawUserData = 4,

    // Indicate that the header precedes a DispatchInfo struct.
    Dispatch = 5,

    // Indicate that the header precedes a BarrierBeginInfo struct
    BarrierBegin = 6,

    // Indicate that the header precedes a BarrierEndInfo struct
    BarrierEnd = 7
};

/// Unique id representing each event. Each variable name of the enum value corresponds to the
/// struct with the same name.
enum class KmdEventId : uint8_t
{
    RgdEventVmPageFault   = DDCommonEventId::FirstEventIdForIndividualProvider,
    RgdEventShaderWaves   = DDCommonEventId::FirstEventIdForIndividualProvider + 1,
    RgdEventSeInfo        = DDCommonEventId::FirstEventIdForIndividualProvider + 2,
    RgdEventMmrRegisters  = DDCommonEventId::FirstEventIdForIndividualProvider + 3,
    RgdEventWaveRegisters = DDCommonEventId::FirstEventIdForIndividualProvider + 4,
};

/// Header used in RGD events
struct DDEventHeader
{
    /// Id for event type.
    uint8_t eventId;

    /// Time delta since the last timing calibration.
    uint8_t delta;

    /// The size of the actual event immediately following this header object,
    /// not including this header.
    uint16_t eventSize;

    DDEventHeader(uint8_t* buffer)
    {
        memcpy(&eventId, buffer, sizeof(eventId));
        buffer += sizeof(eventId);

        delta = 0;
        buffer += sizeof(delta);

        memcpy(&eventSize, buffer, sizeof(eventSize));
    }

    DDEventHeader()
    {
        eventId = 0;
        delta = 0;
        eventSize = 0;
    }
};

/// All RGD Events include the header and inherit from RgdEvent
struct RgdEvent
{
    DDEventHeader header;
};

/// Timestamp event
struct TimestampEvent : RgdEvent
{
    uint64_t timestamp;
};

/// The Marker Source enum
enum class CrashAnalysisExecutionMarkerSource : uint8_t
{
    Application  = 0,
    APILayer     = 1, // (i.e. DX12 or Vulkan)
    PAL          = 2,
    Hardware     = 3,
    // 4-14 are reserved
    System       = 15,
};

/// System Source Special Case Values

constexpr uint32_t kStartingTimestampValue = 0xAAA;
constexpr uint32_t kTimestampsCompleted    = 0xFFF;

/// Umd Markers
struct CrashAnalysisExecutionMarkerBegin : RgdEvent
{
    uint32_t cmdBufferId;
    uint32_t markerValue;
    uint16_t markerStringSize;
    uint8_t  markerName[512];
};

struct CrashAnalysisExecutionMarkerEnd : RgdEvent
{
    uint32_t cmdBufferId;
    uint32_t markerValue;
};

/// Crash Debug Nop Data
struct CrashDebugNopData : RgdEvent
{
    uint32_t cmdBufferId;
    uint32_t beginTimestampValue;
    uint32_t endTimestampValue;
};

/// Execution marker that provide additional information
//
//  The most typical use of the event is to describe an already existing ExecutionMarkerTop event.
//  Take 'Draw' as an example, here is what the tool can expect to see
//
//  => ExecutionMarkerTop({marker=0x10000003, makerName="Draw"} 
//  => ExecutionMarkerInfo({
//          marker=0x10000003,
//          markerInfo={ExecutionMarkerHeader({typeInfo=Draw}) + DrawInfo({drawType=...})
//  => ExecutionMarkerBottom({marker=0x10000003})
//
//  A couple of things to note
//  1. ExecutionMarkerInfo have the same markerValue as the ExecutionMarkerTop that it is describing.
//  2. ExecutionMarkerInfo is only used inside driver so ExecutionMarkerTop(Application)+ExecutionMarkerInfo
//     is not a possible combination. Currently, tool can expect to see back-to-back Top->Info->Bottom if Info
//     is available. However, this may not be true when we generate timestamps for all internal calls in the future.
//
//  There are situations where ExecutionMarkerTop and ExecutionMarkerInfo does not have 1-to-1 relations.
//  1. When using ExecutionMarkerInfo to provide additional info for a CmdBuffer, there will be timestamp but
//     no ExecutionMarkerTop/ExecutionMarkerBottom events. In this case, ExecutionMarkerInfo.marker is set to
//     0xFFFFAAAA (InitialExecutionMarkerValue).
//  2. There will be an ExecutionMarkerInfo for PipelineBind but not timestamp generated for that because binding
//     a pipeline does not cause any GPU work. Therefore no timestamp is needed.
//  3. Barrier operation will have one timestamp generated but 2 different ExecutionMarkerInfo generated (BarrierBegin
//     and BarrierEnd). Expect MarkerTop + MarkerInfo(BarrierBegin) + MarkerInfo(BarrierEnd) + MarkerBottom in this case.
// 
struct CrashAnalysisExecutionMarkerInfo : RgdEvent
{
    // Unique identifier of the relevant command buffer
    uint32_t cmdBufferId;

    /// Execution marker value (see comment in ExecutionMarkerTop). The ExecutionMarkerInfo generally describes an
    /// existing ExecutionMarkerTop and the marker is how ExecutionMarkerInfo relates to an ExecutionMarkerTop.
    uint32_t marker;

    /// The length of `markerInfo`.
    uint16_t markerInfoSize;

    /// Used as a buffer to host additonal structural data. It should starts with ExecutionMarkerInfoHeader followed
    /// by a data structure that ExecutionMarkerInfoHeader.infoType dictates. All the structure are tightly packed
    /// (no paddings).
    uint8_t markerInfo[MarkerInfoBufferSize];
};

/// Vm Pagefault Event
struct VmPageFaultEvent : RgdEvent
{
    uint32_t vmId;
    uint32_t processId;
    uint64_t faultVmAddress;
    uint16_t processNameLength;
    char     processName[64];
};

// offset and data of a single memory mapped register
struct MmrRegisterInfo
{
    uint32_t offset;
    uint32_t data;
};

// Note: Must exactly match KmdMmrRegistersEventData in KmdEventDefs.h
struct MmrRegistersData : RgdEvent
{
    uint32_t version;

    // GPU identifier for these register events
    uint32_t gpuId;

    // number of MMrRegisterInfo structures which follow
    uint32_t numRegisters;

    // array of MMrRegisterInfo
    // actual array length is `numRegisters`
    MmrRegisterInfo registerInfos[1];

    static size_t CalculateStructureSize(uint32_t numRegisterInfoForCalculation)
    {
        // std::max wrapped in parenthesis to ensure use of std::max instead of
        // Windows header 'max' macro
        numRegisterInfoForCalculation = (std::max)(1U, numRegisterInfoForCalculation);
        return sizeof(MmrRegistersData) + sizeof(MmrRegisterInfo) * (numRegisterInfoForCalculation - 1);
    }
};

// Graphics Register Bus Manager status registers
struct GrbmStatusSeRegs
{
    uint32_t version;
    uint32_t grbmStatusSe0;
    uint32_t grbmStatusSe1;
    uint32_t grbmStatusSe2;
    uint32_t grbmStatusSe3;
    // SE4 and SE5 are NV31 specific, 2x does not have this
    uint32_t grbmStatusSe4;
    uint32_t grbmStatusSe5;
};

// Note: Must exactly match KmdWaveInfo in KmdEventDefs.h
struct WaveInfo
{
    uint32_t version;

    union
    {
        struct
        {
            unsigned int waveId : 5;
            unsigned int : 3;
            unsigned int simdId : 2;
            unsigned int wgpId : 4;
            unsigned int : 2;
            unsigned int saId : 1;
            unsigned int : 1;
            unsigned int seId : 4;
            unsigned int reserved : 10;
        };
        uint32_t shaderId;
    };
};

// NOTE: HangType must match the Hangtype enum in kmdEventDefs.h
enum HangType : uint32_t
{
    pageFault    = 0,
    nonPageFault = 1,
    unknown      = 2,
};

// Note: Must exactly match KmdShaderWavesEventData in kmdEventDefs.h
struct ShaderWaves : RgdEvent
{
    // structure version
    uint32_t version;

    // GPU identifier for these register events
    uint32_t gpuId;

    HangType         typeOfHang;
    GrbmStatusSeRegs grbmStatusSeRegs;

    uint32_t numberOfHungWaves;
    uint32_t numberOfActiveWaves;

    // aray of hung waves followed by active waves
    // KmdWaveInfo * [numberOfHungWaves]
    // KmdWaveInfo * [numberOfActiveWaves]
    WaveInfo waveInfos[1];

    static size_t CalculateStructureSize(uint32_t numWaveInfoForCalculation)
    {
        // std::max wrapped in parenthesis to ensure use of std::max instead of
        // Windows header 'max' macro
        numWaveInfoForCalculation = (std::max)(1U, numWaveInfoForCalculation);
        return sizeof(ShaderWaves) + sizeof(WaveInfo) * (numWaveInfoForCalculation - 1);
    }
};

struct SeRegsInfo
{
    uint32_t version;
    uint32_t spiDebugBusy;
    uint32_t sqDebugStsGlobal;
    uint32_t sqDebugStsGlobal2;
};

struct SeInfo : RgdEvent
{
    // structure version
    uint32_t version;

    // GPU identifier for these register events
    uint32_t gpuId;

    // number of SeRegsInfo structures in seRegsInfos array
    uint32_t   numSe;
    SeRegsInfo seRegsInfos[1];

    static size_t CalculateStructureSize(uint32_t numSeRegsInfoForCalculation)
    {
        // std::max wrapped in parenthesis to ensure use of std::max instead of
        // Windows header 'max' macro
        numSeRegsInfoForCalculation = (std::max)(1U, numSeRegsInfoForCalculation);
        return sizeof(SeInfo) + sizeof(SeRegsInfo) * (numSeRegsInfoForCalculation - 1);
    }
};

// offset and data of a single shader wave register
struct WaveRegisterInfo
{
    uint32_t offset;
    uint32_t data;
};

// Note: Must exactly match KmdWaveRegistersEventData in KmdEventDefs.h
struct WaveRegistersData : RgdEvent
{
    uint32_t version;

    uint32_t shaderId;

    // number of WaveRegisterInfo structures which follow
    uint32_t numRegisters;

    // array of WaveRegisterInfo
    // actual array length is `numRegisters`
    WaveRegisterInfo registerInfos[1];
};

// Header information on how to interpret the info struct
struct ExecutionMarkerInfoHeader
{
    ExecutionMarkerInfoType infoType;
};

/// CmdBufferInfo follows header with ExecutionMarkerInfoType::CmdBufStart
struct CmdBufferInfo
{
    // Api-specific queue family index
    uint8_t     queue;

    // Device handle in D3D12 & Vulkan
    uint64_t    deviceId;

    // 0 in D3D12. VkQueueFlagBits in Vulkan
    uint32_t    queueFlags;
};

/// PipelineInfo follows header with ExecutionMarkerInfoType::PipelineBind
struct PipelineInfo
{
    // Pal::PipelineBindPoint
    uint32_t    bindPoint;

    // Api Pipeline hash
    uint64_t    apiPsoHash;
};

/// DrawUserData follows header with ExecutionMarkerInfoType::DrawUserData
struct DrawUserData
{
    // Vertex offset (first vertex) user data register index
    uint32_t     vertexOffset;

    // Instance offset (start instance) user data register index
    uint32_t     instanceOffset;

    // Draw ID SPI user data register index
    uint32_t     drawId;
};

/// DrawInfo follows header with ExecutionMarkerInfoType::Draw
struct DrawInfo
{
    uint32_t     drawType;
    uint32_t     vtxIdxCount;
    uint32_t     instanceCount;
    uint32_t     startIndex;
    DrawUserData userData;
};

/// DispatchInfo follows header with ExecutionMarkerInfoType::Dispatch
struct DispatchInfo
{
    // Api specific. RgpSqttMarkerApiType(DXCP) or RgpSqttMarkerEventType(Vulkan)
    uint32_t    dispatchType;

    // Number of thread groups in X dimension
    uint32_t    threadX;

    // Number of thread groups in Y dimension
    uint32_t    threadY;

    // Number of thread groups in Z dimension
    uint32_t    threadZ;
};

/// BarrierBeginInfo follows header with ExecutionMarkerInfoType::BarrierBegin
struct BarrierBeginInfo
{
    // Internal Barrier or external Barrier
    bool        isInternal;

    // Pal::Developer::BarrierType
    uint32_t    type;

    // Pal::Developer::BarrierReason
    uint32_t    reason;
};

/// BarrierEndInfo follows header with ExecutionMarkerInfoType::BarrierEnd
struct BarrierEndInfo
{
    // Pal::Developer::BarrierOperations.pipelineStalls
    uint16_t    pipelineStalls;

    // Pal::Developer::BarrierOperations.layoutTransitions
    uint16_t    layoutTransitions;

    // Pal::Developer::BarrierOperations.caches
    uint16_t    caches;
};

// CmdBufferQueueType declaration is equivalent to RgpSqttMarkerQueueType and should match the values as defined in dxcp\ddi\rgpSqttMarker.h.
// The following queue type represents the 'queue' in CmdBufferInfo struct.
enum CmdBufferQueueType : uint32_t
{
    CMD_BUFFER_QUEUE_TYPE_DIRECT  = 0x0,
    CMD_BUFFER_QUEUE_TYPE_COMPUTE = 0x1,
    CMD_BUFFER_QUEUE_TYPE_COPY    = 0x2,
};

// CrashAnalysisExecutionMarkerApiType represents the API type for driver markers.
// Following declaration is equivalent to RgpSqttMarkerApiType and should match the values as defined in dxcp\ddi\rgpSqttMarker.h.
enum CrashAnalysisExecutionMarkerApiType : uint32_t
{
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INSTANCED = 0x0,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INDEXED_INSTANCED = 0x1,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH = 0x2,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_RESOURCE = 0x3,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_TEXTURE_REGION = 0x4,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_BUFFER_REGION = 0x5,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_TILES = 0x6,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_ATOMIC_COPY_BUFFER_REGION = 0x7,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_CLEAR_DEPTH = 0x8,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_CLEAR_COLOR = 0x9,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_CLEAR_UAV_FLOAT = 0xa,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_CLEAR_UAV_UINT = 0xb,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_RESOLVE_SUBRESOURCE = 0xc,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_RESOLVE_SUBRESOURCE_REGION = 0xd,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISCARD_RESOURCE = 0xe,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_BARRIER = 0xf,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT = 0x10,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_RESOLVE_QUERY_DATA = 0x11,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_RAYS_INDIRECT = 0x12,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_RAYS_UNIFIED = 0x13,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT_RAYS_UNSPECIFIED = 0x14,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_INTERNAL_DISPATCH_BUILD_BVH = 0x15,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_RESERVED_0 = 0x16,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_MESH = 0x17,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_META_COMMAND = 0x18,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_INITIALIZE_META_COMMAND = 0x19,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_GRAPH = 0x1a,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_INIT_GRAPH_BACKING_STORE = 0x1b,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT_RAYS_INDIRECT = 0x1c,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT_RAYS_UNIFIED = 0x1d,
    CRASH_ANALYSIS_EXECUTION_MARKER_API_UNKNOWN = 0x7fff
};

// ApiInfo chunk data.
enum class TraceApiType : uint32_t
{
    GENERIC    = 0,
    DIRECTX_9  = 1,
    DIRECTX_11 = 2,
    DIRECTX_12 = 3,
    VULKAN     = 4,
    OPENGL     = 5,
    OPENCL     = 6,
    MANTLE     = 7,
    HIP        = 8,
    METAL      = 9
};

struct TraceChunkApiInfo
{
    TraceApiType apiType{0};
    uint16_t     apiVersionMajor = 0;  // Major client API version
    uint16_t     apiVersionMinor = 0;  // Minor client API version
};

// TraceProcessInfo chunk data.
struct TraceProcessInfo
{
    uint32_t process_id = 0;
    std::string process_path;
};

// CodeObject chunk header.
struct RgdCodeObjectHeader
{
    uint32_t      pci_id;            /// The ID of the GPU the trace was run on
    uint32_t      padding;
    Rgd128bitHash code_object_hash;  /// Hash of the code object binary
};

// CodeObject loader event chunk header.
struct RgdCodeObjectLoadEventHeader
{
    uint32_t count;  /// Number of Loader Events in this chunk
};

enum class RgdCodeObjectLoadEventType : uint32_t
{
    kCodeObjectLoadToGpuMemory     = 0,  ///< Code Object was loaded into GPU memory
    kCodeObjectUnloadFromGpuMemory = 1   ///< Code Object was unloaded from GPU memory
};

// Code object loader event RDF chunk data format.
struct RgdCodeObjectLoadEvent
{
    uint32_t                      pci_id;             ///< The ID of the GPU the trace was run on
    RgdCodeObjectLoadEventType    loader_event_type;  ///< Type of loader event
    uint64_t                      base_address;       ///< Base address where the code object was loaded
    Rgd128bitHash                 code_object_hash;   ///< Hash of the (un)loaded code object binary
    uint64_t                      timestamp;          ///< CPU timestamp of this event being triggered
};

/// API PSO correlation RDF chunk header format.
struct RgdPsoCorrelationHeader
{
    uint32_t count;  /// Number of PSO Correlations in this chunk
};

/// API PSO correlation RDF chunk data format.
struct RgdPsoCorrelation
{
    uint32_t      pci_id;                     ///< The ID of the GPU the trace was run on
    uint32_t      padding;
    uint64_t      api_pso_hash;               ///< Hash of the API-level Pipeline State Object
    Rgd128bitHash internal_pipeline_hash;     ///< Hash of all inputs to the pipeline compiler
    char          api_level_object_name[64];  ///< Debug object name (null-terminated)
};

#pragma pack(pop)

/// RDF ChunkIds for the Crash Events:
constexpr char kUmdCrashChunkId[RDF_IDENTIFIER_SIZE] = "UmdCrashData";
constexpr char kKmdCrashChunkId[RDF_IDENTIFIER_SIZE] = "KmdCrashData";
