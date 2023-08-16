//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Structures for crash dump parsing.
//=============================================================================

#pragma once
#pragma pack(push,1)

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

/// Unique id representing each event. Each variable name of the enum value corresponds to the
/// struct with the same name.
enum class UmdEventId : uint8_t
{
    RgdEventExecutionMarkerBegin = DDCommonEventId::FirstEventIdForIndividualProvider + 0,
    RgdEventExecutionMarkerEnd = DDCommonEventId::FirstEventIdForIndividualProvider + 1,
    RgdEventCrashDebugNopData = DDCommonEventId::FirstEventIdForIndividualProvider + 2
};

/// Unique id representing each event. Each variable name of the enum value corresponds to the
/// struct with the same name.
enum class KmdEventId : uint8_t
{
    RgdEventVmPageFault = DDCommonEventId::FirstEventIdForIndividualProvider,
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

/// Vm Pagefault Event
struct VmPageFaultEvent : RgdEvent
{
    uint32_t vmId;
    uint32_t processId;
    uint64_t faultVmAddress;
    uint16_t processNameLength;
    char     processName[64];
};

#pragma pack(pop)

/// RDF ChunkIds for the Crash Events:
constexpr char kUmdCrashChunkId[RDF_IDENTIFIER_SIZE] = "UmdCrashData";
constexpr char kKmdCrashChunkId[RDF_IDENTIFIER_SIZE] = "KmdCrashData";
