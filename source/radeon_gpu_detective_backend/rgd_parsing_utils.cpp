//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  utilities for parsing raw data.
//=============================================================================
#include "rgd_parsing_utils.h"
#include "rgd_data_types.h"
// C++.
#include <cassert>
#include <string>
#include <sstream>
#include <algorithm>
#include <cinttypes>
#include <unordered_set>
#include <algorithm>

// RGD local.
#include "rgd_utils.h"

// Dev driver.
#include "dev_driver/include/rgdevents.h"
#include "rgd_data_types.h"

bool RgdParsingUtils::ParseCrashDataChunks(rdf::ChunkFile& chunk_file, const char* chunk_identifier, CrashData& umd_crash_data, CrashData& kmd_crash_data, std::string& error_msg)
{
    bool ret = true;
    std::stringstream error_txt;
    std::stringstream warning_txt;
    const int64_t kChunkCount = chunk_file.GetChunkCount(chunk_identifier);

    bool is_umd_chunk_found = false;
    bool is_kmd_chunk_found = false;

    // If unknown event id is received, log the warning message only once in case multiple such events are received and continue processing next events.
    bool is_unknown_event_id_reported = false;

    for (int64_t i = 0; i < kChunkCount; i++)
    {
        uint64_t header_size = chunk_file.GetChunkHeaderSize(chunk_identifier, (int)i);
        assert(header_size > 0);
        uint64_t payload_size = chunk_file.GetChunkDataSize(chunk_identifier, (int)i);
        assert(payload_size > 0);

        const uint32_t kProviderIdKmd = 0xE43C9C8E;
        const uint32_t kProviderIdUmd = 0x50434145;
        const uint32_t kProviderIdInvalid = 0xFFF;

        // Clock used for time tracking.
        uint64_t current_time = 0;

        // Provider ID: UMD or KMD in this case.
        uint32_t provider_id = kProviderIdInvalid;

        uint32_t timeUnit = 0;

        // Clock frequency.
        uint64_t frequency = 0;

        if (header_size > 0)
        {
            // Read the chunk header.
            DDEventProviderHeader chunk_header{};
            chunk_file.ReadChunkHeaderToBuffer(chunk_identifier, (int)i, &chunk_header);
            current_time = chunk_header.baseTimestamp;
            frequency = chunk_header.baseTimestampFrequency;
            timeUnit = chunk_header.timeUnit;

            // Validate provide id.
            assert(chunk_header.providerId == kProviderIdUmd || chunk_header.providerId == kProviderIdKmd);
            if (chunk_header.providerId == kProviderIdUmd || chunk_header.providerId == kProviderIdKmd)
            {
                provider_id = chunk_header.providerId;

                if (chunk_header.providerId == kProviderIdUmd)
                {
                    is_umd_chunk_found = true;
                }
                else if (chunk_header.providerId == kProviderIdKmd)
                {
                    is_kmd_chunk_found = true;
                }
            }
            else
            {
                error_txt << " (invalid chunk header - provider id)" << std::endl;
                ret = false;
            }
        }
        else
        {
            error_txt << " (invalid chunk header size)" << std::endl;
            ret = false;
        }

        if (payload_size > 0 && ret)
        {
            // Locate the structure we need to handle now (UMD or KMD) and set the data we already extracted from the chunk header.
            assert(provider_id == kProviderIdUmd || provider_id == kProviderIdKmd);
            CrashData& curr_crash_data = (provider_id == kProviderIdUmd ? umd_crash_data : kmd_crash_data);
            chunk_file.ReadChunkHeaderToBuffer(chunk_identifier, (int)i, &curr_crash_data.chunk_header);
            curr_crash_data.time_info.frequency = frequency;
            curr_crash_data.time_info.start_time = current_time;

            // Read the chunk payload.
            curr_crash_data.chunk_payload.reserve(payload_size);
            chunk_file.ReadChunkDataToBuffer(chunk_identifier, (int)i, curr_crash_data.chunk_payload.data());

            DDEventMetaVersion* meta_version = reinterpret_cast<DDEventMetaVersion*>(curr_crash_data.chunk_payload.data());

            // A major version change or a minor version change with major version zero is a breaking change in semantic versioning.
            if ((meta_version->major != DDEventMetaVersionMajor) || ((meta_version->major == 0) && (meta_version->minor != DDEventMetaVersionMinor)))
            {
                error_txt << " (Chunk provider header version: " << meta_version->major << "." << meta_version->minor << ")" << std::endl;
                break;
            }

            // Read the event stream.
            uint64_t bytes_read = 0;

            bytes_read += sizeof(DDEventMetaVersion);
            bytes_read += sizeof(DDEventProviderHeader);

            // If Crash debug marker value event is seen with Top and Bottom value as '0'(uninitialized),
            // discard the the events associated with this command buffer id.
            // Maintain a set of command buffer ids, for which all the related events needs to be discarded.
            std::unordered_set<uint32_t> discarded_cmd_buffer_ids_set;

            while (bytes_read < payload_size)
            {
                // Start by reading the event header to deduce its type.
                RgdEvent* event_header = reinterpret_cast<RgdEvent*>(curr_crash_data.chunk_payload.data() + bytes_read);

                if (event_header->header.eventId == DDCommonEventId::RgdEventTimestamp)
                {
                    TimestampEvent* curr_event = reinterpret_cast<TimestampEvent*>(curr_crash_data.chunk_payload.data() + bytes_read);
                    current_time += (curr_event->timestamp * timeUnit);
                    curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                    bytes_read += sizeof(TimestampEvent);
                }
                else if (provider_id == kProviderIdUmd)
                {
                    UmdEventId event_id = static_cast<UmdEventId>(event_header->header.eventId);
                    switch (event_id)
                    {
                    case UmdEventId::RgdEventExecutionMarkerBegin:
                    {
                        CrashAnalysisExecutionMarkerBegin* curr_event = reinterpret_cast<CrashAnalysisExecutionMarkerBegin*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        current_time += (curr_event->header.delta * (uint64_t)timeUnit);

                        // Ignore this event if the command buffer id is in discard set.
                        if (discarded_cmd_buffer_ids_set.find(curr_event->cmdBufferId) == discarded_cmd_buffer_ids_set.end())
                        {
                            curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                        }
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                    }
                    break;
                    case UmdEventId::RgdEventExecutionMarkerEnd:
                    {
                        CrashAnalysisExecutionMarkerEnd* curr_event = reinterpret_cast<CrashAnalysisExecutionMarkerEnd*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        current_time += (curr_event->header.delta * (uint64_t)timeUnit);

                        // Ignore this event if the command buffer id is in discard set.
                        if (discarded_cmd_buffer_ids_set.find(curr_event->cmdBufferId) == discarded_cmd_buffer_ids_set.end())
                        {
                            curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                        }
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                    }
                    break;
                    case UmdEventId::RgdEventCrashDebugNopData:
                    {
                        CrashDebugNopData* curr_event = reinterpret_cast<CrashDebugNopData*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        current_time += (curr_event->header.delta * (uint64_t)timeUnit);

                        if (curr_event->beginTimestampValue != UninitializedExecutionMarkerValue)
                        {
                            curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                        }
                        else
                        {
                            // This command buffer is scheduled but not seen by the GPU. Discard all the events for this command buffer id.
                            discarded_cmd_buffer_ids_set.insert(curr_event->cmdBufferId);
                        }
                        bytes_read += sizeof(CrashDebugNopData);
                    }
                    break;
                    case UmdEventId::RgdEventExecutionMarkerInfo:
                    {
                        CrashAnalysisExecutionMarkerInfo* curr_event = reinterpret_cast<CrashAnalysisExecutionMarkerInfo*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        current_time += (curr_event->header.delta * (uint64_t)timeUnit);

                        // Ignore this event if the command buffer id is in discard set.
                        if (discarded_cmd_buffer_ids_set.find(curr_event->cmdBufferId) == discarded_cmd_buffer_ids_set.end())
                        {
                            curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                        }
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                    }
                    break;
                    default:
                        // Notify in case there is an unknown event type.
                        if (!is_unknown_event_id_reported)
                        {
                            warning_txt << "UMD event ignored (unknown UmdEventId: " << (uint32_t)event_id << ").";
                            is_unknown_event_id_reported = true;
                        }

                        // When new events are added by the driver, older version of RGD might see it as an 'Unknown' event.
                        // The 'Unknown' event will be ignored and processing continues to parse next event.
                        RgdEvent* curr_event = reinterpret_cast<RgdEvent*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                        assert(false);
                    }
                }
                else if (provider_id == kProviderIdKmd)
                {
                    KmdEventId event_id = static_cast<KmdEventId>(event_header->header.eventId);
                    switch (event_id)
                    {
                    case KmdEventId::RgdEventVmPageFault:
                    {
                        VmPageFaultEvent* curr_event = reinterpret_cast<VmPageFaultEvent*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        current_time += (curr_event->header.delta * (uint64_t)timeUnit);
                        curr_crash_data.events.push_back(RgdEventOccurrence(curr_event, current_time));
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                    }
                    break;
                    default:
                        // Notify in case there is an unknown event type.
                        if (!is_unknown_event_id_reported)
                        {
                            warning_txt << "KMD event is ignored (unknown KmdEventId " << (uint32_t)event_id << ").";
                            is_unknown_event_id_reported = true;
                        }

                        // When new events are added by the driver, older version of RGD might see it as an 'Unknown' event.
                        // The 'Unknown' event will be ignored and processing continues to parse next event.
                        RgdEvent* curr_event = reinterpret_cast<RgdEvent*>(curr_crash_data.chunk_payload.data() + bytes_read);
                        bytes_read += sizeof(DDEventHeader) + curr_event->header.eventSize;
                        assert(false);
                    }
                }
            }

            // Something went wrong if we ever read beyond the payload_size.
            assert(bytes_read == payload_size);
            ret = (bytes_read == payload_size);
            if (!ret)
            {
                error_txt << " (parsing error - payload size)" << std::endl;
            }
        }
        else
        {
            if (ret)
            {
                error_txt << " (invalid chunk data size)" << std::endl;
            }
        }
    }

    if (!is_umd_chunk_found)
    {
        error_txt << " (execution marker information missing [UMD]";

        if (!is_kmd_chunk_found)
        {
            error_txt << " and page fault information missing [KMD]";
        }
        error_txt << ")" << std::endl;
    }

    if (!warning_txt.str().empty())
    {
        RgdUtils::PrintMessage(warning_txt.str().c_str(), RgdMessageType::kWarning, true);
    }

    error_msg = error_txt.str();  
    ret = error_msg.empty();
    return ret;
}

bool RgdParsingUtils::BuildCommandBufferMapping(const Config& user_config, const CrashData& umd_crash_data, std::unordered_map<uint64_t, std::vector<size_t>>& cmd_buffer_mapping)
{
    const uint32_t kApplicationMarkerValueOne = 1;
    bool ret = !umd_crash_data.events.empty();

    // Store command buffer "in-flight" status.
    std::unordered_map<uint32_t, bool> is_command_buffer_in_flight;

    for (uint32_t i = 0; i < umd_crash_data.events.size(); i++)
    {
        const RgdEventOccurrence& curr_event = umd_crash_data.events[i];
        uint32_t cmd_buffer_id = 0;

        assert(curr_event.rgd_event != nullptr);
        if (curr_event.rgd_event != nullptr)
        {
            // Extract the marker value and the command buffer ID of the current marker.
            if (static_cast<UmdEventId>(curr_event.rgd_event->header.eventId) == UmdEventId::RgdEventExecutionMarkerBegin)
            {
                const CrashAnalysisExecutionMarkerBegin& exec_marker_begin_event = static_cast<const CrashAnalysisExecutionMarkerBegin&>(*curr_event.rgd_event);
                cmd_buffer_id = exec_marker_begin_event.cmdBufferId;

                if (is_command_buffer_in_flight.find(cmd_buffer_id) != is_command_buffer_in_flight.end())
                {
                    if (is_command_buffer_in_flight[cmd_buffer_id])
                    {
                        // Add this marker to the relevant command buffer ID's vector.
                        cmd_buffer_mapping[cmd_buffer_id].push_back(i);
                    }
                }
                else if (exec_marker_begin_event.markerValue == kApplicationMarkerValueOne)
                {
                    // Execution marker top event found without associated crash debug event. Report the warning once.
                    std::stringstream msg;
                    msg << "Crash debug marker value event is missing for command buffer id: 0x" << std::hex << cmd_buffer_id << std::dec;
                    RgdUtils::PrintMessage(msg.str().c_str(), RgdMessageType::kWarning, true);
                }
            }
            else if (static_cast<UmdEventId>(curr_event.rgd_event->header.eventId) == UmdEventId::RgdEventExecutionMarkerEnd)
            {
                const CrashAnalysisExecutionMarkerEnd& exec_marker_end_event = static_cast<const CrashAnalysisExecutionMarkerEnd&>(*curr_event.rgd_event);
                cmd_buffer_id = exec_marker_end_event.cmdBufferId;

                if (is_command_buffer_in_flight.find(cmd_buffer_id) != is_command_buffer_in_flight.end())
                {
                    if (is_command_buffer_in_flight[cmd_buffer_id])
                    {
                        // Internal driver barrier markers should be filtered out.
                        // First check if an 'End' event is received for an internal barrier marker. If yes, remove the relevant 'Begin' event from the cmd_buffer_mapping.
                        bool is_internal_barrier = false;
                        size_t previous_event_idx = cmd_buffer_mapping[cmd_buffer_id].back();
                        const RgdEventOccurrence& previous_event_occurrence = umd_crash_data.events[previous_event_idx];
                        UmdEventId event_id = static_cast<UmdEventId>(previous_event_occurrence.rgd_event->header.eventId);

                        // Check if an event before the current event is 'ExecutionMarkerBegin' event and if it is an event for the Barrier marker.
                        if (event_id == UmdEventId::RgdEventExecutionMarkerBegin)
                        {
                            const CrashAnalysisExecutionMarkerBegin& marker_begin =
                                static_cast<const CrashAnalysisExecutionMarkerBegin&>(*previous_event_occurrence.rgd_event);
                            uint32_t previous_event_marker_value = marker_begin.markerValue;
                            std::string marker_string = (marker_begin.markerStringSize > 0)
                                ? std::string(reinterpret_cast<const char*>(marker_begin.markerName), marker_begin.markerStringSize)
                                : std::string(kStrNotAvailable);

                            // Is this a Barrier Begin marker? Barrier is considered "Internal" when there is no associated "CrashAnalysisExecutionMarkerInfo" event.
                            // External Barrier marker events are received as: Execution marker Begin (with one of the marker strings from kBarrierMarkerStrings) -> Barrier Begin Info -> Barrier End Info -> Execution marker End.
                            // Internal Barrier marker events are received as: Execution marker Begin -> Execution marker End. Notice, there is no "Info" events for interna Barrier marker events.
                            // If "--internal-barriers" option is used, then do not filter out internal barrier markers.
                            if (!user_config.is_include_internal_barriers
                                && kBarrierMarkerStrings.find(marker_string) != kBarrierMarkerStrings.end()
                                && previous_event_marker_value == exec_marker_end_event.markerValue)
                            {
                                // It is a Barrier Begin marker without intermediate "Info" event. So this is an internal barrier.
                                // Ignore internal barrier marker events. Remove the 'Begin' event for this internal barrier.
                                cmd_buffer_mapping[cmd_buffer_id].pop_back();
                                is_internal_barrier = true;
                            }
                        }

                        if (!is_internal_barrier)
                        {
                            // Add this marker to the relevant command buffer ID's vector.
                            cmd_buffer_mapping[cmd_buffer_id].push_back(i);
                        }
                    }
                }
            }
            else if (static_cast<UmdEventId>(curr_event.rgd_event->header.eventId) == UmdEventId::RgdEventExecutionMarkerInfo)
            {
                const CrashAnalysisExecutionMarkerInfo& exec_marker_info_event = static_cast<const CrashAnalysisExecutionMarkerInfo&>(*curr_event.rgd_event);
                cmd_buffer_id = exec_marker_info_event.cmdBufferId;

                if (is_command_buffer_in_flight.find(cmd_buffer_id) != is_command_buffer_in_flight.end())
                {
                    if (is_command_buffer_in_flight[cmd_buffer_id])
                    {
                        // Add this marker to the relevant command buffer ID's vector.
                        cmd_buffer_mapping[cmd_buffer_id].push_back(i);
                    }
                }
            }
            else if (static_cast<UmdEventId>(curr_event.rgd_event->header.eventId) == UmdEventId::RgdEventCrashDebugNopData)
            {
                const CrashDebugNopData& crash_debug_event = static_cast<const CrashDebugNopData&>(*curr_event.rgd_event);
                cmd_buffer_id = crash_debug_event.cmdBufferId;

                if (crash_debug_event.beginTimestampValue != InitialExecutionMarkerValue && crash_debug_event.beginTimestampValue != FinalExecutionMarkerValue)
                {
                    // Reset the command buffer to an initial state
                    cmd_buffer_mapping[cmd_buffer_id].clear();

                    is_command_buffer_in_flight[cmd_buffer_id] = true;
                }
                else
                {
                    is_command_buffer_in_flight[cmd_buffer_id] = false;
                }
            }
        }
    }

    // Sort the execution markers by timestamp.
    for (auto iter = cmd_buffer_mapping.begin(); iter != cmd_buffer_mapping.end(); iter++)
    {
        std::vector<size_t>& markers = iter->second;
        std::stable_sort(markers.begin(), markers.end(), [&](size_t a_index, size_t b_index)
            {
                const RgdEventOccurrence* a = &umd_crash_data.events[a_index];
                const RgdEventOccurrence* b = &umd_crash_data.events[b_index];
                return (a->event_time < b->event_time);
            });
    }

    return ret;
}

std::string RgdParsingUtils::UmdRgdEventIdToString(uint8_t event_id)
{
    std::stringstream ret;
    ret << (uint32_t)event_id << " (";
    switch (event_id)
    {
    case (uint8_t)DDCommonEventId::RgdEventTimestamp:
        ret << "TIMESTAMP";
        break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerBegin:
        ret << "EXEC MARKER BEGIN";
        break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerInfo:
        ret << "EXEC MARKER INFO";
        break;
    case (uint8_t)UmdEventId::RgdEventExecutionMarkerEnd:
        ret << "EXEC MARKER END";
        break;
    case (uint8_t)UmdEventId::RgdEventCrashDebugNopData:
        ret << "DEBUG NOP";
        break;
    default:
        // Shouldn't get here.
        assert(false);
        ret << kStrNotAvailable;
        break;
    }
    ret << ")";
    return ret.str();
}

std::string RgdParsingUtils::KmdRgdEventIdToString(uint8_t event_id)
{
    std::stringstream ret;
    ret << uint32_t(event_id) << " (";
    switch (event_id)
    {
    case (uint8_t)DDCommonEventId::RgdEventTimestamp:
        ret << "TIMESTAMP";
        break;
    case (uint8_t)KmdEventId::RgdEventVmPageFault:
        ret << "PAGE FAULT";
        break;
    default:
        // Shouldn't get here.
        assert(false);
        ret << kStrNotAvailable;
        break;
    }
    ret << ")";
    return ret.str();
}

std::string RgdParsingUtils::ExtractMarkerSource(uint32_t marker_value)
{
    std::string ret;
    uint8_t marker_source_val = static_cast<uint8_t>((marker_value & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen));

    // Marker source must be 4 bits
    assert((marker_source_val & 0xF0) == 0);

    CrashAnalysisExecutionMarkerSource marker_source = static_cast<CrashAnalysisExecutionMarkerSource>(marker_source_val);

    switch (marker_source)
    {
    case CrashAnalysisExecutionMarkerSource::Application:
        ret = "Application";
        break;
    case CrashAnalysisExecutionMarkerSource::APILayer:
        ret = "API layer";
        break;
    case CrashAnalysisExecutionMarkerSource::PAL:
        ret = "PAL";
        break;
    case CrashAnalysisExecutionMarkerSource::Hardware:
        ret = "Hardware";
        break;
    case CrashAnalysisExecutionMarkerSource::System:
        ret = "System";
        break;
    default:
        ret = kStrNotAvailable;
        break;
    }
    return ret;
}

std::string RgdParsingUtils::GetFormattedSizeString(uint64_t size_in_bytes, const char* unit)
{
    std::string ret;
    if (size_in_bytes > 0)
    {
        char s[40];
        memset(s, 0, 40);
        s[39] = '\0';
        if (size_in_bytes < 1024llu)
        {
            sprintf(s, "%" PRIu64 " %s", size_in_bytes, unit);
        }
        else if (size_in_bytes < 1024llu * 1024)
        {
            sprintf(s, "%" PRIu64 " (%.2f K%s)", size_in_bytes, size_in_bytes / 1024., unit);
        }
        else if (size_in_bytes < 1024llu * 1024 * 1024)
        {
            sprintf(s, "%" PRIu64 " (%.2f M%s)", size_in_bytes, size_in_bytes / (1024. * 1024.), unit);
        }
        else if (size_in_bytes < 1024llu * 1024 * 1024 * 1024)
        {
            sprintf(s, "%" PRIu64 " (%.2f G%s)", size_in_bytes, size_in_bytes / (1024. * 1024. * 1024.), unit);
        }
        else if (size_in_bytes < 1024llu * 1024 * 1024 * 1024 * 1024)
        {
            sprintf(s, "%" PRIu64 " (%.2f T%s)", size_in_bytes, size_in_bytes / (1024. * 1024. * 1024. * 1024.), unit);
        }
        ret = s;
    }
    else
    {
        ret = "0";
    }
    return ret;
}

bool RgdParsingUtils::ParseTraceProcessInfoChunk(rdf::ChunkFile& chunk_file, const char* chunk_identifier, TraceProcessInfo& process_info)
{
    bool              ret = true;
    const int64_t     kChunkCount = chunk_file.GetChunkCount(chunk_identifier);
    const int64_t     kChunkIdx = 0;
    const char*       kErrorMsg   = "failed to extract crashing process information";
    std::stringstream error_txt;

    // Parse if TraceProcessInfo chunk is present in the file. TraceProcessInfo will not be present for the files captured with RDP 3.0 and before.
    if (kChunkCount > 0)
    {
        const uint32_t kChunkVersion = chunk_file.GetChunkVersion(chunk_identifier);
        if (kChunkVersion <= kChunkMaxSupportedVersionTraceProcessInfo)
        {
            // Only one TraceProcessInfo chunk is expected so chunk index is set to 0 (first chunk).
            assert(kChunkCount == 1);
            uint64_t             payload_size = chunk_file.GetChunkDataSize(chunk_identifier, kChunkIdx);
            std::vector<uint8_t> payload_data(payload_size, 0);
            if (payload_size > 0)
            {
                // Read the TraceProcessInfo chunk payload data.
                chunk_file.ReadChunkDataToBuffer(chunk_identifier, kChunkIdx, payload_data.data());

                // TraceProcessInfo chunk format is defined in rdf/docs/internal/chunkFormats/traceProcessInfoChunkSpec.md.
                // struct TraceProcessInfo {
                //     uint32_t process_id;
                //     uint32_t process_path_size;  // Length of the path including the null terminator.
                //     char     process_path[1];    // byte-array representing the process path, null-terminated.
                // }

                std::copy(payload_data.begin(), payload_data.begin() + sizeof(process_info.process_id), reinterpret_cast<uint8_t*>(&process_info.process_id));
                uint32_t process_path_size = 0;
                std::copy(payload_data.begin() + sizeof(process_info.process_id),
                            payload_data.begin() + sizeof(process_info.process_id) + sizeof(process_path_size),
                            reinterpret_cast<uint8_t*>(&process_path_size));
                assert(process_path_size != 0);
                if (process_path_size != 0)
                {
                    // Process path is a null terminated char string. Copy the path string without the null char.
                    process_path_size -= 1;
                    std::copy(payload_data.begin() + sizeof(process_info.process_id) + sizeof(process_path_size),
                                payload_data.begin() + sizeof(process_info.process_id) + sizeof(process_path_size) + process_path_size,
                                std::back_inserter(process_info.process_path));
                }
                else
                {
                    error_txt << kErrorMsg << " (crashing process path information missing)";
                    RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
                }
            }
            else
            {
                error_txt << kErrorMsg << " (invalid chunk payload size [" << kChunkIdTraceProcessInfo << "])";
                RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
            }
        }
        else
        {
            error_txt << kErrorMsg << " (unsupported chunk version: " << kChunkVersion << " [" << kChunkIdTraceProcessInfo << "])";
            RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
        }
    }
    else
    {
        error_txt << kErrorMsg << " (crashing process information missing [" << kChunkIdTraceProcessInfo << "])";
        RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
    }

    ret = error_txt.str().empty();

    return ret;
}