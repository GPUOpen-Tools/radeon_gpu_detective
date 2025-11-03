//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief SRD instruction analyzer implementation for detecting SGPR usage and SRD disassembly.
//============================================================================================

#include "rgd_srd_instruction_analyzer.h"

// Local.
#include "rgd_srd_disassembler_factory.h"
#include "rgd_utils.h"

// Standard.
#include <regex>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <filesystem>

SrdInstructionAnalyzer::SrdInstructionAnalyzer()
    : srd_disassembler_(nullptr)
    , isa_decoder_(nullptr)
    , crash_dump_contents_(nullptr)
    , gpu_series_(ecitrace::GpuSeries::kUnknown)
    , gpr_event_indices_(nullptr)
{
}

bool SrdInstructionAnalyzer::Initialize(const RgdCrashDumpContents& crash_dump_contents,
                                       const std::map<uint32_t, std::pair<size_t, std::vector<size_t>>>& gpr_event_indices)
{
    bool result = false;
    gpu_series_ = crash_dump_contents.gpu_series;
    
    // Create SRD disassembler for the detected GPU series.
    srd_disassembler_ = SrdDisassemblerFactory::CreateDisassembler(gpu_series_);
    if (srd_disassembler_ != nullptr)
    {
        // Store reference to crash dump contents for later SGPR data access.
        crash_dump_contents_ = &crash_dump_contents;
        
        // Store reference to GPR event indices for efficient lookup.
        gpr_event_indices_ = &gpr_event_indices;
        
        // Initialize ISA decoder for the GPU series.
        isa_decoder_ = std::make_unique<amdisa::IsaDecoder>();
        
        // Map GPU series to ISA spec XML file path.
        std::string isa_spec_path = GetIsaSpecPath(gpu_series_);
        if (!isa_spec_path.empty())
        {
            // Validate that the ISA spec file exists before trying to initialize.
            if (std::filesystem::exists(isa_spec_path))
            {
                std::string err_message;
                if (isa_decoder_->Initialize(isa_spec_path, err_message))
                {
                    result = true;
                }
                else
                {
                    // ISA decoder initialization failed - SRD analysis cannot work without it.
                    RgdUtils::PrintMessage("ISA decoder initialization failed. SRD analysis requires ISA decoder.", RgdMessageType::kError, true);
                    isa_decoder_.reset();
                    result = false;
                }
            }
            else
            {
                // ISA spec file not found - SRD analysis cannot work without it.
                RgdUtils::PrintMessage("ISA specification file not found. SRD analysis unavailable.", RgdMessageType::kError, true);
                isa_decoder_.reset();
                result = false;
            }
        }
        else
        {
            // No ISA spec available or SRD analysis is not supported for this GPU architecture - SRD analysis cannot work without it.
            RgdUtils::PrintMessage("No ISA specification available or SRD analysis is not supported for this GPU architecture. SRD analysis unavailable.", RgdMessageType::kWarning, true);
            isa_decoder_.reset();
            result = false;
        }
    }

    return result;
}

std::string SrdInstructionAnalyzer::GetIsaSpecPath(ecitrace::GpuSeries gpu_series) const
{
    std::string result = "";
    
    std::string spec_filename;
    switch (gpu_series)
    {
        case ecitrace::GpuSeries::kNavi3:
            spec_filename = "amdgpu_isa_rdna3.xml";
            break;
        case ecitrace::GpuSeries::kStrix1:
            spec_filename = "amdgpu_isa_rdna3_5.xml";
            break;
        case ecitrace::GpuSeries::kNavi4:
            spec_filename = "amdgpu_isa_rdna4.xml";
            break;
        default:
            // Empty string for unsupported architectures.
            break;
    }
    
    // Construct path using the same pattern as DXC executable.
    // ISA spec files are copied to utils/isa_spec/ during build.
    if (!spec_filename.empty())
    {
        result = ".\\utils\\isa_spec\\" + spec_filename;
    }
    
    return result;
}



std::string SrdInstructionAnalyzer::GetSrdAnalysisForOffendingInstruction(const std::string& instruction_text,
                                                                          const std::vector<SgprGroup>& sgpr_groups,
                                                                          uint32_t shader_id) const
{
    std::string result = "";
    
    if (srd_disassembler_ != nullptr && crash_dump_contents_ != nullptr && !sgpr_groups.empty())
    {
        std::stringstream output;
        
        output << std::endl;
        output << "    " << "Wave coordinate ID: 0x" << std::hex << shader_id << std::dec << std::endl;
        // Process each SGPR group separately.
        for (size_t group_idx = 0; group_idx < sgpr_groups.size(); ++group_idx)
        {
            const SgprGroup& current_sgpr_group = sgpr_groups[group_idx];
            
            // Extract SRD data from SGPR registers for this group.
            std::vector<uint32_t> srd_data = ExtractSrdData(current_sgpr_group.indices, shader_id);
            if (!srd_data.empty())
            {
                // Disassemble the SRD with the detected type.
                std::string srd_disassembly = srd_disassembler_->DisassembleSrd(srd_data, current_sgpr_group.type);
                if (!srd_disassembly.empty())
                {
                    // Add group header if multiple groups exist.
                    if (sgpr_groups.size() > 1)
                    {
                        std::string type_name = "";
                        switch (current_sgpr_group.type)
                        {
                            case SrdType::kImage: type_name = "Image"; break;
                            case SrdType::kSampler: type_name = "Sampler"; break;
                            case SrdType::kBuffer: type_name = "Buffer"; break;
                            case SrdType::kBvh: type_name = "BVH"; break;
                        }
                        output << "    " << type_name << " SGPRs (s[" << current_sgpr_group.indices.front() << ":" << current_sgpr_group.indices.back() << "]):\n";
                    }
                    
                    // Add indentation to each line of the SRD disassembly.
                    std::stringstream srd_stream(srd_disassembly);
                    std::string line;
                    while (std::getline(srd_stream, line))
                    {
                        if (sgpr_groups.size() > 1)
                        {
                            // Extra indentation for grouped output.
                            output << "      " << line << "\n";
                        }
                        else
                        {
                            // Standard indentation for single group.
                            output << "    " << line << "\n";
                        }
                    }
                    
                    // Add spacing between groups.
                    if (group_idx < sgpr_groups.size() - 1)
                    {
                        output << "\n";
                    }
                }
            }
        }

        result = output.str();
    }

    return result;
}

nlohmann::json SrdInstructionAnalyzer::GetSrdAnalysisForOffendingInstructionJson(const std::string& instruction_text,
                                                                                  const std::vector<SgprGroup>& sgpr_groups,
                                                                                  uint32_t shader_id) const
{
    nlohmann::json result = nlohmann::json::array();
    
    if (srd_disassembler_ != nullptr && crash_dump_contents_ != nullptr && !sgpr_groups.empty())
    {
        try
        {
            // Process each SGPR group separately to handle multiple SRD types in one instruction.
            for (size_t group_idx = 0; group_idx < sgpr_groups.size(); ++group_idx)
            {
                const SgprGroup& current_sgpr_group = sgpr_groups[group_idx];
                
                // Extract SRD data from SGPR registers for this group.
                std::vector<uint32_t> srd_data = ExtractSrdData(current_sgpr_group.indices, shader_id);
                if (!srd_data.empty())
                {
                    // Disassemble the SRD with the detected type using JSON output.
                    nlohmann::json srd_json = srd_disassembler_->DisassembleSrdJson(srd_data, current_sgpr_group.type);
                    if (!srd_json.empty())
                    {
                        // Add group metadata to the SRD analysis.
                        std::string type_name = "";
                        switch (current_sgpr_group.type)
                        {
                            case SrdType::kImage: type_name = "Image"; break;
                            case SrdType::kSampler: type_name = "Sampler"; break;
                            case SrdType::kBuffer: type_name = "Buffer"; break;
                            case SrdType::kBvh: type_name = "BVH"; break;
                        }
                        
                        // Add metadata to the SRD JSON.
                        srd_json[SrdAnalysisJsonFields::kJsonElemWaveCoordinateId] = shader_id;
                        srd_json["type"] = type_name;
                        srd_json["sgpr_range"] = "s[" + std::to_string(current_sgpr_group.indices.front()) + ":" + std::to_string(current_sgpr_group.indices.back()) + "]";
                        
                        result.push_back(std::move(srd_json));
                    }
                }
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            // Log the error and return empty result.
            std::string error_msg = SrdAnalysisErrors::kJsonErrorPrefix + std::string(e.what());
            RgdUtils::PrintMessage(error_msg.c_str(), RgdMessageType::kError, true);
            result = nlohmann::json::array();
        }
        catch (const std::exception& e)
        {
            // Handle other exceptions that might occur during SRD analysis.
            std::string error_msg = SrdAnalysisErrors::kGeneralErrorPrefix + std::string(e.what());
            RgdUtils::PrintMessage(error_msg.c_str(), RgdMessageType::kError, true);
            result = nlohmann::json::array();
        }
    }

    return result;
}

bool SrdInstructionAnalyzer::DetectSgprUsage(const std::string& instruction_text, std::vector<SgprGroup>& sgpr_groups) const
{
    bool result = false;
    sgpr_groups.clear();

    // ISA decoder must be available for SGPR analysis.
    if (isa_decoder_ != nullptr)
    {
        // Try to detect SGPR groups using ISA decoder.
        result = DetectSgprGroupsUsingIsaDecoder(instruction_text, "", sgpr_groups);
    }
    else
    {
        // ISA decoder not initialized - error already logged during initialization.
        result = false;
    }

    return result;
}

bool SrdInstructionAnalyzer::DetectSgprGroupsUsingIsaDecoder(const std::string& instruction_text,
                                                            const std::string& machine_code,
                                                            std::vector<SgprGroup>& sgpr_groups) const
{
    bool result = false;
    
    if (isa_decoder_ != nullptr)
    {
        sgpr_groups.clear();

        // Extract machine code from instruction text or use provided machine code.
        std::string binary_code = machine_code;
        if (binary_code.empty())
        {
            binary_code = ExtractMachineCode(instruction_text);
        }

        // Try to decode using machine code binary first (more accurate).
        if (!binary_code.empty())
        {
            std::vector<uint32_t> machine_code_stream = ParseMachineCodeString(binary_code);
            if (!machine_code_stream.empty())
            {
                std::vector<amdisa::InstructionInfoBundle> instruction_info_stream;
                std::string err_message;
                
                if (isa_decoder_->DecodeInstructionStream(machine_code_stream, instruction_info_stream, err_message))
                {
                    // ISA decoding was successful - now try to extract SGPR groups.
                    for (const auto& bundle : instruction_info_stream)
                    {
                        for (const auto& instruction_info : bundle.bundle)
                        {
                            // Extract SGPR ranges from the actual instruction text and match with ISA info.
                            if (ExtractSgprGroupsFromInstructionAndIsa(instruction_text, instruction_info, sgpr_groups))
                            {
                                result = true;
                                break;
                            }
                        }
                        if (result)
                        {
                            // Only one instruction is being decoded at per call to DetectSgprGroupsUsingIsaDecoder().
                            break;
                        }
                    }
                    // Offending instruction may not always have the SGPR usage.
                    // So when no SGPRs were found, result remains false but this is not an error.
                }
                else
                {
                    // ISA decoder failed to decode this instruction - this is an error.
                    std::string error_msg = "ISA decoder failed to decode instruction stream. Error: " + err_message;
                    RgdUtils::PrintMessage(error_msg.c_str(), RgdMessageType::kError, true);
                    result = false;
                }
            }
            else
            {
                // Failed to parse machine code - Should not reach here.
                assert(false);
                result = false;
                RgdUtils::PrintMessage("failed to parse instruction machine code.", RgdMessageType::kError, true);
            }
        }
        else
        {
            // No machine code found - - Should not reach here.
            assert(false);
            result = false;
            RgdUtils::PrintMessage("failed to find instruction machine code.", RgdMessageType::kError, true);
        }
    }

    return result;
}

std::string SrdInstructionAnalyzer::ExtractMachineCode(const std::string& instruction_text) const
{
    std::string result = "";
    
    // Look for machine code after "//" comment marker.
    size_t comment_pos = instruction_text.find("//");
    if (comment_pos != std::string::npos)
    {
        std::string code_part = instruction_text.substr(comment_pos + 2);
        
        // Look for offset pattern and extract machine code after ": ".
        size_t colon_pos = code_part.find(": ");
        if (colon_pos != std::string::npos)
        {
            // Extract everything after the ": " separator.
            code_part = code_part.substr(colon_pos + 2);
        }
        
        // Trim leading/trailing whitespace using utility function.
        RgdUtils::TrimLeadingAndTrailingWhitespace(code_part, result);
    }
    
    return result;
}

std::vector<uint32_t> SrdInstructionAnalyzer::ParseMachineCodeString(const std::string& machine_code_str) const
{
    std::vector<uint32_t> machine_code_stream;
    
    if (machine_code_str.empty())
    {
        return machine_code_stream;
    }
    
    // Split the machine code string by spaces to get individual DWORD values.
    std::stringstream ss(machine_code_str);
    std::string dword_str;
    
    while (ss >> dword_str)
    {
        // Remove any non-hex characters and convert to uint32_t.
        std::string clean_dword;
        for (char c : dword_str)
        {
            if (std::isxdigit(c))
            {
                clean_dword += c;
            }
        }
        
        if (!clean_dword.empty() && clean_dword.length() <= 8) // Max 8 hex chars for 32-bit
        {
            try
            {
                uint32_t dword_val = static_cast<uint32_t>(std::stoul(clean_dword, nullptr, 16));
                machine_code_stream.push_back(dword_val);
            }
            catch (const std::exception&)
            {
                // Skip invalid hex values.
                continue;
            }
        }
    }
    
    return machine_code_stream;
}

bool SrdInstructionAnalyzer::ExtractSgprGroupsFromInstructionAndIsa(const std::string& instruction_text,
                                                                     const amdisa::InstructionInfo& instruction_info,
                                                                     std::vector<SgprGroup>& sgpr_groups) const
{
    bool result = false;
    sgpr_groups.clear();
    
    // First, check if this instruction has any RSRC or SAMP encoding fields.
    // Only instructions with RSRC fields should be considered for SRD analysis.
    bool has_rsrc_field = false;

    // Instructions with SAMP fields appear in texture/sample instructions along with RSRC.
    bool has_samp_field = false;
    for (const auto& field : instruction_info.encoding_fields)
    {
        if (field.field_name == "RSRC" || field.field_name == "SRSRC")
        {
            has_rsrc_field = true;
        }
        else if (field.field_name == "SAMP" || field.field_name == "SSAMP")
        {
            has_samp_field = true;
        }
    }
    
    // If RSRC field is present, this instruction uses resource descriptors.
    if (has_rsrc_field)
    {
        // Use regex to find SGPR ranges in the instruction text.
        std::regex sgpr_range_pattern(R"(\bs\[(\d+):(\d+)\])");
        std::smatch match;
        
        std::string::const_iterator search_start = instruction_text.cbegin();
        size_t group_index = 0;
        
        while (std::regex_search(search_start, instruction_text.cend(), match, sgpr_range_pattern))
        {
            uint32_t start_reg = static_cast<uint32_t>(std::stoul(match[1].str()));
            uint32_t end_reg = static_cast<uint32_t>(std::stoul(match[2].str()));
            
            SgprGroup current_sgpr_group;
            
            // Add all registers in the range.
            for (uint32_t reg = start_reg; reg <= end_reg; ++reg)
            {
                current_sgpr_group.indices.push_back(reg);
            }
            
            // Determine SRD type using ISA decoder information.
            current_sgpr_group.type = DetermineSrdTypeForGroup(instruction_info, group_index, sgpr_groups.size(), 
                                                              start_reg, end_reg, has_rsrc_field, has_samp_field);
            
            sgpr_groups.push_back(current_sgpr_group);
            
            search_start = match.suffix().first;
            ++group_index;
        }
        
        result = !sgpr_groups.empty();
    }

    return result;
}

SrdType SrdInstructionAnalyzer::DetermineSrdTypeForGroup(const amdisa::InstructionInfo& instruction_info, 
                                                        size_t operand_index,
                                                        size_t group_index, 
                                                        uint32_t start_reg, 
                                                        uint32_t end_reg,
                                                        bool has_rsrc_field,
                                                        bool has_samp_field) const
{
    const uint32_t group_size = end_reg - start_reg + 1;
    
    // SRD size constants.
    constexpr uint32_t kImageSrdSizeDwords = 8;
    constexpr uint32_t kSamplerSrdSizeDwords = 4;
    constexpr uint32_t kBufferSrdSizeDwords = 4;

    // Analyze instruction using ISA decoder metadata.
    SrdType srd_type = SrdType::kBuffer;  // Default fallback.

    // Check functional group and subgroup information.
    const auto subgroup = instruction_info.functional_group_subgroup_info.IsaFunctionalSubgroup;
    
    if (subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupBvh)
    {
        srd_type = SrdType::kBvh;
    }
    else if (subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupBuffer ||
             subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupLoad ||
             subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupStore)
    {
        srd_type = SrdType::kBuffer;
    }
    else if (subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupTexture ||
             subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupSample)
    {
        // For texture/sample instructions, distinguish between RSRC and SAMP based on the encoding fields.
        if (has_rsrc_field && has_samp_field)
        {
            // Both RSRC and SAMP present - distinguish by size and position.
            // First SGPR group (operand_index 0) with 8 dwords = Image RSRC.
            // Second SGPR group (operand_index 1) with 4 dwords = Sampler SAMP.
            if (group_size >= kImageSrdSizeDwords)
            {
                srd_type = SrdType::kImage;
            }
            else if (group_size == kSamplerSrdSizeDwords)
            {
                srd_type = SrdType::kSampler;
            }
            else
            {
                // Fallback: use operand position.
                srd_type = (operand_index == 0) ? SrdType::kImage : SrdType::kSampler;
            }
        }
        else if (has_rsrc_field)
        {
            // Only RSRC field present - must be image or buffer descriptor.
            if (group_size >= kImageSrdSizeDwords)
            {
                srd_type = SrdType::kImage;
            }
            else if (group_size == kSamplerSrdSizeDwords)
            {
                srd_type = SrdType::kBuffer;
            }
        }
    }
    else if(subgroup == amdisa::kFunctionalSubgroup::kFunctionalSubgroupAtomic)
    {
        // Atomic instructions can use buffer or Image SRDs.
        if(instruction_info.encoding_name == "ENC_MIMG" ||
           instruction_info.encoding_name == "MIMG_NSA1" ||
           instruction_info.encoding_name == "ENC_VIMAGE")
        {
            srd_type = SrdType::kImage;
        }
        else
        {
            srd_type = SrdType::kBuffer;
        }
    }

    return srd_type;
}

std::vector<uint32_t> SrdInstructionAnalyzer::ExtractSrdData(const std::vector<uint32_t>& sgpr_indices, uint32_t shader_id) const
{
    std::vector<uint32_t> srd_data;
    
    if (crash_dump_contents_ != nullptr && gpr_event_indices_ != nullptr && !sgpr_indices.empty())
    {
        // Use pre-built index to find SGPR data for this shader.
        auto it = gpr_event_indices_->find(shader_id);
        if (it != gpr_event_indices_->end() && it->second.first != 0)
        {
            size_t sgpr_event_index = it->second.first;
            const RgdEventOccurrence& event = crash_dump_contents_->kmd_crash_data.events[sgpr_event_index];
            const GprRegistersData* gpr_data = static_cast<const GprRegistersData*>(event.rgd_event);
            
            // Sort the indices to ensure proper order.
            std::vector<uint32_t> sorted_indices = sgpr_indices;
            std::sort(sorted_indices.begin(), sorted_indices.end());
            
            // Extract up to 8 consecutive dwords starting from the first SGPR.
            uint32_t start_reg = sorted_indices[0];
            constexpr uint32_t kMaxSrdDwords = 8;

            uint32_t i = 0;
            for (; i < kMaxSrdDwords; ++i)
            {
                uint32_t reg_index = start_reg + i;
                if (reg_index < gpr_data->regToRead && reg_index < kMaxGPRRegs)
                {
                    srd_data.push_back(gpr_data->reg[reg_index]);
                }
                else
                {
                    // If we can't find a consecutive register, break.
                    break;
                }
            }
            for (; i < kMaxSrdDwords; ++i)
            {
                // Pad with zeros if less than 8 dwords found. Avoids OOB access when parsing bitfields.
                srd_data.push_back(0);
            }
        }
    }

    return srd_data;
}

std::string SrdInstructionAnalyzer::GetSgprSignatureFromGroups(const std::vector<SgprGroup>& sgpr_groups, uint32_t shader_id) const
{
    std::string signature;
    
    if (!sgpr_groups.empty() && 
        gpr_event_indices_ != nullptr && 
        crash_dump_contents_ != nullptr)
    {
        auto it = gpr_event_indices_->find(shader_id);
        if (it != gpr_event_indices_->end() && it->second.first != 0)
        {
            // Get the SGPR event index.
            size_t sgpr_event_index = it->second.first;
            
            // Access the SGPR event.
            const RgdEventOccurrence& sgpr_event_occurrence = crash_dump_contents_->kmd_crash_data.events[sgpr_event_index];
            const GprRegistersData& sgpr_event = static_cast<const GprRegistersData&>(*sgpr_event_occurrence.rgd_event);
            
            // Create a signature from all SGPR groups.
            std::stringstream sig_stream;
            bool first_group = true;
            
            for (const auto& group : sgpr_groups)
            {
                if (!first_group)
                {
                    // Separate groups with pipe character.
                    sig_stream << "|";
                }
                
                // Sort the indices to ensure consistent signature generation.
                std::vector<uint32_t> sorted_indices = group.indices;
                std::sort(sorted_indices.begin(), sorted_indices.end());
                
                bool first_reg = true;
                for (uint32_t reg_index : sorted_indices)
                {
                    if (reg_index < sgpr_event.regToRead)
                    {
                        if (!first_reg)
                        {
                            sig_stream << "_";
                        }
                        sig_stream << std::hex << sgpr_event.reg[reg_index];
                        first_reg = false;
                    }
                }
                first_group = false;
            }
            
            signature = sig_stream.str();
        }
    }
    else
    {
        // Should not reach here.
        assert(false);
    }
    
    return signature;
}
