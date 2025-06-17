//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief serializer for enhanced crash information.
//============================================================================================

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <regex>

// Local.
#include "rgd_asic_info.h"
#include "rgd_enhanced_crash_info_serializer.h"
#include "rgd_data_types.h"
#include "rgd_crash_info_registers_parser_context.h"
#include "rgd_crash_info_registers_parser_rdna2.h"
#include "rgd_crash_info_registers_parser_rdna3.h"
#include "rgd_crash_info_registers_parser_rdna4.h"
#include "rgd_crash_info_registers_parser_strix1.h"
#include "rgd_utils.h"
#include "rgd_code_object_database.h"

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

static constexpr size_t kMaxLinesToShowFullSource = 120;
static constexpr size_t kMaxInitialSourceLines    = 10;
static constexpr size_t kSourceContextLines       = 34;

// Factory class to create the crash info registers parser based on the asic architecture.
class CrashInfoRegistersParserFactory
{
public:
    static std::unique_ptr<ICrashInfoRegistersParser> CreateParser(ecitrace::GpuSeries gpu_series)
    {
        switch (gpu_series)
        {
        case ecitrace::GpuSeries::kNavi2:
            return std::make_unique<CrashInfoRegistersParserRdna2>();
        case ecitrace::GpuSeries::kNavi3:
            return std::make_unique<CrashInfoRegistersParserRdna3>();
        case ecitrace::GpuSeries::kNavi4:
            return std::make_unique<CrashInfoRegistersParserRdna4>();
        case ecitrace::GpuSeries::kStrix1:
            return std::make_unique<CrashInfoRegistersParserStrix1>();
        default:
            RgdUtils::PrintMessage("unsupported asic architecture.", RgdMessageType::kError, true);
            return nullptr;
        }
    }
};

static size_t GetMaxDisassemblyLenForInstructions(const std::vector<std::pair<uint64_t, std::string>>& instructions)
{
    size_t max_len = 0;
    for (const std::pair<uint64_t, std::string>& instr : instructions)
    {
        max_len = std::max(max_len, instr.second.size());
    }

    return max_len;
}

// Helper function to count lines in a source string
static size_t CountSourceLines(const std::string& source)
{
    size_t line_count = 0;
    std::istringstream stream(source);
    std::string line;
    
    while (std::getline(stream, line)) {
        line_count++;
    }
    
    return line_count;
}

// Helper function to print vertical ellipsis with a message on the second line
static void PrintVerticalEllipsisWithMessage(const std::string& message, std::stringstream& out)
{
    out << "\t." << std::endl
        << "\t." << std::endl
        << "\t." << std::endl
        << "\t. " << message << std::endl
        << "\t." << std::endl
        << "\t." << std::endl
        << "\t." << std::endl;
}

// Helper function to build source JSON with context around entry point.
static void BuildHighLevelSourceJson(const std::string& high_level_source,
                                     const std::string& entry_point_name,
                                     const bool         is_full_source,
                                     nlohmann::json&    out_json)
{
    // Check if we should display full source or apply truncation rules.
    const bool show_full_source = is_full_source || CountSourceLines(high_level_source) <= kMaxLinesToShowFullSource || entry_point_name.empty();

    out_json = nlohmann::json::array();
    if (show_full_source)
    {
        std::istringstream source_stream(high_level_source);
        std::string        source_line;
        while (std::getline(source_stream, source_line))
        {
            nlohmann::json source_line_json;
            source_line_json[KJsonElemSourceLine] = source_line;
            out_json.push_back(source_line_json);
        }
    }
    else
    {
        std::istringstream       stream(high_level_source);
        std::string              line;
        std::vector<std::string> lines;

        // Store all lines for processing.
        while (std::getline(stream, line))
        {
            lines.push_back(line);
        }

        // Add first 10 lines.
        size_t line_index = 0;
        for (; line_index < std::min(kMaxInitialSourceLines, lines.size()); ++line_index)
        {
            nlohmann::json source_line_json;
            source_line_json[KJsonElemSourceLine] = lines[line_index];
            out_json.push_back(source_line_json);
        }

        if (lines.size() > kMaxInitialSourceLines)
        {
            // Find entry point line.
            size_t entry_line = 0;
            for (size_t j = 0; j < lines.size(); ++j)
            {
                if (lines[j].find(entry_point_name) != std::string::npos &&
                    (lines[j].find("(") != std::string::npos || (j + 1 < lines.size() && lines[j + 1].find("(") != std::string::npos)))
                {
                    entry_line = j;
                    break;
                }
            }

            // If entry point found, add context lines around it.
            if (entry_line > 0)
            {
                // Calculate start_line ensuring we don't show already shown lines.
                size_t start_line = (entry_line > kSourceContextLines) ? entry_line - kSourceContextLines : 0;

                // Adjust start_line to avoid overlap with already shown lines.
                if (start_line < kMaxInitialSourceLines)
                {
                    start_line = kMaxInitialSourceLines;
                }

                // Add lines_hidden count if there's a gap between what we've shown and what we'll show next.
                if (start_line > kMaxInitialSourceLines)
                {
                    nlohmann::json hidden_json;
                    hidden_json[kJsonElemLinesHidden] = start_line - kMaxInitialSourceLines;
                    out_json.push_back(hidden_json);
                }

                size_t end_line = std::min(entry_line + kSourceContextLines, lines.size() - 1);

                // Add the context lines if there are any new lines to add.
                if (start_line <= end_line)
                {
                    for (line_index = start_line; line_index <= end_line; ++line_index)
                    {
                        nlohmann::json source_line_json;
                        source_line_json[KJsonElemSourceLine] = lines[line_index];
                        out_json.push_back(source_line_json);
                    }
                }

                // Add trailing lines_hidden count if we haven't reached the end.
                if (end_line < lines.size() - 1)
                {
                    nlohmann::json hidden_json;
                    hidden_json[kJsonElemLinesHidden] = lines.size() - end_line - 1;
                    out_json.push_back(hidden_json);
                }
            }
        }
    }
}

// Helper function to display truncated source with context around entry point.
static void DisplayTruncatedSource(std::stringstream& out, const std::string& source, const std::string& entry_name, const std::string& source_file_name)
{
    std::istringstream stream(source);
    std::string line;
    std::vector<std::string> lines;

    constexpr const char* kFullSourceMsg = "// Run rgd with --full-source to include the complete source code of";
    
    // Store all lines for processing
    while (std::getline(stream, line))
    {
        lines.push_back(line);
    }
    
    // Display first 10 lines
    const size_t kInitialLines = 10;
    size_t i = 0;
    for (; i < std::min(kInitialLines, lines.size()); ++i)
    {
        out << lines[i] << "\n";
    }
    
    if (lines.size() > kInitialLines)
    {
        // Find entry point line
        size_t entry_line = 0;
        for (size_t j = 0; j < lines.size(); ++j)
        {
            if (lines[j].find(entry_name) != std::string::npos && 
                (lines[j].find("(") != std::string::npos || 
                 (j+1 < lines.size() && lines[j+1].find("(") != std::string::npos)))
            {
                entry_line = j;
                break;
            }
        }
        
        // If entry point found, display context lines around it
        if (entry_line > 0)
        {
            // Calculate start_line ensuring we don't print already shown lines
            size_t start_line = (entry_line > kSourceContextLines) ? entry_line - kSourceContextLines : 0;
            
            // Adjust start_line to avoid overlap with already printed lines
            if (start_line < kInitialLines)
            {
                start_line = kInitialLines;
            }
            
            // Only add ellipsis if there's a gap between what we've shown and what we'll show next
            if (start_line > kInitialLines)
            {
                PrintVerticalEllipsisWithMessage(std::string(kFullSourceMsg) + " " + source_file_name + ".", out);
            }
            
            size_t end_line = std::min(entry_line + kSourceContextLines, lines.size() - 1);
            
            // Only print lines if there are any new lines to print
            if (start_line <= end_line)
            {
                for (i = start_line; i <= end_line; ++i)
                {
                    out << lines[i] << "\n";
                }
            }
            
            // Add trailing ellipsis if we haven't reached the end
            if (end_line < lines.size() - 1)
            {
                out << ".\n.\n.\n";
            }
        }
    }
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

class RgdEnhancedCrashInfoSerializer::Impl
{
public:
    /// @brief Constructor.
    Impl() {}

    /// @brief Destructor.
    ~Impl() {}

    /// @brief Build the crashing code object info map.
    /// 
    /// The code object database holds the information of the crashing code objects.
    bool BuildCrashingCodeObjectDatabase(const Config& user_config, RgdCrashDumpContents& rgd_crash_dump_contents);

    /// @brief Build the output text for the shader info section. 
    bool BuildInFlightShaderInfo(const Config& user_config, std::string& out_text);

    /// @brief Build the output JSON for the shader info section.
    bool BuildInFlightShaderInfoJson(const Config& user_config, nlohmann::json& out_json);

    /// @brief Get the map of API PSO hashes and related crashing shader info for the shaders that were in flight at the time of the crash.
    std::unordered_map<uint64_t, RgdCrashingShaderInfo> GetInFlightShaderApiHashes();

    /// @brief Set the crash type - page fault or hang.
    void SetIsPageFault(bool is_page_fault);

    /// @brief Set the trace API type.
    void SetTraceApiType(TraceApiType trace_api_type);

    /// @brief Allocate the code object database.
    void AllocateRgdCodeObjectDatabase();

    /// @brief Get the complete disassembly of the shaders that were in flight at the time of the crash.
    bool GetCompleteDisassembly(const Config& user_config, std::string& out_text);

    /// @brief Get the list of debug info files for the shaders that were in flight at the time of the crash.
    std::vector<std::string> GetDebugInfoFiles() const;

private:
    /// @brief Get the code object hash for the given program counter.
    uint64_t GetCodeObjectBaseAddressForPC(uint64_t program_counter);

    /// @brief Get the Program Counter from the wave registers info map.
    bool BuildProgramCountersMap();

    /// @brief Build the enhanced crash info register context.
    bool BuildEnhancedCrashInfoRegisterContext(const RgdCrashDumpContents& rgd_crash_dump_contents);

    /// @brief Build the base address and code object hash map.
    bool BuildBaseAddressCodeObjectHashMap(const std::vector<RgdCodeObjectLoadEvent>& code_object_load_events);

    /// @brief Build the set of api pso hashes (in_flight_shader_api_pso_hashes_) for the shaders that were in-flight at the time of the crash.
    void BuildInFlightShaderApiPsoHashesAndCrashingShaderInfoMap();

    /// @brief Get the API PSO hash from the PSO correlations chunk.
    uint64_t GetApiPsoHashFromPsoCorrelationsChunk(const Rgd128bitHash& internal_pipeline_hash, const std::vector<RgdPsoCorrelation>& pso_correlations);

    /// @brief Post process the disassembly text for the base address.
    void PostProcessDisassemblyText(const std::map<uint64_t, size_t>&              pc_offset_to_hung_wave_count_map_,
                                    std::vector<std::pair<uint64_t, std::string>>& instructions,
                                    std::string&                                   out_disassembly_text);

    void GetDisassemblyJson(const std::map<uint64_t, size_t>&              pc_offset_to_hung_wave_count_map_,
                            std::vector<std::pair<uint64_t, std::string>>& instructions,
                            nlohmann::json&                                out_disassembly_json);

    /// @brief Calculate the instruction ranges around the program counter to print the disassembly text.
    void CalculateInstructionRangesToPrint(const std::map<uint64_t, size_t>&                    pc_offset_to_hung_wave_count_map_,
                                           std::vector<std::pair<uint64_t, std::string>>& instructions,
                                           std::vector<std::pair<size_t, size_t>>&              out_instruction_ranges,
                                           bool is_annotate = true);

    /// @brief  In the disassembly text, annotate the instruction/s which caused the crash in case of a page fault or the last instruction executed for the wave in case it was a hang.
    void AnnotateCrashingInstruction(size_t             pc_wave_count,
                                     size_t             max_instruction_disassembly_len,
                                     const std::string& crashing_instr_disassembly,
                                     std::string&       out_instruction_disassembly);

    /// @brief Get the page fault status.
    bool GetIsPageFault() const;

    /// @brief Get the trace API type.
    TraceApiType GetTraceApiType() const;

    // @brief Get the debug info support status.
    bool GetIsDebugInfoSupported() const;

    // The map for base address and code object hash.
    std::map<uint64_t, Rgd128bitHash> base_address_code_object_hash_map_;

    // Wave info registers map.
    std::unordered_map<uint32_t, WaveInfoRegisters> wave_info_registers_map_;

    // The map of in-flight shader API PSO hashes to their corresponding shader info.
    std::unordered_map<uint64_t, RgdCrashingShaderInfo> in_flight_shader_api_pso_hashes_to_shader_info_;

    // Map of the program counters and the count of waves that have the same program counter.
    std::map<uint64_t, size_t> program_counters_map_;

    /// @brief The list of debug info files for the shaders which are in flight at the time of the crash.
    std::vector<std::string> debug_info_files_;

    // Rgd Code Object Database of the crashing code objects.
    std::unique_ptr<RgdCodeObjectDatabase> rgd_code_object_database_ = nullptr;

    // Is the crash due to a page fault or a hang.
    bool is_page_fault_{false};

    // The trace API type.
    TraceApiType trace_api_{0};
};

RgdEnhancedCrashInfoSerializer::RgdEnhancedCrashInfoSerializer()
{
    enhanced_crash_info_serializer_impl_ = std::make_unique<Impl>();
    enhanced_crash_info_serializer_impl_->AllocateRgdCodeObjectDatabase();
}

RgdEnhancedCrashInfoSerializer::~RgdEnhancedCrashInfoSerializer() {}

bool RgdEnhancedCrashInfoSerializer::Initialize(const Config& user_config, RgdCrashDumpContents& rgd_crash_dump_contents, bool is_page_fault)
{
    bool ret = true;

    // Set the crash type - page fault or hang.
    enhanced_crash_info_serializer_impl_->SetIsPageFault(is_page_fault);
    
    // Set the API type.
    enhanced_crash_info_serializer_impl_->SetTraceApiType(rgd_crash_dump_contents.api_info.apiType);

    if (rgd_crash_dump_contents.rgd_extended_info.is_hca_enabled)
    {
        // Hardware crash analysis feature is enabled by the user.
        // Build the crashing code object info map.
        ret = enhanced_crash_info_serializer_impl_->BuildCrashingCodeObjectDatabase(user_config, rgd_crash_dump_contents);
    }
    else
    {
        // When enhanced crash analysis option is not enabled by the user, Wave registers info is not available.
        // The RgdEnhancedCrashInfoSerializer is not able to build the crashing code object info map.
        // The class instance is initialized to a default state.
        RgdUtils::PrintMessage("Hardware crash analysis feature is not enabled.", RgdMessageType::kInfo, user_config.is_verbose);
    }
    
    return ret;
}

void RgdEnhancedCrashInfoSerializer::Impl::AllocateRgdCodeObjectDatabase()
{
    rgd_code_object_database_ = std::make_unique<RgdCodeObjectDatabase>();
}

bool RgdEnhancedCrashInfoSerializer::GetInFlightShaderApiPsoHashes(
    std::unordered_map<uint64_t, RgdCrashingShaderInfo>& in_flight_shader_api_pso_hashes_to_shader_info_)
{
    in_flight_shader_api_pso_hashes_to_shader_info_ = enhanced_crash_info_serializer_impl_->GetInFlightShaderApiHashes();
    return !in_flight_shader_api_pso_hashes_to_shader_info_.empty();
}

bool RgdEnhancedCrashInfoSerializer::GetCompleteDisassembly(const Config& user_config, std::string& out_text)
{
    return enhanced_crash_info_serializer_impl_->GetCompleteDisassembly(user_config, out_text);
}

std::vector<std::string> RgdEnhancedCrashInfoSerializer::GetDebugInfoFiles() const
{
    return enhanced_crash_info_serializer_impl_->GetDebugInfoFiles();
}

std::vector<std::string> RgdEnhancedCrashInfoSerializer::Impl::GetDebugInfoFiles() const
{
    return debug_info_files_;
}

void RgdEnhancedCrashInfoSerializer::Impl::BuildInFlightShaderApiPsoHashesAndCrashingShaderInfoMap()
{
    in_flight_shader_api_pso_hashes_to_shader_info_.clear();
    for (RgdCodeObjectEntry& entry : rgd_code_object_database_->entries_)
    {
        uint64_t crashing_code_object_api_pso_hash = entry.api_pso_hash_;
        for (auto& shader_info_pair : entry.hw_stage_to_shader_info_map_)
        {
            const RgdShaderInfo& shader_info = shader_info_pair.second;
            if (shader_info.is_in_flight_shader)
            {
                if (in_flight_shader_api_pso_hashes_to_shader_info_.find(crashing_code_object_api_pso_hash) ==
                    in_flight_shader_api_pso_hashes_to_shader_info_.end())
                {
                    RgdCrashingShaderInfo crashing_shader_info;
                    crashing_shader_info.crashing_shader_ids.push_back(shader_info.str_shader_info_id);
                    crashing_shader_info.api_stages.push_back(shader_info.api_stage);

                    // Source file name and entry point name are optional fields and present only when valid PDB path is provided and matching PDB files are found.
                    if (shader_info.has_debug_info && !shader_info.source_file_name.empty() && !shader_info.entry_point_name.empty())
                    {
                        crashing_shader_info.source_file_names.push_back(shader_info.source_file_name);
                        crashing_shader_info.source_entry_point_names.push_back(shader_info.entry_point_name);
                    }
                    in_flight_shader_api_pso_hashes_to_shader_info_[crashing_code_object_api_pso_hash] = std::move(crashing_shader_info);
                }
                else
                {
                    RgdCrashingShaderInfo& crashing_shader_info = in_flight_shader_api_pso_hashes_to_shader_info_[crashing_code_object_api_pso_hash];
                    crashing_shader_info.crashing_shader_ids.push_back(shader_info.str_shader_info_id);
                    crashing_shader_info.api_stages.push_back(shader_info.api_stage);

                    // Source file name and entry point name are optional fields and present only when valid PDB path is provided and matching PDB files are found.
                    if (shader_info.has_debug_info && !shader_info.source_file_name.empty() && !shader_info.entry_point_name.empty())
                    {
                        crashing_shader_info.source_file_names.push_back(shader_info.source_file_name);
                        crashing_shader_info.source_entry_point_names.push_back(shader_info.entry_point_name);
                    }
                }
            }
        }
    }
}

uint64_t RgdEnhancedCrashInfoSerializer::Impl::GetApiPsoHashFromPsoCorrelationsChunk(
    const Rgd128bitHash&                         internal_pipeline_hash,
    const std::vector<RgdPsoCorrelation>& pso_correlations)
{
    uint64_t api_pso_hash = 0;

    assert(!pso_correlations.empty());
    for (const RgdPsoCorrelation& pso_correlation : pso_correlations)
    {
        if (pso_correlation.internal_pipeline_hash == internal_pipeline_hash)
        {
            api_pso_hash = pso_correlation.api_pso_hash;
            break;
        }
    }

    return api_pso_hash;
}

std::unordered_map<uint64_t, RgdCrashingShaderInfo> RgdEnhancedCrashInfoSerializer::Impl::GetInFlightShaderApiHashes()
{
    return in_flight_shader_api_pso_hashes_to_shader_info_;
}

bool RgdEnhancedCrashInfoSerializer::GetInFlightShaderInfo(const Config&         user_config,
                                                                 std::string& out_text)
{
    // Build the output text for the code object disassembly section.
    return enhanced_crash_info_serializer_impl_->BuildInFlightShaderInfo(user_config, out_text);;
}

bool RgdEnhancedCrashInfoSerializer::GetInFlightShaderInfoJson(const Config& user_config, nlohmann::json& out_json)
{
    // Build the output JSON for the SHADER INFO section.
    return enhanced_crash_info_serializer_impl_->BuildInFlightShaderInfoJson(user_config, out_json);
}

bool RgdEnhancedCrashInfoSerializer::Impl::BuildCrashingCodeObjectDatabase(const Config& user_config, RgdCrashDumpContents& rgd_crash_dump_contents)
{
    bool ret = false;

    // Build enhanced crash info register context for relevant asic architecture (RDNA) and parse the required registers info.
    ret = BuildEnhancedCrashInfoRegisterContext(rgd_crash_dump_contents);

    assert(ret);
    if (ret)
    {
        // Build the map of base address and hash of the most recent code object loaded at the base address.
        ret = BuildBaseAddressCodeObjectHashMap(rgd_crash_dump_contents.code_object_load_events);

        assert(ret);
        if (ret)
        {
            // Build the map of all the program counter values consolidated from all the wave registers info events and map them to the count of total waves with the respective pc value.
            // The program counter values are captured at the time of the crash and they represent the next instruction to be executed by the wave.
            // The instruction before the instruction pointed by the program counter is the one that caused the crash.
            ret = BuildProgramCountersMap();

            assert(ret);
            for (const std::pair<uint64_t, size_t>& program_counter_pair : program_counters_map_)
            {
                const uint64_t program_counter = program_counter_pair.first;
                const size_t   pc_wave_count   = program_counter_pair.second;

                // Get the base address of the code object where the instruction pointed by this program counter value resides.
                uint64_t base_address = GetCodeObjectBaseAddressForPC(program_counter);

                // Get the code object hash for the base address.
                assert(base_address_code_object_hash_map_.find(base_address) != base_address_code_object_hash_map_.end());
                if (base_address_code_object_hash_map_.find(base_address) != base_address_code_object_hash_map_.end())
                {
                    
                    Rgd128bitHash code_object_hash = base_address_code_object_hash_map_[base_address];
                    
                    if (rgd_crash_dump_contents.code_objects_map.find(code_object_hash) != rgd_crash_dump_contents.code_objects_map.end())
                    {
                        uint64_t    api_pso_hash          = GetApiPsoHashFromPsoCorrelationsChunk(code_object_hash, rgd_crash_dump_contents.pso_correlations);
                        uint64_t    pc_instruction_offset = (program_counter | kAddressHighWordOne) - base_address;
                        CodeObject& code_object           = rgd_crash_dump_contents.code_objects_map.at(code_object_hash);
                        rgd_code_object_database_->AddCodeObject(pc_instruction_offset, api_pso_hash, pc_wave_count, code_object_hash, std::move(code_object.chunk_payload));
                    }
                    else
                    {
                        assert(false);
                        RgdUtils::PrintMessage("code object not found in the code objects map.", RgdMessageType::kError, true);
                    }
                }
                else
                {
                    assert(false);
                    RgdUtils::PrintMessage("base address not found in the code object hash map.", RgdMessageType::kError, true);
                }
            }

            ret = rgd_code_object_database_->entries_.size() != 0;
            if (ret)
            {
                // Populate the code object database entries with additional information.
                rgd_code_object_database_->Populate();

                // Extract the debug info if the pdb files are available.
                // Collect the pdb search paths from the user config and the crash dump contents.
                // Prioritize the user config paths over the crash dump contents paths.
                std::vector<std::string> pdb_search_paths = user_config.pdb_dir;

                if (rgd_crash_dump_contents.rgd_extended_info.pdb_search_paths.size() > 0)
                {
                    for (const std::string& pdb_dir : rgd_crash_dump_contents.rgd_extended_info.pdb_search_paths)
                    {
                        if (std::find(pdb_search_paths.begin(), pdb_search_paths.end(), pdb_dir) == pdb_search_paths.end())
                        {
                            pdb_search_paths.push_back(pdb_dir);
                        }
                    }
                }

                if (!pdb_search_paths.empty())
                {
                    // Update the in-flight shader info with the debug info extracted from the PDB files.
                    std::cout << "PDB search path found. Extracting debug info for in-flight shaders..." << std::endl;
                    
                    if (rgd_code_object_database_->ExtractDebugInfo(user_config, pdb_search_paths))
                    {
                        std::cout << "Debug info extracted successfully." << std::endl;
                    }
                    else
                    {
                        RgdUtils::PrintMessage("failed to extract debug info.", RgdMessageType::kError, true);
                    }
                }

                // Build the set of api pso hashes for the shaders that were in-flight at the time of the crash.
                // The api pso hashes are used to correlate the set of user/driver markers with the crashing shaders.
                BuildInFlightShaderApiPsoHashesAndCrashingShaderInfoMap();
            }
        }
        else
        {
            RgdUtils::PrintMessage("failed to build base address and code object hash map.", RgdMessageType::kError, true);
        }
    }

    return ret;
}

bool RgdEnhancedCrashInfoSerializer::Impl::BuildBaseAddressCodeObjectHashMap(
    const std::vector<RgdCodeObjectLoadEvent>& code_object_load_events)
{
    base_address_code_object_hash_map_.clear(); 

    for (const auto& code_object_load_event : code_object_load_events)
    {
        // Build the map of base address and hash of the most recent code object loaded at the base address.
        base_address_code_object_hash_map_[code_object_load_event.base_address] = code_object_load_event.code_object_hash;
    }

    return !base_address_code_object_hash_map_.empty();
}

bool RgdEnhancedCrashInfoSerializer::Impl::BuildProgramCountersMap()
{
    program_counters_map_.clear();

    for (const auto& pair : wave_info_registers_map_)
    {
        const WaveInfoRegisters& wave_info_registers = pair.second;
        uint64_t                 program_counter = wave_info_registers.sq_wave_pc_lo;
        if (program_counters_map_.find(program_counter) == program_counters_map_.end())
        {
            program_counters_map_[program_counter] = 1;
        }
        else
        {
            program_counters_map_[program_counter]++;
        }
    }

    return !program_counters_map_.empty();
}

// Build enhanced crash info register context and parse the required registers info.
bool RgdEnhancedCrashInfoSerializer::Impl::BuildEnhancedCrashInfoRegisterContext(const RgdCrashDumpContents& rgd_crash_dump_contents)
{
    bool ret = false;
    
    std::unique_ptr<ICrashInfoRegistersParser> crash_info_registers_parser = CrashInfoRegistersParserFactory::CreateParser(rgd_crash_dump_contents.gpu_series);

    assert(crash_info_registers_parser != nullptr);
    if (crash_info_registers_parser != nullptr)
    {
        CrashInfoRegistersParserContext crash_info_registers_context(std::move(crash_info_registers_parser));

        // Parse wave info registers.
        ret = crash_info_registers_context.ParseWaveInfoRegisters(rgd_crash_dump_contents.kmd_crash_data, wave_info_registers_map_);
    }
    if (!ret)
    {
        RgdUtils::PrintMessage("failed to parse wave info registers.", RgdMessageType::kError, true);
    }
    return ret;
}

uint64_t RgdEnhancedCrashInfoSerializer::Impl::GetCodeObjectBaseAddressForPC(uint64_t program_counter)
{
    uint64_t ret_base_address = 0;
    uint64_t           address             = (program_counter | kAddressHighWordOne);

    auto               it                  = base_address_code_object_hash_map_.upper_bound(address);

    if (it != base_address_code_object_hash_map_.end())
    {
        assert(it != base_address_code_object_hash_map_.begin());
        if (it!= base_address_code_object_hash_map_.begin())
        {
            it--;
            ret_base_address = it->first;
        }
    }
    else
    {
        if (!base_address_code_object_hash_map_.empty())
        {
            // The address is larger than the base address of the last code object.
            ret_base_address = base_address_code_object_hash_map_.rbegin()->first;
        }
    }

    if (ret_base_address == 0)
    {
        RgdUtils::PrintMessage("no code object found for the program counter.", RgdMessageType::kError, true);
    }

    return ret_base_address;
}

bool RgdEnhancedCrashInfoSerializer::Impl::BuildInFlightShaderInfo(const Config& user_config, std::string& out_text)
{
    out_text = "";
    std::stringstream txt;

    constexpr const char* kMsgRequiresDebugInfo =
        " (requires debug information, recompile shader with '-Zi -Qembed_debug', '-Zi -Qsource_in_debug_module' or '-Zs')";

    for (RgdCodeObjectEntry& entry : rgd_code_object_database_->entries_)
    {
        for (auto& hw_stage_shader_info_pair : entry.hw_stage_to_shader_info_map_)
        {
            RgdShaderInfo& shader_info = hw_stage_shader_info_pair.second;
            if (shader_info.is_in_flight_shader)
            {
                if (!shader_info.pdb_file_path.empty())
                {
                    // Update the list of PDB files relevant to the shader which is in flight.
                    debug_info_files_.push_back(shader_info.pdb_file_path);
                }

                txt << "Shader info ID  : " << shader_info.str_shader_info_id << std::endl;
                txt << "API stage       : " << shader_info.api_stage << std::endl;
                txt << "API PSO hash    : 0x" << std::hex << entry.api_pso_hash_ << std::dec << std::endl;
                txt << "API shader hash : 0x" << std::hex << std::setw(16) << std::setfill('0') << shader_info.api_shader_hash_hi << std::setw(16)
                    << std::setfill('0') << shader_info.api_shader_hash_lo << " (high: 0x" << shader_info.api_shader_hash_hi << ", low: 0x"
                    << shader_info.api_shader_hash_lo << std::dec << ")" << std::endl;

                txt << "File name       : " << shader_info.source_file_name
                    << ((shader_info.source_file_name == kStrNotAvailable && GetIsDebugInfoSupported()) ? kMsgRequiresDebugInfo : "")
                    << std::endl;
                txt << "Entry point name: " << shader_info.entry_point_name
                    << ((shader_info.entry_point_name == kStrNotAvailable && GetIsDebugInfoSupported()) ? kMsgRequiresDebugInfo : "")
                    << std::endl;

                txt << std::endl;
                txt << "Shader IO and resource bindings" << std::endl;
                txt << "===============================" << std::endl;
                if (shader_info.shader_io_and_resource_bindings == kStrNotAvailable)
                {
                    txt << shader_info.shader_io_and_resource_bindings << (GetIsDebugInfoSupported() ? kMsgRequiresDebugInfo : "") << std::endl;
                }
                else
                {
                    txt << shader_info.shader_io_and_resource_bindings;
                }

                txt << std::endl;
                txt << "HLSL source code" << std::endl;
                txt << "================" << std::endl;

                if (shader_info.high_level_source == kStrNotAvailable)
                {
                    txt << shader_info.high_level_source << (GetIsDebugInfoSupported() ? kMsgRequiresDebugInfo : "") << std::endl;
                }
                else
                {
                    // Check if we should display full source or apply truncation rules.
                    const bool show_full_source = user_config.is_full_source || CountSourceLines(shader_info.high_level_source) <= kMaxLinesToShowFullSource ||
                                                  shader_info.entry_point_name.empty();
                    if (show_full_source)
                    {
                        txt << shader_info.high_level_source;
                    }
                    else
                    {
                        DisplayTruncatedSource(txt, shader_info.high_level_source, shader_info.entry_point_name, shader_info.source_file_name);
                    }
                }

                txt << std::endl;
                txt << "Disassembly" << std::endl;
                txt << "===========" << std::endl;

                std::string out_shader_disassembly_text;
                PostProcessDisassemblyText(entry.pc_offset_to_hung_wave_count_map_, shader_info.instructions, out_shader_disassembly_text);

                txt << out_shader_disassembly_text << std::endl;
            }
        }
    }
    out_text = txt.str();
    return out_text.empty();
}

bool RgdEnhancedCrashInfoSerializer::Impl::BuildInFlightShaderInfoJson(const Config& user_config, nlohmann::json& out_json)
{
    out_json[kJsonElemShaderInfo] = nlohmann::json::array();

    for (RgdCodeObjectEntry& entry : rgd_code_object_database_->entries_)
    {
        for (auto& hw_stage_shader_info_pair : entry.hw_stage_to_shader_info_map_)
        {
            RgdShaderInfo& shader_info = hw_stage_shader_info_pair.second;
            if (shader_info.is_in_flight_shader)
            {
                try
                {
                    nlohmann::json shader_info_json;
                    shader_info_json[kJsonElemShaderInfoId]    = shader_info.str_shader_info_id;
                    shader_info_json[kJsonElemApiPsoHash]      = entry.api_pso_hash_;
                    shader_info_json[kJsonElemApiShaderHashHi] = shader_info.api_shader_hash_hi;
                    shader_info_json[kJsonElemApiShaderHashLo] = shader_info.api_shader_hash_lo;
                    shader_info_json[kJsonElemApiStage]        = shader_info.api_stage;
                    shader_info_json[kJsonElemSourceFileName]  = shader_info.source_file_name;
                    shader_info_json[kJsonElemEntryPointName]  = shader_info.entry_point_name;

                    std::istringstream shader_io_stream(shader_info.shader_io_and_resource_bindings);
                    std::string        shader_io_line;
                    shader_info_json[kJsonElemShaderIoAndResourceBindings] = nlohmann::json::array();
                    while (std::getline(shader_io_stream, shader_io_line))
                    {
                        shader_info_json[kJsonElemShaderIoAndResourceBindings].push_back(shader_io_line);
                    }
                    
                    // Build source json (truncated if necessary) with context around entry point.
                    BuildHighLevelSourceJson(shader_info.high_level_source, shader_info.entry_point_name, user_config.is_full_source, shader_info_json[kJsonElemSourceCode]);

                    nlohmann::json disassembly_json;
                    GetDisassemblyJson(entry.pc_offset_to_hung_wave_count_map_, shader_info.instructions_json_output, disassembly_json);
                    shader_info_json[kJsonElemDisassembly] = disassembly_json;

                    out_json[kJsonElemShaderInfo].push_back(std::move(shader_info_json));
                }
                catch (nlohmann::json::exception& e)
                {
                    RgdUtils::PrintMessage(e.what(), RgdMessageType::kError, true);
                }
            }
        }
    }
    return !out_json[kJsonElemShaderInfo].empty();
}
void RgdEnhancedCrashInfoSerializer::Impl::PostProcessDisassemblyText(
    const std::map<uint64_t, size_t>&              pc_offset_to_hung_wave_count_map_,
    std::vector<std::pair<uint64_t, std::string>>& instructions,
    std::string&                                   out_disassembly_text)
{
    assert(!instructions.empty());
    if (!instructions.empty())
    {
        std::vector<std::pair<size_t, size_t>> instruction_ranges;
        CalculateInstructionRangesToPrint(pc_offset_to_hung_wave_count_map_, instructions, instruction_ranges);
        assert(!instruction_ranges.empty());
        if (!instruction_ranges.empty())
        {
            std::stringstream disassembly_stream;
            for (size_t range_pair_idx = 0; range_pair_idx < instruction_ranges.size(); range_pair_idx++)
            {
                const auto& instruction_range = instruction_ranges[range_pair_idx];
                if (range_pair_idx != 0 || (range_pair_idx == 0 && instruction_range.first > 0))
                {
                    // Print the continuation dots when there are multiple ranges or the first range does not start at the beginning.
                    disassembly_stream << '\t' << '.' << std::endl;
                    disassembly_stream << '\t' << '.' << std::endl;
                    disassembly_stream << '\t' << '.' << std::endl;
                }

                for (size_t i = instruction_range.first; i <= instruction_range.second; i++)
                {
                    disassembly_stream << instructions[i].second << std::endl;
                }

                if (range_pair_idx == (instruction_ranges.size() - 1) && instruction_range.second != (instructions.size() - 1))
                {
                    // Print the continuation dots when the last range does not end at the end.
                    disassembly_stream << '\t' << '.' << std::endl;
                    disassembly_stream << '\t' << '.' << std::endl;
                    disassembly_stream << '\t' << '.' << std::endl;
                }
            }
            out_disassembly_text = disassembly_stream.str();
        }
        else
        {
            RgdUtils::PrintMessage("no instruction ranges found to print.", RgdMessageType::kError, true);
            out_disassembly_text.clear();
        }
    }
    else
    {
        RgdUtils::PrintMessage("invalid disassembly text.", RgdMessageType::kError, true);
        out_disassembly_text.clear();
    }
}

void RgdEnhancedCrashInfoSerializer::Impl::GetDisassemblyJson(const std::map<uint64_t, size_t>& pc_offset_to_hung_wave_count_map_,
                                                                                          std::vector<std::pair<uint64_t, std::string>>& instructions,
                                                                                          nlohmann::json&                                out_disassembly_json)
{
    assert(!instructions.empty());
    if (!instructions.empty())
    {
        std::vector<std::pair<size_t, size_t>> instruction_ranges;

        // Do not annotate the page fault suspect instruction/s for the JSON output.
        bool                                   is_annotate = false;
        CalculateInstructionRangesToPrint(pc_offset_to_hung_wave_count_map_, instructions, instruction_ranges, is_annotate);
        assert(!instruction_ranges.empty());
        if (!instruction_ranges.empty())
        {
            try
            {
                out_disassembly_json[kJsonElemInstructionsDisassembly] = nlohmann::json::array();
                for (size_t range_pair_idx = 0; range_pair_idx < instruction_ranges.size(); range_pair_idx++)
                {
                    const auto& instruction_range = instruction_ranges[range_pair_idx];
                    
                    if (range_pair_idx == 0 && instruction_range.first > 0)
                    {
                        // Add leading json object for hidden instructions.
                        nlohmann::json instruction_json;
                        instruction_json[kJsonElemInstructionsHidden] = instruction_range.first;
                        out_disassembly_json[kJsonElemInstructionsDisassembly].push_back(std::move(instruction_json));
                    }
                    else
                    {
                        // Add middle json object for hidden instructions.
                        const auto&    instruction_range_pre = instruction_ranges[range_pair_idx - 1];
                        nlohmann::json instruction_json;
                        instruction_json[kJsonElemInstructionsHidden] = instruction_range.first - instruction_range_pre.second;
                        out_disassembly_json[kJsonElemInstructionsDisassembly].push_back(std::move(instruction_json));
                    }

                    for (size_t i = instruction_range.first; i <= instruction_range.second; i++)
                    {
                        nlohmann::json instruction_json;
                        instruction_json[kJsonElemInstr] = instructions[i].second;
                        size_t wave_count                                 = 0;

                        // Get the wave count for current instruction.
                        // For a page fault case, wave count represents the number of waves that executed the instruction.
                        if (GetIsPageFault())
                        {
                            // The program counter value is the address of the next instruction to be executed by the wave.
                            // So for the page fault, on instruction before the program counter is the one that caused the crash.
                            // Wave count represents the number waves having the same program counter value.
                            // For the page fault, attrubute the wave count from next instruction indicating number waves already executed current instruction.
                            
                            if (i + 1 < instructions.size() &&
                                pc_offset_to_hung_wave_count_map_.find(instructions[i + 1].first) != pc_offset_to_hung_wave_count_map_.end())
                            {
                                wave_count = pc_offset_to_hung_wave_count_map_.at(instructions[i + 1].first);
                            }
                        }
                        else
                        {
                            // For the hang case, wave count represents number of waves that were going to execute the instruction.
                            if (pc_offset_to_hung_wave_count_map_.find(instructions[i].first) != pc_offset_to_hung_wave_count_map_.end())
                            {
                                wave_count = pc_offset_to_hung_wave_count_map_.at(instructions[i].first);
                            }
                        }
                        if (wave_count > 0)
                        {
                            instruction_json[kJsonElemWaveCount] = wave_count;
                        }

                        out_disassembly_json[kJsonElemInstructionsDisassembly].push_back(std::move(instruction_json));
                    }

                    if (range_pair_idx == (instruction_ranges.size() - 1) && instruction_range.second < (instructions.size() - 1))
                    {
                        // Add trailing json object for hidden instructions.
                        nlohmann::json instruction_json;
                        instruction_json[kJsonElemInstructionsHidden] = instructions.size() - instruction_range.second - 1;
                        out_disassembly_json[kJsonElemInstructionsDisassembly].push_back(std::move(instruction_json));
                    }
                }
            }
            catch (nlohmann::json::exception& e)
            {
                RgdUtils::PrintMessage(e.what(), RgdMessageType::kError, true);
            }
        }
    }
}

void RgdEnhancedCrashInfoSerializer::Impl::CalculateInstructionRangesToPrint(
    const std::map<uint64_t, size_t>&                    pc_offset_to_hung_wave_count_map_,
    std::vector<std::pair<uint64_t, std::string>>& instructions,
    std::vector<std::pair<size_t, size_t>>&              out_instruction_ranges,
    bool is_annotate)
{
    constexpr int kInstructionRange               = 17;
    size_t        max_instruction_disassembly_len = GetMaxDisassemblyLenForInstructions(instructions);
    assert(max_instruction_disassembly_len != 0);

    for (const std::pair<uint64_t, size_t>& offset_wave_count_pair : pc_offset_to_hung_wave_count_map_)
    {
        // Get the instruction offset.
        uint64_t instruction_offset = offset_wave_count_pair.first;

        // Find the instruction pointed by the program counter.
        auto it = std::find_if(instructions.begin(), instructions.end(), [instruction_offset](const std::pair<uint64_t, std::string>& instr) {
            return instr.first == instruction_offset;
        });
        if (it != instructions.end())
        {
            if (GetIsPageFault() && it != instructions.begin())
            {
                // The program counter points to the next instruction to be executed by the wave.
                // In case of a page fault, since instruction before the program counter is the one that caused the crash, annotate previous instruction as page faulted.
                // Otherwise (in case of a hang), the instruction pointed by the program counter will be annotated with an arrow notation (<--).
                it--;
            }

            if (it->second[0] == '_')
            {
                // This is a branch label. The branch label has the same offset as the instruction that follows it. Point the instruction instead.
                it++;
            }

            // Get the index of the found instruction.
            size_t index = std::distance(instructions.begin(), it);

            // Annotation for the crashing instruction.
            std::string crashing_instr_disassembly = it->second;

            // Remove the new line character from the end of the instruction.
            if (!crashing_instr_disassembly.empty() && crashing_instr_disassembly.back() == '\n')
            {
                crashing_instr_disassembly.pop_back();
            }

            std::string out_instruction_disassembly;
            size_t      pc_wave_count = offset_wave_count_pair.second;
            
            assert(pc_wave_count != 0);

            // Annotate the instructions pointed by program counter for text output only.
            if (is_annotate)
            {
                AnnotateCrashingInstruction(pc_wave_count, max_instruction_disassembly_len, crashing_instr_disassembly, out_instruction_disassembly);

                // Replace the crashing instruction with the annotated instruction.
                instructions[index].second = out_instruction_disassembly;
            }
            else
            {
                instructions[index].second = crashing_instr_disassembly;
            }

            // Print kInstructionRange instructions around the found instruction.
            size_t start = (index > kInstructionRange) ? index - kInstructionRange : 0;
            size_t end   = std::min(index + kInstructionRange + 1, instructions.size() - 1);

            if (!out_instruction_ranges.empty() && out_instruction_ranges.back().second > start)
            {
                out_instruction_ranges.back().second = end;
            }
            else
            {
                out_instruction_ranges.emplace_back(start, end);
            }
        }
    }
}

void RgdEnhancedCrashInfoSerializer::Impl::AnnotateCrashingInstruction(size_t             pc_wave_count,
                                                                                                   size_t             max_instruction_disassembly_len,
                                                                                                   const std::string& crashing_instr_disassembly,
                                                                                                   std::string&       out_instruction_disassembly)
{
    constexpr const char* kInstructionAnnotationPrefix    = " <-- ***";
    constexpr const char* kPageFaultInstructionAnnotation = "PAGE FAULT SUSPECT";
    constexpr const char* kInstructionAnnotationSuffix    = "***";
    const char*           kPrintWaves                     = (pc_wave_count == 1 ? "wave" : "waves");
    std::stringstream disassembly_stream;
    assert(!crashing_instr_disassembly.empty());

    // Calculate the padding required to align the annotation prefix
    assert(max_instruction_disassembly_len >= crashing_instr_disassembly.size());
    size_t      padding_len = 1;
    if (max_instruction_disassembly_len >= crashing_instr_disassembly.size())
    {
        padding_len = 1 + max_instruction_disassembly_len - crashing_instr_disassembly.size();
    }
    std::string padding(padding_len, ' ');

    disassembly_stream << crashing_instr_disassembly << padding << kInstructionAnnotationPrefix;
    if (GetIsPageFault())
    {
        disassembly_stream << kPageFaultInstructionAnnotation;

        if (pc_wave_count != 0)
        {
            disassembly_stream << " (" << pc_wave_count << " " << kPrintWaves << ")";
        }
    }
    else
    {
        assert(pc_wave_count != 0);
        disassembly_stream << pc_wave_count << " " << kPrintWaves;
    }
    disassembly_stream << kInstructionAnnotationSuffix;

    out_instruction_disassembly = disassembly_stream.str();

    // Replace the space with '>' to indicate the crashing instruction.
    out_instruction_disassembly.replace(0, 1, ">");
}

void RgdEnhancedCrashInfoSerializer::Impl::SetIsPageFault(bool is_page_fault)
{
    is_page_fault_ = is_page_fault;
}

void RgdEnhancedCrashInfoSerializer::Impl::SetTraceApiType(TraceApiType trace_api_type)
{
    trace_api_ = trace_api_type;
}

TraceApiType RgdEnhancedCrashInfoSerializer::Impl::GetTraceApiType() const
{
    return trace_api_;
}

bool RgdEnhancedCrashInfoSerializer::Impl::GetIsDebugInfoSupported() const
{
    return GetTraceApiType() == TraceApiType::DIRECTX_12;
}

bool RgdEnhancedCrashInfoSerializer::Impl::GetIsPageFault() const
{
    return is_page_fault_;
}

bool RgdEnhancedCrashInfoSerializer::Impl::GetCompleteDisassembly(const Config& user_config, std::string& out_text)
{
    out_text = "";
    size_t            code_object_id = 1;
    std::stringstream txt;
    std::stringstream code_object_ids_txt;
    for (RgdCodeObjectEntry& entry : rgd_code_object_database_->entries_)
    {
        txt << "Code object ID   : " << RgdUtils::GetAlphaNumericId(kStrPrefixCodeObjectId, code_object_id++) << std::endl;
        txt << "API PSO hash     : 0x" << std::hex << entry.api_pso_hash_ << std::dec << std::endl;
        txt << "In flight shaders: ";
        for (auto it = entry.hw_stage_to_shader_info_map_.begin(); it != entry.hw_stage_to_shader_info_map_.end(); ++it)
        {
            RgdShaderInfo& shader_info = it->second;

            if (shader_info.is_in_flight_shader)
            {
                if (it != entry.hw_stage_to_shader_info_map_.begin())
                {
                    txt << std::endl << "                 : ";
                }

                txt << shader_info.api_stage << "(shader ID: " << std::hex << shader_info.str_shader_info_id << ", "
                    << "API Shader Hash: 0x" << shader_info.api_shader_hash_hi << shader_info.api_shader_hash_lo << " (high: 0x" << std::hex << shader_info.api_shader_hash_hi << ", low: 0x" << shader_info.api_shader_hash_lo << ")" << std::dec << ")";
            }
        }

        txt << std::endl << std::endl;
        txt << "Disassembly" << std::endl;
        txt << "===========" << std::endl;
        txt << entry.disassembly_ << std::endl << std::endl;
    }
    
    out_text = txt.str();
    return !out_text.empty();
}