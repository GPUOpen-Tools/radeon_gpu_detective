//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  test rgd file structure.
//=============================================================================

#include <iostream>
#include <sstream>

#include "test_rgd_file.h"

// RGD CLI.
#include "rgd_parsing_utils.h"
#include "rgd_data_types.h"
#include "rgd_utils.h"

// RDF.
#include "rdf/rdf/inc/amdrdf.h"

// System info.
#include "system_info_utils/source/system_info_reader.h"

TestRgdFile::TestRgdFile(const std::string& path) : file_path_(path) {}

bool TestRgdFile::ParseRgdFile(bool is_page_fault)
{
    bool ret = false;

    const char* kErrorTextCouldNotParseRgdFile = "could not parse the crash dump file";

    // Read and parse the RDF file.
    auto         file = rdf::Stream::OpenFile(file_path_.c_str());

    std::string error_msg;
    try
    {
        rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

        // Parse the UMD and KMD crash data chunk.
        const char* kChunkCrashData = "DDEvent";
        ret = RgdParsingUtils::ParseCrashDataChunks(chunk_file, kChunkCrashData, rgd_file_contents_.umd_crash_data, rgd_file_contents_.kmd_crash_data, error_msg);

        if (!error_msg.empty())
        {
            std::stringstream error_txt;
            error_txt << kErrorTextCouldNotParseRgdFile << error_msg;
            RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
        }

        // Check if KMD chunk data is found. KMD chunk is optional and it is received only for the page fault crashes.
        is_kmd_chunk_found_ = !rgd_file_contents_.kmd_crash_data.events.empty();

        if (is_page_fault && !is_kmd_chunk_found_)
        {
            ret = false;
            RgdUtils::PrintMessage("KMD chunk data is not found in the crash dump file.", RgdMessageType::kError, true);
        }

        // Parse System Info chunk.
        is_system_info_parsed_ = system_info_utils::SystemInfoReader::Parse(chunk_file, rgd_file_contents_.system_info);

        if (!is_system_info_parsed_)
        {
            RgdUtils::PrintMessage("failed to parse system information contents in crash dump file.", RgdMessageType::kError, true);
        }
    }
    catch (const std::exception& e)
    {
        is_rdf_parsing_error_ = true;
        std::stringstream error_txt;
        error_txt << kErrorTextCouldNotParseRgdFile << " (" << e.what() << ")";
        RgdUtils::PrintMessage(error_txt.str().c_str(), RgdMessageType::kError, true);
    }

    return ret;
}

bool TestRgdFile::IsSystemInfoParsed()
{
    return is_system_info_parsed_;
}

bool TestRgdFile::IsRdfParsingError()
{
    return is_rdf_parsing_error_;
}

bool TestRgdFile::IsAppMarkersFound()
{
    bool is_app_marker_found = false;
    for (uint32_t i = 0; i < rgd_file_contents_.umd_crash_data.events.size(); i++)
    {
        const RgdEventOccurrence& curr_marker_event = rgd_file_contents_.umd_crash_data.events[i];
        UmdEventId marker_event_id = static_cast<UmdEventId>(curr_marker_event.rgd_event->header.eventId);
        if (marker_event_id == UmdEventId::RgdEventExecutionMarkerBegin)
        {
            const CrashAnalysisExecutionMarkerBegin& marker_begin =
                static_cast<const CrashAnalysisExecutionMarkerBegin&>(*curr_marker_event.rgd_event);

            if ((marker_begin.markerValue & kMarkerSrcMask) >> (kUint32Bits - kMarkerSrcBitLen) == (uint32_t)CrashAnalysisExecutionMarkerSource::Application)
            {
                is_app_marker_found = true;

                // Break if atleast one 'APP' marker is found.
                break;
            }
        }
    }

    if (!is_app_marker_found)
    {
        RgdUtils::PrintMessage("no application markers found in the crash dump file.", RgdMessageType::kError, true);
    }

    return is_app_marker_found;
}

bool TestRgdFile::IsMarkerContextFound()
{
    bool is_marker_context_found     = false;
    bool is_cmd_buf_start_info_found = false;

    // Test for Draw info event when previous marker is Draw Begin.
    bool is_previous_marker_draw_begin = false;
    bool is_draw_info_found            = true;

    // Test for Dispatch info event when previous marker is Dispatch Begin.
    bool is_previous_marker_dispatch_begin = false;
    bool is_dispatch_info_found            = true;

    bool is_barrier_found                  = false;
    bool is_pipeline_bind_found            = false;

    // Iterate through all UMD events and check if atleast one of each 'CmdBufStart', 'Pipeline Bind', 'Draw', 'Dispatch' and 'Barrier' info event is found.
    for (uint32_t i = 0; i < rgd_file_contents_.umd_crash_data.events.size(); i++)
    {
        const RgdEventOccurrence& curr_marker_event = rgd_file_contents_.umd_crash_data.events[i];
        UmdEventId                marker_event_id   = static_cast<UmdEventId>(curr_marker_event.rgd_event->header.eventId);
        if (marker_event_id == UmdEventId::RgdEventExecutionMarkerBegin)
        {
            const CrashAnalysisExecutionMarkerBegin& marker_begin = static_cast<const CrashAnalysisExecutionMarkerBegin&>(*curr_marker_event.rgd_event);
            std::string                              marker_name  = (marker_begin.markerStringSize > 0)
                                                                        ? std::string(reinterpret_cast<const char*>(marker_begin.markerName), marker_begin.markerStringSize)
                                                                        : std::string(kStrNotAvailable);
            if (marker_name == kStrDraw)
            {
                is_previous_marker_draw_begin = true;
            }
            else if (marker_name == kStrDispatch)
            {
                is_previous_marker_dispatch_begin = true;
            }
        }
        if (marker_event_id == UmdEventId::RgdEventExecutionMarkerInfo)
        {
            const CrashAnalysisExecutionMarkerInfo& exec_marker_info_event = static_cast<const CrashAnalysisExecutionMarkerInfo&>(*curr_marker_event.rgd_event);
            uint32_t                                marker_value           = exec_marker_info_event.marker;

            uint8_t*                   marker_info             = const_cast<uint8_t*>(exec_marker_info_event.markerInfo);
            ExecutionMarkerInfoHeader* exec_marker_info_header = reinterpret_cast<ExecutionMarkerInfoHeader*>(marker_info);

            switch (exec_marker_info_header->infoType)
            {
                case ExecutionMarkerInfoType::CmdBufStart:
                    is_cmd_buf_start_info_found = true;
                    break;
                case ExecutionMarkerInfoType::BarrierBegin:
                    is_barrier_found = true;
                    break;
                case ExecutionMarkerInfoType::PipelineBind:
                    is_pipeline_bind_found = true;
                    break;
                default:
                    break;
            }

            // Check if Draw/Dispatch info event is found when previous marker is Draw/Dispatch Begin.
            if (is_previous_marker_draw_begin && exec_marker_info_header->infoType != ExecutionMarkerInfoType::Draw)
            {
                is_draw_info_found = false;
            }
            else if (is_previous_marker_dispatch_begin && exec_marker_info_header->infoType != ExecutionMarkerInfoType::Dispatch)
            {
                is_dispatch_info_found = false;
            }

            // Reset the previous begin marker flags.
            is_previous_marker_draw_begin     = false;
            is_previous_marker_dispatch_begin = false;

            if (is_cmd_buf_start_info_found
                && is_draw_info_found
                && is_dispatch_info_found
                && is_barrier_found
                && is_pipeline_bind_found)
            {
                is_marker_context_found = true;
            }
        }
    }

    if (!is_marker_context_found)
    {
        std::stringstream missing_events_txt;
        missing_events_txt << "marker context/info events are missing in the crash dump file. Missing info events:";
        if (!is_cmd_buf_start_info_found)
        {
            missing_events_txt << " CmdBufStart";
        }
        if (!is_draw_info_found)
        {
            missing_events_txt << " " << kStrDraw;
        }
        if (!is_dispatch_info_found)
        {
            missing_events_txt << " " << kStrDispatch;
        }
        if (!is_barrier_found)
        {
            missing_events_txt << " Barrier";
        }
        if (!is_pipeline_bind_found)
        {
            missing_events_txt << " PipelineBind";
        }

        RgdUtils::PrintMessage(missing_events_txt.str().c_str(), RgdMessageType::kError, true);
    }

    return is_marker_context_found;
}