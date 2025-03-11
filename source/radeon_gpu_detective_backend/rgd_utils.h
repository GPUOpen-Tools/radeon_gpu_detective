//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  general utilities.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_UTILS_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_UTILS_H_

// C++.
#include <string>

// System Info.
#include "system_info_utils/source/system_info_reader.h"

// Dev driver.
#include "dev_driver/include/rgdevents.h"

// Types of messages that are printed by the tool.
enum class RgdMessageType
{
    kInfo,
    kWarning,
    kError
};

class RgdUtils
{
public:

    // *** FILESYSTEM - START ***

    // Returns true if the file exists and false otherwise.
    static bool IsFileExists(const std::string& file_name);

    // Returns true if the given full path is valid and false otherwise.
    static bool IsValidFilePath(const std::string& file_name);

    // Returns creation time of the input crash dump file.
    static std::string GetFileCreationTime(const std::string& file_name);

    // Writes the contents into the given full text file path. Returns true on success and false otherwise.
    static bool WriteTextFile(const std::string& file_name, const std::string& contents);

    // *** FILESYSTEM - END ***

    // Prints a message to the console based on the the message type and the verbosity level.
    // Error messages are always printed to std::cerr.
    // Warning and INFO messages are only printed in verbose mode.
    static void PrintMessage(const char* msg, RgdMessageType msg_type, bool is_verbose);

    // Trim the whitespace off the left side of the incoming string.
    static void LeftTrim(const std::string& text, std::string& trimmed_text);

    // Trim the whitespace off the right side of the incoming string.
    static void RightTrim(const std::string& text, std::string& trimmed_text);

    // Trim the leading or trailing whitespaces if any.
    static void TrimLeadingAndTrailingWhitespace(const std::string& text, std::string& trimmed_text);

    // Append and return the heap type string with developer friendly type name.
    static std::string ToHeapTypeString(const std::string& heap_type_str);

    // Format the input number to a comma separated string.
    static std::string ToFormattedNumericString(size_t  number);

    // Returns the command buffer queue type string.
    static std::string GetCmdBufferQueueTypeString(CmdBufferQueueType queue_type);

    // Returns the Execution Marker API type string.
    static std::string GetExecMarkerApiTypeString(CrashAnalysisExecutionMarkerApiType api_type);

    // Returns the API string.
    static std::string GetApiString(TraceApiType api_type);

    // Returns the hang type string.
    static std::string GetHangTypeString(HangType hang_type);

    // Returns the alphanumeric string id bu appending the input string prefix and id.
    static std::string GetAlphaNumericId(std::string id_prefix, uint64_t id);

private:
    RgdUtils() = delete;
    ~RgdUtils() = delete;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_UTILS_H_
