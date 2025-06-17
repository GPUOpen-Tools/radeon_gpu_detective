//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief  DXBC parser and debug info extractor implementation.
//============================================================================================

#include <iostream>
#include <memory>
#include <cstring>
#include <cassert>
#include <sstream>
#include <regex>
#include <unordered_map>

// Add miniz header for zlib decompression.
#include "miniz/miniz.h"

// Local.
#include "rgd_dxbc_parser.h"
#include "rgd_utils.h"
#include "rgd_process_utils.h"
#include "rgd_data_types.h"

namespace {
    // Constants with internal linkage.
    constexpr const char* kDxEntryPointsTag = "!dx.entryPoints";
    constexpr const char* kDxMainFileNameTag = "!dx.source.mainFileName";
    constexpr const char* kDxSourceContentsTag = "!dx.source.contents";
    constexpr const char* kDxcExecutablePath = ".\\utils\\dx12\\dxc\\dxc.exe";
    
    // DXBC magic number: "DXBC".
    constexpr uint8_t kDxbcMagic[4] = {'D', 'X', 'B', 'C'};

    // DXBC part identifiers - "ILDN".
    constexpr uint8_t kFourccIldn[4] = {'I', 'L', 'D', 'N'};

    // DXBC part identifiers - "SRCI".
    constexpr uint8_t kFourccSrci[4] = {'S', 'R', 'C', 'I'};

    // Console message constants.
    constexpr const char* kMsgFailedToOpenFile = "failed to open file: ";
    constexpr const char* kMsgFailedToReadDxbcHeader = "failed to read DXBC header from file: ";
    constexpr const char* kMsgInvalidDxbcMagic = "invalid DXBC magic number in file: ";
    constexpr const char* kMsgFailedToReadChunkOffsets = "failed to read chunk offsets from file: ";
    constexpr const char* kMsgFilesystemError = "filesystem error while searching for ";
    constexpr const char* kMsgFoundPdbFile = "found PDB file: ";
    constexpr const char* kMsgFoundPdbInSubdir = "found PDB file in subdirectory: ";
    constexpr const char* kMsgFoundPdbInIldn = "found PDB filename in ILDN chunk: ";
    constexpr const char* kMsgDxcNotFound = "dxc.exe not found at path: ";
    constexpr const char* kMsgDxcFailedInvocation = "failed to invoke dxc.exe or capture output. Exit code: ";
    constexpr const char* kMsgFailedToOpenPdb = "failed to open PDB file: ";
    constexpr const char* kMsgFailedToReadPdb = "failed to read PDB file: ";
    constexpr const char* kMsgNoPdbPath = "no ILDN chunk with PDB path found in file: ";
    constexpr const char* kMsgPdbFilenameEmpty = "PDB filename is empty.";
    constexpr const char* kMsgFoundMsfFormat = "found MSF format PDB file. Searching for DXBC container...";
    constexpr const char* kMsgErrorOutput = "error output: ";
    constexpr const char* kMsgFailedToFindSource = "failed to find source contents for file: ";
    constexpr const char* kMsgSuccessExtractSource = "successfully extracted source code from PDB file: ";
    constexpr const char* kMsgNoSourceFiles = "no source files found in PDB file: ";
}

// Initialize static regex patterns.
const std::regex RgdDxbcParser::kLineRefPattern("!\\{!(\\d+)\\}");
const std::regex RgdDxbcParser::kEntryNamePattern("@\\w+, !\"([^\"]+)\"");
const std::regex RgdDxbcParser::kFileNamePattern("!\\{!\"([^\"]+)\"\\}");
const std::regex RgdDxbcParser::kContentRefPattern("!(\\d+)");
const std::regex RgdDxbcParser::kNewlinePattern("\\\\n");
const std::regex RgdDxbcParser::kTabPattern("\\\\t");
const std::regex RgdDxbcParser::kQuotePattern("\\\\\"");
const std::regex RgdDxbcParser::kHexNewlinePattern("\\\\0A");

bool RgdDxbcParser::Initialize(const Config& user_config, const std::vector<std::string>& debug_info_dirs)
{
    debug_info_dirs_ = debug_info_dirs;
    is_verbose_      = user_config.is_verbose;
    return !debug_info_dirs_.empty();
}

bool RgdDxbcParser::GetDumpbinOutputForFile(const std::string& input_pdb_file_path, std::string& dxc_dumpbin_output)
{
    bool result = false;
    constexpr const char* kDumpBinCommand = "-dumpbin";
    
    // Use dxc.exe to dump binary information.
    if (!std::filesystem::exists(kDxcExecutablePath))
    {
        std::stringstream error_msg;
        error_msg << kMsgDxcNotFound << kDxcExecutablePath;
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, true);
    }
    else
    {
        // Prepare arguments for the DXC command.
        std::vector<std::string> arguments = {kDumpBinCommand, input_pdb_file_path};
            
        std::string dxc_error_output;

        // Execute the command and capture output.
        int process_result = RgdProcessUtils::ExecuteAndCapture(kDxcExecutablePath, arguments, dxc_dumpbin_output, dxc_error_output);
                
        result = (process_result == 0 && !dxc_dumpbin_output.empty());
            
        if (!result)
        {
            std::stringstream error_msg;
            error_msg << kMsgDxcFailedInvocation << process_result << std::endl;
            if (!dxc_error_output.empty())
            {
                error_msg << kMsgErrorOutput << dxc_error_output;
            }
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
        }
    }
    
    return result;
}

bool RgdDxbcParser::FindDxbcFileByHash(uint64_t hash_hi, uint64_t hash_lo, std::string& file_path)
{
    bool result = false;
    
    if (!debug_info_dirs_.empty())
    {
        try
        {
            // Iterate through all directories.
            for (const auto& debug_info_dir : debug_info_dirs_)
            {
                if (!debug_info_dir.empty() || std::filesystem::exists(debug_info_dir))
                {
                    // Iterate through all files in the current debug info directory.
                    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(debug_info_dir))
                    {
                        if (entry.is_regular_file())
                        {
                            const std::string& current_file = entry.path().string();

                            // Check if this file is a 'DXBC' file and the digest matches our hash.
                            if (CheckDigestMatch(current_file, hash_hi, hash_lo))
                            {
                                file_path = current_file;
                                result    = true;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // Should not reach here.
                    assert(false);
                }
                
                if (result)
                {
                    // Found a match, stop searching directories.
                    break;
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::stringstream error_msg;
            error_msg << kMsgFilesystemError << "DXBC files: " << e.what();
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
        }
    }

    return result;
}

std::ifstream RgdDxbcParser::OpenFileStream(const std::string& file_path, std::ios_base::openmode mode)
{
    std::ifstream file(file_path, mode);
    
    if (!file.is_open())
    {
        std::stringstream error_msg;
        error_msg << kMsgFailedToOpenFile << file_path;
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
    }
    
    return file;
}

bool RgdDxbcParser::CheckDigestMatch(const std::string& file_path, uint64_t hash_hi, uint64_t hash_lo)
{
    bool result = false;
    std::ifstream file = OpenFileStream(file_path, std::ios::binary);
    
    if (file.is_open())
    {
        // For the input file, check magic number to be 'DXBC' and ignore the file if it doesn't match.
        // If PDBs are generated using '-Zi -Qembed_debug', a single file with magic number 'DXBC' is created for each shader and it includes the debug info.
        // If PDBs are generated using '-Zi -Qsource_in_debug_module', a file with magic number 'DXBC' along with a separate PDB file is created for each shader.
        // If PDBs are generated using '-Zs', a file with magic number 'DXBC' along with a separate slim/small PDB file is created for each shader. This slim pdb file only contains the shader source code and no debug info.
        // Read the header.
        DxbcHeader header;
        if (!file.read(reinterpret_cast<char*>(&header), sizeof(header)))
        {
            std::stringstream error_msg;
            error_msg << kMsgFailedToReadDxbcHeader << file_path;
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
        }
        else if (memcmp(header.magic, kDxbcMagic, sizeof(kDxbcMagic)) == 0)
        {
            // Extract the digest (16 bytes = 128 bits).
            uint64_t file_digest_lo = 0;
            uint64_t file_digest_hi = 0;

            // First 8 bytes for hi part.
            memcpy(&file_digest_hi, header.digest, sizeof(uint64_t));
            
            // Next 8 bytes for low part.
            memcpy(&file_digest_lo, header.digest + sizeof(uint64_t), sizeof(uint64_t));

            // Compare with our hash values.
            result = (file_digest_lo == hash_lo) && (file_digest_hi == hash_hi);
        }
    }
    
    return result;
}

bool RgdDxbcParser::ExtractShaderDebugInfo(
    const std::string& dxc_dumpbin_output,
    std::string& entry_name,
    std::string& source_file_name,
    std::string& high_level_source,
    std::string& io_and_bindings,
    const bool is_separate_pdb)
{
    // Maps to store the line contents.
    std::unordered_map<std::string, std::string> line_map;
    
    // Parse the file line by line.
    std::istringstream iss(dxc_dumpbin_output);
    std::string line;
    while (std::getline(iss, line)) 
    {
        // Extract line number and content for lines starting with "!".
        if (line.size() > 1 && line[0] == '!') 
        {
            size_t equal_pos = line.find("=");
            if (equal_pos != std::string::npos) 
            {
                std::string line_num = line.substr(0, equal_pos);
                std::string content = line.substr(equal_pos + 1);
                
                // Remove leading and trailing whitespace from line number and content.
                std::string trimmed_line_num;
                RgdUtils::TrimLeadingAndTrailingWhitespace(line_num, trimmed_line_num);
                
                std::string trimmed_content;
                RgdUtils::TrimLeadingAndTrailingWhitespace(content, trimmed_content);
                
                line_map[trimmed_line_num] = trimmed_content;
            }
        }
    }
    
    // Call specialized helper functions to extract different parts of the debug info.
    bool is_found_entry = FindEntryPoint(line_map, entry_name);
    bool is_found_file = FindSourceFileName(line_map, source_file_name);

    bool is_found_source = false;
    
    if (is_found_file) {
        is_found_source = FindSourceContents(line_map, source_file_name, high_level_source);
        assert(is_found_source);
        if (!is_found_source) {
            std::stringstream error_msg;
            error_msg << kMsgFailedToFindSource << source_file_name;
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
        }
    }

    if (is_separate_pdb && is_found_entry && !is_found_file)
    {
        // When entry point is found but source file name is not, it may be a PDB generated without source info.
        // Recommend user to generate PDB with source info using '-Zi -Qembed_debug' or '-Zi -Qsource_in_debug_module or -Zs' options.
        std::stringstream txt;
        txt << kStrNotAvailable
            << " (PDB file generated without high-level source code, recompile shader with '-Zi -Qembed_debug', '-Zi -Qsource_in_debug_module' or 'Zs')"
            << std::endl;
        high_level_source = txt.str();
        source_file_name  = kStrNotAvailable;
        is_found_file     = true;
        is_found_source   = true;
    }

    // Extract shader IO and resource bindings.
    bool is_found_io_bindings = FindShaderIoAndBindings(dxc_dumpbin_output, io_and_bindings);
    
    return is_found_entry && is_found_file && is_found_source && is_found_io_bindings;
}

bool RgdDxbcParser::FindEntryPoint(const std::unordered_map<std::string, std::string>& line_map, std::string& entry_name)
{
    bool result = false;
    std::unordered_map<std::string, std::string>::const_iterator entry_points_it = line_map.find(kDxEntryPointsTag);
    if (entry_points_it != line_map.end()) 
    {
        std::string entry_points_ref = entry_points_it->second;

        // Extract the line number reference (e.g., !{!364}).
        std::smatch matches;
        if (std::regex_search(entry_points_ref, matches, kLineRefPattern)) 
        {
            std::string entry_point_line = "!" + matches[1].str();

            // Find the actual entry point line.
            std::unordered_map<std::string, std::string>::const_iterator entry_line_it = line_map.find(entry_point_line);
            if (entry_line_it != line_map.end()) 
            {
                // Extract entry name (the second string in the tuple).
                if (std::regex_search(entry_line_it->second, matches, kEntryNamePattern)) 
                {
                    entry_name = matches[1].str();
                    result = true;
                }
            }
        }
    }
    return result;
}

bool RgdDxbcParser::FindSourceFileName(const std::unordered_map<std::string, std::string>& line_map, std::string& source_file_name)
{
    bool result = false;
    std::unordered_map<std::string, std::string>::const_iterator main_file_it = line_map.find(kDxMainFileNameTag);
    if (main_file_it != line_map.end()) 
    {
        std::string main_file_ref = main_file_it->second;

        // Extract the line number reference.
        std::smatch matches;
        if (std::regex_search(main_file_ref, matches, kLineRefPattern)) 
        {
            std::string main_file_line = "!" + matches[1].str();

            // Find the actual main file line.
            std::unordered_map<std::string, std::string>::const_iterator file_line_it = line_map.find(main_file_line);
            if (file_line_it != line_map.end()) 
            {
                // Extract file name.
                if (std::regex_search(file_line_it->second, matches, kFileNamePattern)) 
                {
                    source_file_name = matches[1].str();
                    result = true;
                }
            }
        }
    }
    return result;
}

std::regex RgdDxbcParser::CreateFileContentPattern(const std::string& source_file_name) const
{
    return std::regex("!\\{!\"" + source_file_name + "\", !\"([\\s\\S]+?)\"\\}");
}

bool RgdDxbcParser::FindSourceContents(
    const std::unordered_map<std::string, std::string>& line_map, 
    const std::string& source_file_name, 
    std::string& high_level_source)
{
    bool result = false;
    std::unordered_map<std::string, std::string>::const_iterator source_contents_it = line_map.find(kDxSourceContentsTag);
    if (source_contents_it != line_map.end()) 
    {
        std::string content_refs = source_contents_it->second;

        // Extract all content references.
        std::sregex_iterator it(content_refs.begin(), content_refs.end(), kContentRefPattern);
        std::sregex_iterator end;
        
        // Create regex pattern for this specific source file.
        std::regex file_content_pattern = CreateFileContentPattern(source_file_name);
        
        while (it != end && !result) 
        {
            std::smatch match = *it;
            std::string content_line = "!" + match[1].str();
            std::unordered_map<std::string, std::string>::const_iterator content_it = line_map.find(content_line);
            
            if (content_it != line_map.end()) 
            {
                // Check if this content entry is for our main file.
                std::smatch content_match;
                if (std::regex_search(content_it->second, content_match, file_content_pattern)) 
                {
                    // Extract the source code (unescape escape sequences).
                    high_level_source = content_match[1].str();

                    // Unescape common escape sequences using static regex patterns.
                    high_level_source = std::regex_replace(high_level_source, kNewlinePattern, "\n");
                    high_level_source = std::regex_replace(high_level_source, kTabPattern, "\t");
                    high_level_source = std::regex_replace(high_level_source, kQuotePattern, "\"");
                    high_level_source = std::regex_replace(high_level_source, kHexNewlinePattern, "\n");
                    result = true;
                }
            }
            ++it;
        }
    }
    return result;
}

bool RgdDxbcParser::FindShaderIoAndBindings(const std::string& dxc_dumpbin_output, std::string& io_and_bindings)
{
    // Parse the file line by line to extract shader IO and resource bindings.
    std::istringstream iss(dxc_dumpbin_output);
    std::string line;
    bool found_section = false;
    bool section_ended = false;
    std::stringstream extracted_content;
    
    while (std::getline(iss, line) && !section_ended) 
    {
        // Trim whitespace from the line.
        std::string trimmed_line;
        RgdUtils::TrimLeadingAndTrailingWhitespace(line, trimmed_line);
        
        // Check if the line starts with a semicolon.
        if (!trimmed_line.empty() && trimmed_line[0] == ';') 
        {
            found_section = true;
            extracted_content << trimmed_line << std::endl;
        }
        else if (found_section) 
        {
            // Once we've found lines with semicolons and encountered a line without,
            // we've reached the end of the section.
            section_ended = true;
        }
    }
    
    io_and_bindings = extracted_content.str();
    return found_section;
}

bool RgdDxbcParser::ExtractSeparatePdbFileNameFromDxbc(const std::string& dxbc_file_path, std::string& pdb_file_name)
{
    bool result = false;
    std::ifstream file = OpenFileStream(dxbc_file_path, std::ios::binary);
    
    if (file.is_open())
    {
        // Read the header.
        DxbcHeader header;
        if (file.read(reinterpret_cast<char*>(&header), sizeof(header)))
        {
            if (memcmp(header.magic, kDxbcMagic, sizeof(kDxbcMagic)) == 0)
            {
                // Read chunk offsets.
                std::vector<uint32_t> chunk_offsets(header.part_count);
                if (file.read(reinterpret_cast<char*>(chunk_offsets.data()), sizeof(uint32_t) * header.part_count))
                {
                    // Find the ILDN chunk.
                    for (uint32_t chunk_idx = 0; chunk_idx < header.part_count && !result; chunk_idx++)
                    {
                        // Seek to chunk position.
                        file.seekg(chunk_offsets[chunk_idx], std::ios::beg);
                        
                        // Read chunk header.
                        uint32_t fourcc;
                        uint32_t chunk_size;
                        if (file.read(reinterpret_cast<char*>(&fourcc), sizeof(fourcc)) &&
                            file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size)))
                        {
                            // Check if this is an ILDN chunk.
                            if (memcmp(&fourcc, kFourccIldn, sizeof(kFourccIldn)) == 0)
                            {
                                // Read ILDN header.
                                IldnHeader ildn_header;
                                if (file.read(reinterpret_cast<char*>(&ildn_header), sizeof(ildn_header)))
                                {
                                    // Read the path name.
                                    std::vector<char> path_buffer(ildn_header.name_length);
                                    if (file.read(path_buffer.data(), ildn_header.name_length))
                                    {
                                        // Extract the file name from the path (could be a full path).
                                        std::string path_str(path_buffer.data(), ildn_header.name_length);
                                        
                                        // Get just the filename part.
                                        std::filesystem::path full_path(path_str);
                                        pdb_file_name = full_path.filename().string();
                                        
                                        std::stringstream debug_msg;
                                        debug_msg << kMsgFoundPdbInIldn << pdb_file_name;
                                        RgdUtils::PrintMessage(debug_msg.str().c_str(), RgdMessageType::kInfo, is_verbose_);
                                        
                                        result = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (!result)
                    {
                        std::stringstream warning_msg;
                        warning_msg << kMsgNoPdbPath << dxbc_file_path;
                        RgdUtils::PrintMessage(warning_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
                    }
                }
                else
                {
                    std::stringstream error_msg;
                    error_msg << kMsgFailedToReadChunkOffsets << dxbc_file_path;
                    RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
                }
            }
            else
            {
                std::stringstream error_msg;
                error_msg << kMsgInvalidDxbcMagic << dxbc_file_path;
                RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
            }
        }
        else
        {
            std::stringstream error_msg;
            error_msg << kMsgFailedToReadDxbcHeader << dxbc_file_path;
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
        }
    }
    else
    {
        std::stringstream error_msg;
        error_msg << kMsgFailedToOpenPdb << dxbc_file_path;
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
    }
    
    return result;
}

bool RgdDxbcParser::FindPdbFileInDirectories(const std::string& pdb_file_name, std::string& found_pdb_path)
{
    bool result = false;
    
    if (!pdb_file_name.empty())
    {
        for (const auto& dir : debug_info_dirs_)
        {
            if (!dir.empty() || std::filesystem::exists(dir))
            {
                try
                {
                    std::filesystem::path test_path = std::filesystem::path(dir) / pdb_file_name;
                    if (std::filesystem::exists(test_path))
                    {
                        found_pdb_path = test_path.string();

                        std::stringstream info_msg;
                        info_msg << kMsgFoundPdbFile << found_pdb_path;
                        RgdUtils::PrintMessage(info_msg.str().c_str(), RgdMessageType::kInfo, is_verbose_);

                        result = true;
                        break;
                    }

                    // Recursively search subdirectories if the file wasn't found directly.
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
                    {
                        if (entry.is_regular_file() && entry.path().filename() == pdb_file_name)
                        {
                            found_pdb_path = entry.path().string();

                            std::stringstream info_msg;
                            info_msg << kMsgFoundPdbInSubdir << found_pdb_path;
                            RgdUtils::PrintMessage(info_msg.str().c_str(), RgdMessageType::kInfo, is_verbose_);

                            result = true;
                            break;
                        }
                    }
                }
                catch (const std::filesystem::filesystem_error& e)
                {
                    std::stringstream error_msg;
                    error_msg << kMsgFilesystemError << "PDB: " << e.what();
                    RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                }
            }
            else
            {
                // Should not reach here.
                assert(false);
            }
            if (result)
            {
                // Found the PDB file, no need to search further.
                break;
            }
        }
    }
    else
    {
        RgdUtils::PrintMessage(kMsgPdbFilenameEmpty, RgdMessageType::kError, is_verbose_);
    }
    
    return result;
}

bool RgdDxbcParser::ExtractDebugInfoFromSmallPdb(
    const std::string& pdb_file_path, std::string& high_level_source_code,
    std::string& source_file_name)
{
    bool result = false;
    std::ifstream file;
    std::vector<uint8_t> file_data;
    size_t file_size = 0;
    const uint8_t* dxbc_start = nullptr;
    size_t dxbc_size = 0;
    bool is_msf_format = false;

    // Open the PDB file as binary.
    file = OpenFileStream(pdb_file_path, std::ios::binary);
    
    if (file.is_open())
    {
        // Read the entire file into a buffer.
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        file_data.resize(file_size);
        if (file.read(reinterpret_cast<char*>(file_data.data()), file_size))
        {
            // Check if the file starts with "Microsoft C/C++ MSF 7.00".
            const char* msf_signature = "Microsoft C/C++ MSF 7.00";
            is_msf_format = (file_size > std::strlen(msf_signature) && 
                            std::memcmp(file_data.data(), msf_signature, std::strlen(msf_signature)) == 0);

            // If it's not a DXBC file directly, we need to find the DXBC section in the PDB.
            dxbc_start = file_data.data();
            dxbc_size = file_size;

            if (is_msf_format)
            {
                // For now, just log that we found an MSF format file.
                std::stringstream info_msg;
                info_msg << kMsgFoundMsfFormat;
                RgdUtils::PrintMessage(info_msg.str().c_str(), RgdMessageType::kInfo, is_verbose_);

                // Search for DXBC magic within the file.
                for (size_t i = 0; i < file_size - sizeof(kDxbcMagic) && !result; i++)
                {
                    if (std::memcmp(file_data.data() + i, kDxbcMagic, sizeof(kDxbcMagic)) == 0)
                    {
                        dxbc_start = file_data.data() + i;
                        dxbc_size = file_size - i;
                        break;
                    }
                }
            }

            // Check if it has a valid DXBC magic number now.
            if (dxbc_size >= sizeof(DxbcHeader) && std::memcmp(dxbc_start, kDxbcMagic, sizeof(kDxbcMagic)) == 0)
            {
                // Parse the DXBC container header.
                const DxbcHeader* header = reinterpret_cast<const DxbcHeader*>(dxbc_start);
                const uint32_t* chunk_offsets = reinterpret_cast<const uint32_t*>(dxbc_start + sizeof(DxbcHeader));

                // Look for the SRCI chunk.
                for (uint32_t chunk_idx = 0; chunk_idx < header->part_count && !result; chunk_idx++)
                {
                    uint32_t chunk_offset = chunk_offsets[chunk_idx];
                    const uint32_t* fourcc = reinterpret_cast<const uint32_t*>(dxbc_start + chunk_offset);
                    const uint32_t* chunk_size = fourcc + 1;
                    const uint8_t* chunk_contents = reinterpret_cast<const uint8_t*>(chunk_size + 1);

                    if (std::memcmp(fourcc, kFourccSrci, sizeof(kFourccSrci)) == 0)
                    {
                        // We found SRCI chunk, now process it.
                        const SRCIHeader* srci = reinterpret_cast<const SRCIHeader*>(chunk_contents);
                        chunk_contents += sizeof(SRCIHeader);

                        std::vector<ShaderSourceFile> source_files;
                        bool processing_failed = false;

                        // Process each section of the SRCI chunk.
                        for (uint32_t sec = 0; sec < srci->num_sections && !processing_failed; sec++)
                        {
                            const SRCISection* section = reinterpret_cast<const SRCISection*>(chunk_contents);
                            const uint8_t* section_contents = reinterpret_cast<const uint8_t*>(section + 1);
                            
                            // Move to the next section for the next iteration.
                            chunk_contents += section->section_size;

                            switch (section->type)
                            {
                                case SRCISectionType::FileContents:
                                {
                                    const SRCIFileContentsSection* contents = reinterpret_cast<const SRCIFileContentsSection*>(section + 1);

                                    if (!source_files.empty() && source_files.size() != contents->num_files)
                                    {
                                        std::stringstream error_msg;
                                        error_msg << "unexpected number of source files in contents section " 
                                                << contents->num_files << " when we have " << source_files.size() << " already";
                                        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                                    }
                                    else
                                    {
                                        source_files.resize(contents->num_files);

                                        const uint8_t* contents_data = reinterpret_cast<const uint8_t*>(contents + 1);
                                        std::vector<uint8_t> decompressed_data;
                                        bool decompression_succeeded = true;

                                        // Handle zlib compressed data.
                                        if (contents->zlib_compressed == 1)
                                        {
                                            decompressed_data.resize(contents->uncompressed_data_size);

                                            // Initialize miniz.
                                            mz_stream stream = {};
                                            stream.next_in = contents_data;
                                            stream.avail_in = contents->data_size;
                                            stream.next_out = decompressed_data.data();
                                            stream.avail_out = contents->uncompressed_data_size;

                                            int status = mz_inflateInit(&stream);
                                            if (status != MZ_OK)
                                            {
                                                std::stringstream error_msg;
                                                error_msg << "could not initialize zlib decompressor.";
                                                RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                                                decompression_succeeded = false;
                                            }
                                            else
                                            {
                                                status = mz_inflate(&stream, MZ_FINISH);
                                                if (status != MZ_STREAM_END)
                                                {
                                                    mz_inflateEnd(&stream);
                                                    std::stringstream error_msg;
                                                    error_msg << "zlib decompression failed with status " << status;
                                                    RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                                                    decompression_succeeded = false;
                                                }
                                                else
                                                {
                                                    decompressed_data.resize(stream.total_out);
                                                    
                                                    status = mz_inflateEnd(&stream);
                                                    if (status != MZ_OK)
                                                    {
                                                        std::stringstream error_msg;
                                                        error_msg << "failed shutting down zlib decompressor.";
                                                        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                                                        decompression_succeeded = false;
                                                    }
                                                }
                                            }

                                            if (decompression_succeeded)
                                            {
                                                contents_data = decompressed_data.data();
                                            }
                                            else
                                            {
                                                processing_failed = true;
                                                break;
                                            }
                                        }

                                        // Extract file contents from data (compressed or uncompressed).
                                        for (uint32_t file_idx = 0; file_idx < contents->num_files; file_idx++)
                                        {
                                            const SRCIFileContentsEntry* contents_entry = reinterpret_cast<const SRCIFileContentsEntry*>(contents_data);
                                            const char* file_contents = reinterpret_cast<const char*>(contents_entry + 1);

                                            // Should be null terminated but don't take any chances.
                                            source_files[file_idx].contents.assign(file_contents, contents_entry->file_size - 1);

                                            contents_data += contents_entry->entry_size;
                                        }
                                    }
                                    break;
                                }
                                
                                case SRCISectionType::Filenames:
                                {
                                    const SRCIFilenamesSection* names = reinterpret_cast<const SRCIFilenamesSection*>(section + 1);

                                    if (!source_files.empty() && source_files.size() != names->num_files)
                                    {
                                        std::stringstream error_msg;
                                        error_msg << "unexpected number of source files in filenames section "
                                                << names->num_files << " when we have " << source_files.size() << " already";
                                        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
                                    }
                                    else
                                    {
                                        source_files.resize(names->num_files);

                                        const uint8_t* name_contents = reinterpret_cast<const uint8_t*>(names) + SRCIFilenamesSection::unpadded_size();
                                        for (uint32_t file_idx = 0; file_idx < names->num_files; file_idx++)
                                        {
                                            const SRCIFilenameEntry* filename_entry = reinterpret_cast<const SRCIFilenameEntry*>(name_contents);
                                            const char* filename = reinterpret_cast<const char*>(filename_entry + 1);

                                            source_files[file_idx].filename.assign(filename, filename_entry->name_size - 1);

                                            name_contents += filename_entry->entry_size;
                                        }
                                    }
                                    break;
                                }
                                
                                case SRCISectionType::Args:
                                {
                                    // We don't need to process arguments for our purpose.
                                    break;
                                }
                                
                                default:
                                {
                                    std::stringstream warning_msg;
                                    warning_msg << "unexpected SRCI section type " << static_cast<uint16_t>(section->type) << ".";
                                    RgdUtils::PrintMessage(warning_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
                                    break;
                                }
                            }
                        }

                        // Extract the high-level source code and source file name.
                        if (!processing_failed && !source_files.empty())
                        {
                            // We'll use the first source file as the main one.
                            source_file_name = source_files[0].filename;
                            high_level_source_code = source_files[0].contents;
                            result = true;
                            
                            std::stringstream info_msg;
                            info_msg << kMsgSuccessExtractSource << pdb_file_path;
                            RgdUtils::PrintMessage(info_msg.str().c_str(), RgdMessageType::kInfo, is_verbose_);
                        }
                        else if (!processing_failed)
                        {
                            std::stringstream warning_msg;
                            warning_msg << kMsgNoSourceFiles << pdb_file_path;
                            RgdUtils::PrintMessage(warning_msg.str().c_str(), RgdMessageType::kWarning, is_verbose_);
                        }
                    }
                }
            }
            else
            {
                std::stringstream error_msg;
                error_msg << kMsgInvalidDxbcMagic << pdb_file_path;
                RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
            }
        }
        else
        {
            std::stringstream error_msg;
            error_msg << kMsgFailedToReadPdb << pdb_file_path;
            RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
        }
    }
    else
    {
        std::stringstream error_msg;
        error_msg << kMsgFailedToOpenPdb << pdb_file_path;
        RgdUtils::PrintMessage(error_msg.str().c_str(), RgdMessageType::kError, is_verbose_);
    }

    return result;
}