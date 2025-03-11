//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

// JSON.
#include "json/single_include/nlohmann/json.hpp"

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
    static bool BuildCommandBufferMapping(const Config& user_config, const CrashData& umd_crash_data,
                                          std::unordered_map<uint64_t, std::vector<size_t>>& cmd_buffer_mapping);

    // Returns a formatted size string for the number of bytes (KB, MB etc.).
    static std::string GetFormattedSizeString(uint64_t size_in_bytes, const char* unit = "B");

    // Returns the string representation of the Umd event type based on its ID.
    static std::string UmdRgdEventIdToString(uint8_t event_id);

    // Returns the string representation of the Kmd event type based on its ID.
    static std::string KmdRgdEventIdToString(uint8_t event_id);

    // Extracts the name of the marker's source which is packed into the upper 4 bits of the marker value.
    static std::string ExtractMarkerSource(uint32_t marker_value);

    // Parses a TraceProcessInfo chunk.
    static bool ParseTraceProcessInfoChunk(rdf::ChunkFile& chunk_file, const char* chunk_identifier, TraceProcessInfo& process_info);

    // Parse a 'DriverOverrides' chunk from the given chunk file.
    static bool ParseDriverOverridesChunk(rdf::ChunkFile& chunk_file, const char* chunk_identifier, nlohmann::json& driver_experiments_json);

    // Parse a 'CodeObject' chunk from the given chunk file.
    static bool ParseCodeObjectChunk(rdf::ChunkFile& chunk_file, const char* chunk_identifier, std::map<Rgd128bitHash, CodeObject>& code_objects_map);

    // Parse a 'COLoadEvent' chunk from the given chunk file.
    static bool ParseCodeObjectLoadEventChunk(rdf::ChunkFile&                      chunk_file,
                                              const char*                          chunk_identifier,
                                              std::vector<RgdCodeObjectLoadEvent>& code_object_loader_events);

    // Parse a 'PsoCorrelation' chunk from the given chunk file.
    static bool PsoCorrelationChunk(rdf::ChunkFile& chunk_file, const char* chunk_identifier, std::vector<RgdPsoCorrelation>& pso_correlations);

    // Get the crash type - page fault or hang.
    static bool GetIsPageFault();

private:
    RgdParsingUtils() = delete;
    ~RgdParsingUtils() = delete;

    // Set the crash type - page fault or hang.
    static void SetIsPageFault(bool is_page_fault);

    // Is the crash type a page fault or a hang?
    static bool is_page_fault_;
};

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_PARSING_UTILS_H_
