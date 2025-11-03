//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief SRD instruction analyzer for detecting SGPR usage and SRD disassembly integration.
//============================================================================================

#ifndef RGD_SRD_INSTRUCTION_ANALYZER_H_
#define RGD_SRD_INSTRUCTION_ANALYZER_H_

// Local.
#include "rgd_data_types.h"
#include "rgd_srd_disassembler.h"

// ISA Decoder.
#include "amdisa/isa_decoder.h"

// Standard.
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <map>

// SRD analysis error message constants.
namespace SrdAnalysisErrors
{
    constexpr const char* kJsonErrorPrefix = "JSON error during SRD analysis generation: ";
    constexpr const char* kGeneralErrorPrefix = "unable to generate SRD analysis: ";
}

// SRD analysis json field name constants.
namespace SrdAnalysisJsonFields
{
    constexpr const char* kJsonElemWaveCoordinateId = "wave_coordinate_id";
}

/// @brief Analyzes shader instructions for SGPR usage and integrates SRD disassembly.
class SrdInstructionAnalyzer
{
public:
    /// @brief Structure to hold SGPR group information for SRD analysis.
    struct SgprGroup
    {
        std::vector<uint32_t> indices;  ///< SGPR register indices in this group.
        SrdType type;                   ///< Detected SRD type for this group.
    };

    /// @brief Constructor.
    SrdInstructionAnalyzer();

    /// @brief Destructor.
    ~SrdInstructionAnalyzer() = default;

    /// @brief Initialize the analyzer with crash dump contents.
    /// @param [in] crash_dump_contents The crash dump contents containing GPU series and SGPR data.
    /// @return True if initialization was successful.
    bool Initialize(const RgdCrashDumpContents& crash_dump_contents);

    /// @brief Initialize the SRD instruction analyzer with crash dump contents and GPR event index.
    /// @param crash_dump_contents The crash dump contents containing GPR data.
    /// @param gpr_event_indices Map of shader IDs to GPR event indices for efficient lookup.
    /// @return True if initialization succeeded, false otherwise.
    bool Initialize(const RgdCrashDumpContents& crash_dump_contents,
                   const std::map<uint32_t, std::pair<size_t, std::vector<size_t>>>& gpr_event_indices);

    /// @brief Analyze crashing instruction using pre-detected SGPRs and get the SRD analysis if available.
    /// @param [in] instruction_text The disassembled instruction text.
    /// @param [in] sgpr_groups Pre-detected SGPR groups to avoid redundant parsing.
    /// @param [in] shader_id The shader ID for the crashing wave.
    /// @return SRD disassembly text if SGPR registers contain SRD data, empty string otherwise.
    std::string GetSrdAnalysisForOffendingInstruction(const std::string& instruction_text,
                                                     const std::vector<SgprGroup>& sgpr_groups,
                                                     uint32_t shader_id) const;

    /// @brief Analyze crashing instruction using pre-detected SGPRs and get the SRD analysis JSON if available.
    /// @param [in] instruction_text The disassembled instruction text.
    /// @param [in] sgpr_groups Pre-detected SGPR groups to avoid redundant parsing.
    /// @param [in] shader_id The shader ID for the crashing wave.
    /// @return SRD disassembly JSON if SGPR registers contain SRD data, empty JSON otherwise.
    nlohmann::json GetSrdAnalysisForOffendingInstructionJson(const std::string& instruction_text,
                                                             const std::vector<SgprGroup>& sgpr_groups,
                                                             uint32_t shader_id) const;
    
    /// @brief Get a unique signature based on SGPR values for pre-detected SGPR groups.
    /// @param sgpr_groups Pre-detected SGPR groups from the instruction.
    /// @param shader_id The shader ID to get the SGPR values from.
    /// @return A string signature representing the SGPR values used in the instruction, or empty string if no SGPR data found.
    std::string GetSgprSignatureFromGroups(const std::vector<SgprGroup>& sgpr_groups, uint32_t shader_id) const;

    /// @brief Detect SGPR usage in an instruction.
    /// @param [in] instruction_text The disassembled instruction text.
    /// @param [out] sgpr_groups Vector of detected SGPR groups.
    /// @return True if SGPR usage is detected in the instruction.
    bool DetectSgprUsage(const std::string& instruction_text, std::vector<SgprGroup>& sgpr_groups) const;

private:

    /// @brief Determine the SRD type for a specific SGPR group based on ISA decoder analysis.
    /// @details This method uses the ISA decoder's instruction info to infer the SRD type for a group of SGPRs.
    ///          Following SRD types are supported: 'Image', 'Sampler', 'Buffer' and 'BVH'.
    /// @param [in] instruction_info Decoded instruction information from ISA decoder.
    /// @param [in] operand_index The index of the operand in the instruction.
    /// @param [in] group_index The index of this group (0-based).
    /// @param [in] start_reg The starting SGPR register index.
    /// @param [in] end_reg The ending SGPR register index.
    /// @param [in] has_rsrc_field True if the instruction has an RSRC encoding field.
    /// @param [in] has_samp_field True if the instruction has a SAMP encoding field.
    /// @return The detected SRD type for this specific group.
    SrdType DetermineSrdTypeForGroup(const amdisa::InstructionInfo& instruction_info, 
                                     size_t operand_index,
                                     size_t group_index, 
                                     uint32_t start_reg, 
                                     uint32_t end_reg,
                                     bool has_rsrc_field,
                                     bool has_samp_field) const;

    /// @brief Detect SGPR groups using ISA decoder for more accurate analysis.
    /// @param [in] instruction_text The disassembled instruction text.
    /// @param [in] machine_code Optional machine code for the instruction.
    /// @param [out] sgpr_groups Vector of detected SGPR groups.
    /// @return True if SGPR groups are detected in the instruction.
    bool DetectSgprGroupsUsingIsaDecoder(const std::string& instruction_text,
                                         const std::string& machine_code,
                                         std::vector<SgprGroup>& sgpr_groups) const;

    /// @brief Get ISA specification file path for the given GPU series.
    /// @param [in] gpu_series The GPU series to get the ISA spec for.
    /// @return Path to the ISA specification XML file, or empty string if not available.
    std::string GetIsaSpecPath(ecitrace::GpuSeries gpu_series) const;

    /// @brief Extract machine code from instruction text.
    /// @param [in] instruction_text The instruction text containing machine code after "//".
    /// @return The machine code string, or empty string if not found.
    std::string ExtractMachineCode(const std::string& instruction_text) const;

    /// @brief Parse machine code string into vector of DWORDs.
    /// @param [in] machine_code_str The machine code string with hex values.
    /// @return Vector of 32-bit machine code values.
    std::vector<uint32_t> ParseMachineCodeString(const std::string& machine_code_str) const;

    /// @brief Extract SGPR groups from instruction text using ISA decoder info.
    /// @param [in] instruction_text The disassembled instruction text.
    /// @param [in] instruction_info Decoded instruction information from ISA decoder.
    /// @param [out] sgpr_groups Vector of detected SGPR groups.
    /// @return True if SGPR groups are detected.
    bool ExtractSgprGroupsFromInstructionAndIsa(const std::string& instruction_text,
                                                const amdisa::InstructionInfo& instruction_info,
                                                std::vector<SgprGroup>& sgpr_groups) const;

    /// @brief Extract SRD data from SGPR registers for a specific shader.
    /// @param [in] sgpr_indices The SGPR register indices to extract.
    /// @param [in] shader_id The shader ID to look up SGPR data.
    /// @return Vector of 32-bit words representing the SRD data.
    std::vector<uint32_t> ExtractSrdData(const std::vector<uint32_t>& sgpr_indices, uint32_t shader_id) const;

    /// @brief SRD disassembler instance.
    std::unique_ptr<ISrdDisassembler> srd_disassembler_;

    /// @brief ISA decoder instance for analyzing instructions.
    std::unique_ptr<amdisa::IsaDecoder> isa_decoder_;

    /// @brief Pointer to crash dump contents for accessing SGPR data when needed.
    const RgdCrashDumpContents* crash_dump_contents_;

    /// @brief GPU series for hardware-specific SRD formats.
    ecitrace::GpuSeries gpu_series_;

    // Map of shader IDs to GPR event indices for efficient lookup.
    const std::map<uint32_t, std::pair<size_t, std::vector<size_t>>>* gpr_event_indices_;
};

#endif  // RGD_SRD_INSTRUCTION_ANALYZER_H_
