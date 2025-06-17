//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief  DXBC parser and debug info extractor.
//============================================================================================

#ifndef RGD_DXBC_PARSER_H_
#define RGD_DXBC_PARSER_H_

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <regex>

// Local.
#include "rgd_data_types.h"

// DirectX Container format structures.
// Based on https://llvm.org/docs/DirectX/DXContainer.html#file-header

// DXBC file header.
struct DxbcHeader
{
    // 'DXBC' signature.
    uint8_t  magic[4];

    // 128-bit hash digest.
    uint8_t  digest[16];
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t file_size;
    uint32_t part_count;
};

// DXBC part header.
struct DxbcPartHeader
{
    // Four-character part name.
    uint8_t  name[4];
    uint32_t size;
};

// ILDN part header (for debug info paths).
struct IldnHeader
{
    uint16_t flags;
    uint16_t name_length;
    // Name follows here (variable length)
};

// Structures needed for SRCI chunk parsing.
// DXBC SRCI chunk for source code information.
struct SRCIHeader
{
    uint32_t size;
    uint16_t flags;
    uint16_t num_sections;
};

// Section types in SRCI chunk.
enum class SRCISectionType : uint16_t
{
    FileContents = 0,
    Filenames,
    Args,
};

// Base section header in SRCI chunk.
struct SRCISection
{
    uint32_t section_size;
    uint16_t flags;
    SRCISectionType type;
};

// File contents section in SRCI chunk.
struct SRCIFileContentsSection
{
    uint32_t section_size;
    uint16_t flags;
    uint16_t zlib_compressed;
    uint32_t data_size;
    uint32_t uncompressed_data_size;
    uint32_t num_files;
};

// File content entry in SRCI chunk.
struct SRCIFileContentsEntry
{
    uint32_t entry_size;
    uint32_t flags;
    uint32_t file_size;
};

// Filenames section in SRCI chunk.
struct SRCIFilenamesSection
{
    uint32_t flags;
    uint32_t num_files;
    uint16_t data_size;

    // No padding here.
    uint8_t beginning_of_data;

    static constexpr size_t unpadded_size() { return offsetof(SRCIFilenamesSection, beginning_of_data); }
};

// Filename entry in SRCI chunk.
struct SRCIFilenameEntry
{
    uint32_t entry_size;
    uint32_t flags;
    uint32_t name_size;
    uint32_t file_size;
};

// Helper structure to store shader source file information.
struct ShaderSourceFile
{
    std::string filename;
    std::string contents;
};

// Class to handle DXBC file parsing and debug info extraction.
class RgdDxbcParser
{
public:
    RgdDxbcParser() = default;
    ~RgdDxbcParser() = default;

    // Initialize parser with directories containing debug info files.
    bool Initialize(const Config& user_config, const std::vector<std::string>& debug_info_dirs);

    // Find DXBC file based on shader hash.
    bool FindDxbcFileByHash(const uint64_t hash_hi, const uint64_t hash_lo, std::string& output_file_path);

    // Find debug info file that matches given shader hash.
    bool GetDumpbinOutputForFile(const std::string& input_pdb_file_path,
                               std::string& dxc_dumpbin_output);

    // Extract shader debug info from the DXBC dumpbin output.
    bool ExtractShaderDebugInfo(
        const std::string& dxc_dumpbin_output,
        std::string& entry_name,
        std::string& source_file_name,
        std::string& high_level_source,
        std::string& io_and_bindings,
        const bool is_separate_pdb = false);
        
    // Extract PDB file path from a DXBC file.
    bool ExtractSeparatePdbFileNameFromDxbc(const std::string& dxbc_file_path, std::string& pdb_file_name);
    
    // Search for a PDB file in debug directories.
    bool FindPdbFileInDirectories(const std::string& pdb_file_name, std::string& found_pdb_path);

    // Extract debug information from small PDB files (Zs option).
    bool ExtractDebugInfoFromSmallPdb(
        const std::string& pdb_file_path, std::string& high_level_source_code,
        std::string& source_file_name);

private:

    // Check if the file's digest matches the given hash.
    bool CheckDigestMatch(const std::string& file_path, const uint64_t hash_hi, const uint64_t hash_lo);

    // Helper methods for shader debug info extraction.
    bool FindEntryPoint(const std::unordered_map<std::string, std::string>& line_map, std::string& entry_name);
    bool FindSourceFileName(const std::unordered_map<std::string, std::string>& line_map, std::string& source_file_name);
    bool FindSourceContents(
        const std::unordered_map<std::string, std::string>& line_map,
        const std::string& source_file_name,
        std::string& high_level_source);
    bool FindShaderIoAndBindings(const std::string& dxc_dumpbin_output, std::string& io_and_bindings);
        
    // Helper method to create file content pattern based on file name.
    std::regex CreateFileContentPattern(const std::string& source_file_name) const;

    // Helper method for opening file streams with error handling.
    std::ifstream OpenFileStream(const std::string& file_path, std::ios_base::openmode mode = std::ios::in);

    // Directories containing debug info files.
    std::vector<std::string> debug_info_dirs_;

    // Should print verbose error messages and debug information to the console?
    bool is_verbose_ = false;
    
    // Static regex patterns for parsing.
    static const std::regex kLineRefPattern;
    static const std::regex kEntryNamePattern;
    static const std::regex kFileNamePattern;
    static const std::regex kContentRefPattern;
    static const std::regex kNewlinePattern;
    static const std::regex kTabPattern;
    static const std::regex kQuotePattern;
    static const std::regex kHexNewlinePattern;
};

#endif // RGD_DXBC_PARSER_H_