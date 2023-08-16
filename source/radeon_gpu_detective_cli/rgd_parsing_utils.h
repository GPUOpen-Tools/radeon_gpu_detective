//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  utilities for parsing raw data.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_PARSING_UTILS_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_PARSING_UTILS_H_

// C++.
#include <string>
#include <vector>
#include <unordered_map>

// RDF.
#include "rdf/rdf/inc/amdrdf.h"

// Local.
#include "rgd_data_types.h"

class RgdParsingUtils
{
public:
    // Parses a CrashData chunk (either UMD or KMD) from the given chunk file. chunk_identifier is used to identify the chunk.
    static bool ParseCrashDataChunks(rdf::ChunkFile& chunk_file, const char* chunk_identifier, CrashData& umd_crash_data, CrashData& kmd_crash_data, std::string& error_msg);

    // Builds a mapping between the command buffer ID and the list of execution markers (begin and end) for that
    // command buffer ID. Execution markers will be sorted chronologically.
    static bool BuildCommandBufferMapping(const CrashData& umd_crash_data, std::unordered_map <uint64_t, std::vector<size_t>>& cmd_buffer_mapping);

    // Returns a formatted size string for the number of bytes (KB, MB etc.).
    static std::string GetFormattedSizeString(uint64_t size_in_bytes, const char* unit = "B");

    // Returns the string representation of the Umd event type based on its ID.
    static std::string UmdRgdEventIdToString(uint8_t event_id);

    // Returns the string representation of the Kmd event type based on its ID.
    static std::string KmdRgdEventIdToString(uint8_t event_id);

    // Extracts the name of the marker's source which is packed into the upper 4 bits of the marker value.
    static std::string ExtractMarkerSource(uint32_t marker_value);

private:
    RgdParsingUtils() = delete;
    ~RgdParsingUtils() = delete;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_PARSING_UTILS_H_
