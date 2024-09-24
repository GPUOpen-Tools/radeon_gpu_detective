//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  general utilities.
//=============================================================================
#include "rgd_utils.h"
#include "rgd_data_types.h"

// C++.
#include <cassert>
#include <ctime>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <locale>
#include <algorithm>

bool RgdUtils::IsFileExists(const std::string& file_name)
{
    std::ifstream fs(file_name.c_str());
    return fs.good();
}

bool RgdUtils::IsValidFilePath(const std::string& file_name)
{
    std::ofstream file_path(file_name);
    bool is_invalid = !file_path;
    return !is_invalid;
}

bool RgdUtils::WriteTextFile(const std::string& file_name, const std::string& contents)
{
    bool ret = false;
    std::ofstream output;
    output.open(file_name.c_str());
    if (output.is_open())
    {
        output << contents << std::endl;
        output.close();
        ret = true;
    }
    return ret;
}

std::string RgdUtils::GetFileCreationTime(const std::string& file_name)
{
    struct stat fileInfo;
    std::stringstream return_txt;
    if (stat(file_name.c_str(), &fileInfo) == 0)
    {
        char time_string[100] = "";
        if (std::strftime(time_string, sizeof(time_string), "%c", std::localtime(&fileInfo.st_mtime)) != 0)
        {
            return_txt << time_string;
        }
    }

    return (return_txt.str().empty() ? kStrNotAvailable : return_txt.str());
}

void RgdUtils::PrintMessage(const char* msg, RgdMessageType msg_type, bool is_verbose)
{
    if(is_verbose || !is_verbose && msg_type == RgdMessageType::kError)
    {
        switch (msg_type)
        {
        case RgdMessageType::kInfo:
            std::cout << "INFO: " << msg << std::endl;
            break;
        case RgdMessageType::kWarning:
            std::cout << "WARNING: " << msg << std::endl;
            break;
        case RgdMessageType::kError:
            std::cerr << "ERROR: " << msg << std::endl;
            break;
        }
    }
}

void RgdUtils::LeftTrim(const std::string& text, std::string& trimmed_text)
{
    trimmed_text = text;
    auto space_iter = std::find_if(trimmed_text.begin(), trimmed_text.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    trimmed_text.erase(trimmed_text.begin(), space_iter);
}

void RgdUtils::RightTrim(const std::string& text, std::string& trimmed_text)
{
    trimmed_text = text;
    auto space_iter = std::find_if(trimmed_text.rbegin(), trimmed_text.rend(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    trimmed_text.erase(space_iter.base(), trimmed_text.end());
}

void RgdUtils::TrimLeadingAndTrailingWhitespace(const std::string& text, std::string& trimmed_text)
{
    // Trim the whitespace off the left and right sides of the incoming text.
    std::string left_trimmed;
    LeftTrim(text, left_trimmed);
    RightTrim(left_trimmed, trimmed_text);
}

// Append and return the heap type string with developer friendly type name.
std::string RgdUtils::ToHeapTypeString(const std::string& heap_type_str)
{
    assert(!heap_type_str.empty());
    std::string extended_heap_type_string = heap_type_str;

    if (heap_type_str == "local")
    {
        extended_heap_type_string = kStrHeapTypeLocal;
    }
    else if (heap_type_str == "invisible")
    {
        extended_heap_type_string = kStrHeapTypeInvisible;
    }

    return extended_heap_type_string;
}

std::string RgdUtils::ToFormattedNumericString(size_t  number)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << number;
    return ss.str();
}

// Returns the command buffer queue type string.
std::string RgdUtils::GetCmdBufferQueueTypeString(CmdBufferQueueType queue_type)
{
    std::stringstream ret;
    switch (queue_type)
    {
    case CMD_BUFFER_QUEUE_TYPE_DIRECT:
        ret << "Direct";
        break;
    case CMD_BUFFER_QUEUE_TYPE_COMPUTE:
        ret << "Compute";
        break;
    case CMD_BUFFER_QUEUE_TYPE_COPY:
        ret << "Copy";
        break;
    default:
        ret << "Unknown";
        break;
    }

    return ret.str();
}

// Returns the Execution Marker API type string.
std::string RgdUtils::GetExecMarkerApiTypeString(CrashAnalysisExecutionMarkerApiType api_type)
{
    std::stringstream ret;

    switch (api_type)
    {
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INSTANCED:
        ret << "Draw";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DRAW_INDEXED_INSTANCED:
        ret << "DrawIndexed";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH:
        ret << "Dispatch";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_RESOURCE:
        ret << "CopyResource";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_TEXTURE_REGION:
        ret << "CopyTextureRegion";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_BUFFER_REGION:
        ret << "CopyBufferRegion";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_COPY_TILES:
        ret << "CopyTiles";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_ATOMIC_COPY_BUFFER_REGION:
        ret << "AtomicCopyBufferRegion";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_BARRIER:
        ret << "Barrier";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT:
        ret << "ExecuteIndirect";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_RAYS_INDIRECT:
        ret << "DispatchRaysIndirect";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_RAYS_UNIFIED:
        ret << "DispatchRaysUnified";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_EXECUTE_INDIRECT_RAYS_UNSPECIFIED:
        ret << "ExecuteIndirectRaysUnspecified";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_INTERNAL_DISPATCH_BUILD_BVH:
        ret << "InternalDispatchBuildBvh";
        break;
    case CrashAnalysisExecutionMarkerApiType::CRASH_ANALYSIS_EXECUTION_MARKER_API_DISPATCH_MESH:
        ret << "DispatchMesh";
        break;
    default:
        ret << "Unknown";
        break;
    }

    return ret.str();
}

// Returns the API string.
std::string RgdUtils::GetApiString(TraceApiType api_type)
{
    std::stringstream ret_txt;

    switch (api_type)
    {
    case TraceApiType::GENERIC:
        ret_txt << kStrNotAvailable;
        break;
    case TraceApiType::DIRECTX_12:
        ret_txt << "DirectX 12";
        break;
    case TraceApiType::VULKAN:
        ret_txt << "Vulkan";
        break;
    case TraceApiType::DIRECTX_9:
    case TraceApiType::DIRECTX_11:
    case TraceApiType::OPENGL:
    case TraceApiType::OPENCL:
    case TraceApiType::MANTLE:
    case TraceApiType::HIP:
    case TraceApiType::METAL:
    default:
        // Should not reach here.
        assert(false);
        ret_txt << "Invalid";
        break;
    }

    return ret_txt.str();
}