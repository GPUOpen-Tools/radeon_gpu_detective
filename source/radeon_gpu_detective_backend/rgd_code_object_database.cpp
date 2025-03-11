//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief 
//============================================================================================

#include <cmath>
#include <sstream>
#include <regex>

#include "rgd_amd_gpu_dis_loader.h"
#include "rgd_code_object_database.h"
#include "rgd_utils.h"

// Global disassembler function table.
AmdGpuDisApiTable rgd_disassembler_api_table = {};

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***
static uint64_t crashing_shader_count = 0;

// Initialize the comgr handles for the code object entries.
static bool RgdCodeObjDbInitializeComgrHandles(RgdCodeObjectDatabase* code_object_db)
{
    assert(code_object_db != nullptr);
    
    bool ret = true;
    if (code_object_db != nullptr || code_object_db->entries_.size() == 0)
    {
        ret = false;
    }

    for (RgdCodeObjectEntry& entry : code_object_db->entries_)
    {
        const char* code_object_payload = (const char*)entry.code_object_payload_.data();
        entry.comgr_handle_             = amdt::CodeObj::OpenBufferRaw(code_object_payload, entry.code_obj_size_in_bytes_);
        if (entry.comgr_handle_ != nullptr)
        {
            // Extract the Pal PipelineData.
            const bool [[maybe_unused]] is_contains_pal_metadata =
                (amdt::kComgrUtilsStatusSuccess == entry.comgr_handle_->ExtractPalPipelineData(entry.pipeline_data_));

            // Now try to extract symbols.
            if (!entry.comgr_handle_->ExtractSymbolData(entry.symbol_info_))
            {
                RgdUtils::PrintMessage("failed to extract symbol data for the code object", RgdMessageType::kError, true);
                ret = false;
            }

            // Really should have at least 1 symbol.
            assert(is_contains_pal_metadata && entry.symbol_info_.num_symbols > 0);
        }
        else
        {
            // Failed to open the buffer.
            RgdUtils::PrintMessage("failed to open code object buffer.", RgdMessageType::kError, true);
            ret = false;
        }
    }

    return ret;
}

// Get the hardware stage for the shader.
static amdt::HwStageType GetHwStageForShader(uint16_t hardware_mapping_bit_field)
{
    int ret = log2(hardware_mapping_bit_field);
    amdt::HwStageType hw_stage = (amdt::HwStageType)ret;
    return hw_stage;
}

// Get the shader type string.
static std::string GetShaderTypeString(amdt::ShaderInfoType shader_type)
{
    std::stringstream ret;
    switch (shader_type)
    {
    case amdt::kVertexShader:
        ret << "Vertex";
        break;
    case amdt::kHullShader:
        ret << "Hull";
        break;
    case amdt::kDomainShader:
        ret << "Domain";
        break;
    case amdt::kGeometryShader:
        ret << "Geometry";
        break;
    case amdt::kPixelShader:
        ret << "Pixel";
        break;
    case amdt::kComputeShader:
        ret << "Compute";
        break;
    case amdt::kMeshShader:
        ret << "Mesh";
        break;
    case amdt::kTaskShader:
        ret << "Task";
        break;
    default:
        ret << kStrUnknown;
        break;
    }

    return ret.str();
}

// Get the hardware stage string.
static std::string GetHwStageString(amdt::HwStageType hw_stage)
{
    std::stringstream ret;
    switch (hw_stage)
    {
    case amdt::kHwStageEs:
        ret << "ES";
        break;
    case amdt::kHwStageGs:
        ret << "GS";
        break;
    case amdt::kHwStageVs:
        ret << "VS";
        break;
    case amdt::kHwStageHs:
        ret << "HS";
        break;
    case amdt::kHwStageLs:
        ret << "LS";
        break;
    case amdt::kHwStageSs:
        ret << "SS";
        break;
    case amdt::kHwStagePrimS:
        ret << "PrimS";
        break;
    case amdt::kHwStagePs:
        ret << "PS";
        break;
    case amdt::kHwStageCs:
        ret << "CS";
        break;
    default:
        ret << kStrUnknown;
        break;
    }

    return ret.str();
}

// Build the instructions vector for the shader.
static void BuildInstructionsVectorForShader(const std::string&                             disassembly_text,
                                             const std::string                              entry_point_symbol_name,
                                             uint64_t                                       symbol_size,
                                             std::vector<std::pair<uint64_t, std::string>>& out_instructions_vector)
{
    std::istringstream disassembly_stream(disassembly_text);
    std::string        line;

    // Regular expression to match lines with instructions.
    std::regex symbol_entry_point_regex(R"((_amdgpu_.._main):)");
    std::regex instruction_regex(R"((^\s*)(.+)(//\s)([0-9a-fA-F]+):(\s[0-9a-fA-F]+){1,3})");
    std::regex branch_label_regex(R"(^_L[\d]+:)");
    std::regex padding_instruction_s_code_end_regex(R"((^\s*)(s_code_end)(.+))");

    bool is_entry_point_found = false;
    uint64_t start_offset     = UINT64_MAX;
    uint64_t end_offset       = 0;

    // Parse the disassembly text to extract instructions and their offsets.
    while (std::getline(disassembly_stream, line))
    {
        // Check if the line starts with a tab character
        if (!line.empty() && line[0] == '\t')
        {
            // Replace the tab character with spaces (e.g., 4 spaces)
            line.replace(0, 1, "    ");
        }

        std::smatch match;
        if (std::regex_match(line, match, symbol_entry_point_regex))
        {
            if (match[1].str() == entry_point_symbol_name)
            {
                is_entry_point_found = true;
            }
        }

        if (is_entry_point_found)
        {
            if (std::regex_match(line, match, padding_instruction_s_code_end_regex))
            {
                break;
            }

            if (std::regex_match(line, match, instruction_regex))
            {
                // Extract the offset from the matched line.
                uint64_t offset = std::stoull(match[4].str(), nullptr, 16);
                if (start_offset == UINT64_MAX)
                {
                    // Extract the offset from the matched line.
                    start_offset = offset;
                    end_offset   = start_offset + symbol_size;
                }

                if (offset < end_offset)
                {
                    out_instructions_vector.emplace_back(offset, line);
                }
                else
                {
                    break;
                }
            }
            else if (std::regex_match(line, match, branch_label_regex))
            {
                uint64_t offset = 0;
                out_instructions_vector.emplace_back(offset, line);
            }
        }
    }
}

// Check if the shader was in-flight at the time of the crash.
static bool IsInFlightShader(const std::map<uint64_t, size_t>& pc_offset_to_hung_wave_count_map_, RgdShaderInfo shader_info)
{
    bool is_in_flight_shader = false;
    assert(shader_info.instructions.size() != 0);
    if (shader_info.instructions.size() != 0)
    {
        uint64_t start_offset = shader_info.instructions[0].first;
        uint64_t end_offset   = shader_info.instructions[shader_info.instructions.size() - 1].first;

        for (const std::pair<uint64_t, size_t>& pc_offset_to_hung_wave_count : pc_offset_to_hung_wave_count_map_)
        {
            // Check if the PC offset falls within the range of the shader instructions.
            // PC always points to the next instruction to execute. So it not expected to be equal to the offset for the first instruction in the shader
            if (pc_offset_to_hung_wave_count.first > start_offset && pc_offset_to_hung_wave_count.first <= end_offset)
            {
                is_in_flight_shader = true;
                break;
            }
        }
    }
    return is_in_flight_shader;
}

// Get the symbol size from the symbol name metadata.
static uint64_t RgdGetSymbolSizeFromSymbolName(const std::string symbol_name, RgdCodeObjectEntry* entry)
{
    uint64_t symbol_size = 0;
    for (uint32_t j = 0; j < entry->symbol_info_.num_symbols; ++j)
    {
        const amdt::CodeObjSymbol* symbol = &(entry->symbol_info_.symbols[j]);
        if (symbol_name == symbol->symbol_function.name)
        {
            symbol_size = symbol->symbol_function.symbol_size;
            break;
        }
    }

    return symbol_size;
}

// Initialize the code object entry.
static void RgdCodeObjDbInitCodeObjEntry(RgdCodeObjectEntry* entry)
{
    assert(entry != nullptr);
    if (entry != nullptr)
    {
        if (RgdCodeObjDbCreateIsaContextAmdGpuDis(entry))
        {
            AmdGpuDisStatus status = AMD_GPU_DIS_STATUS_FAILED;

            // Get the full AMDGPU disassembly for the code object entry.
            size_t disassembly_string_size = 0;
            status = rgd_disassembler_api_table.AmdGpuDisGetDisassemblyStringSize(entry->amd_gpu_dis_context_, &disassembly_string_size);
            if (status == AMD_GPU_DIS_STATUS_SUCCESS)
            {
                std::vector<char> disassembly_string(disassembly_string_size);
                status = rgd_disassembler_api_table.AmdGpuDisGetDisassemblyString(entry->amd_gpu_dis_context_, disassembly_string.data());
                if (status == AMD_GPU_DIS_STATUS_SUCCESS)
                {
                    entry->disassembly_ = std::string(disassembly_string.data());
                }
                else
                {
                    entry->disassembly_ = kStrNotAvailable;
                }
            }

            const amdt::PalPipelineData* pipeline_data = &(entry->pipeline_data_);

            assert(pipeline_data->num_pipelines != 0);
            assert(pipeline_data->pipelines != nullptr);

            if (pipeline_data->pipelines != nullptr && pipeline_data->num_pipelines != 0)
            {
                // For each shader in the pipeline metadata, build the shader info.
                for (size_t j = 0; j < pipeline_data->pipelines[0].num_shaders; ++j)
                {
                    amdt::HwStageType stage = GetHwStageForShader(pipeline_data->pipelines[0].shader_list[j].hardware_mapping_bit_field);
                    RgdShaderInfo     shader_info;
                    assert(entry->hw_stage_to_shader_info_map_.find(stage) == entry->hw_stage_to_shader_info_map_.end());
                    shader_info.api_shader_hash_lo             = pipeline_data->pipelines[0].shader_list[j].api_shader_hash_lo;
                    shader_info.api_shader_hash_hi             = pipeline_data->pipelines[0].shader_list[j].api_shader_hash_hi;
                    shader_info.api_stage                      = GetShaderTypeString(pipeline_data->pipelines[0].shader_list[j].shader_type);
                    shader_info.hw_stage                       = GetHwStageString(stage);
                    entry->hw_stage_to_shader_info_map_[stage] = shader_info;
                }

                // For each stage in the pipeline metadata, update the respective shader info.
                for (size_t j = 0; j < pipeline_data->pipelines[0].num_stages; ++j)
                {
                    amdt::HwStageType stage = pipeline_data->pipelines[0].stage_list[j].stage_type;

                    assert(entry->hw_stage_to_shader_info_map_.find(stage) != entry->hw_stage_to_shader_info_map_.end());
                    if (entry->hw_stage_to_shader_info_map_.find(stage) != entry->hw_stage_to_shader_info_map_.end())
                    {
                        RgdShaderInfo& shader_info          = entry->hw_stage_to_shader_info_map_[stage];
                        assert(pipeline_data->pipelines[0].stage_list[j].entry_point_symbol_name != nullptr);
                        if (pipeline_data->pipelines[0].stage_list[j].entry_point_symbol_name != nullptr)
                        {
                            shader_info.entry_point_symbol_name = pipeline_data->pipelines[0].stage_list[j].entry_point_symbol_name;
                        }
                        else
                        {
                            shader_info.entry_point_symbol_name = "";
                            RgdUtils::PrintMessage("failed to find entry point symbol name for the hardware stage.", RgdMessageType::kError, true);
                        }
                        shader_info.symbol_size             = RgdGetSymbolSizeFromSymbolName(shader_info.entry_point_symbol_name, entry);

                        // Build the instructions vector for the shader.
                        assert(shader_info.symbol_size != 0);
                        assert(!entry->disassembly_.empty());
                        BuildInstructionsVectorForShader(
                            entry->disassembly_, shader_info.entry_point_symbol_name, shader_info.symbol_size, shader_info.instructions);

                        // Make a copy of instructions for json output.
                        shader_info.instructions_json_output = shader_info.instructions;

                        // Check if this shader is the crashing shader.
                        assert(shader_info.instructions.size() != 0);
                        if (shader_info.instructions.size() != 0)
                        {
                            // Update the crashing shader flag and assign the seq number/id to the crashing shader.
                            if (IsInFlightShader(entry->pc_offset_to_hung_wave_count_map_, shader_info))
                            {
                                shader_info.is_in_flight_shader = true;
                                shader_info.crashing_shader_id  = ++crashing_shader_count;
                                shader_info.str_shader_info_id  = RgdUtils::GetAlphaNumericId(kStrPrefixShaderInfoId, shader_info.crashing_shader_id);
                            }
                        }
                    }
                    else
                    {
                        RgdUtils::PrintMessage("failed to find shader info for the hardware stage.", RgdMessageType::kError, true);
                    }
                }
            }
            else
            {
                RgdUtils::PrintMessage("failed to find pipeline data for the code object.", RgdMessageType::kError, true);
            }
        }
    }
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

bool RgdCodeObjectDatabase::AddCodeObject(uint64_t               pc_instruction_offset,
                                          uint64_t               api_pso_hash,
                                          size_t                 pc_wave_count,
                                          Rgd128bitHash          internal_pipeline_hash,
                                          std::vector<uint8_t>&& code_object_payload)
{
    bool ret = true;
    if (!is_code_obj_db_built_)
    {
        if (internal_pipeline_hash_to_entry_map_.find(internal_pipeline_hash) == internal_pipeline_hash_to_entry_map_.end())
        {
            // Code object entry does not exist. Create a new one.
            entries_.emplace_back();
            RgdCodeObjectEntry& entry     = entries_.back();
            entry.internal_pipeline_hash_ = internal_pipeline_hash;
            entry.api_pso_hash_           = api_pso_hash;
            entry.code_object_payload_    = std::move(code_object_payload);
            entry.code_obj_size_in_bytes_ = entry.code_object_payload_.size();
            assert(entry.code_obj_size_in_bytes_ != 0);
            internal_pipeline_hash_to_entry_map_[internal_pipeline_hash] = &entry;

            assert(entry.pc_offset_to_hung_wave_count_map_.find(pc_instruction_offset) == entry.pc_offset_to_hung_wave_count_map_.end());
            entry.pc_offset_to_hung_wave_count_map_[pc_instruction_offset] = pc_wave_count;
        }
        else
        {
            // Code object entry already exists. Update the additional pc instruction offset information.
            RgdCodeObjectEntry& entry = *internal_pipeline_hash_to_entry_map_[internal_pipeline_hash];

            if (entry.pc_offset_to_hung_wave_count_map_.find(pc_instruction_offset) != entry.pc_offset_to_hung_wave_count_map_.end())
            {
                // This pc instruction offset already exists. Update the wave count.
                entry.pc_offset_to_hung_wave_count_map_[pc_instruction_offset] += pc_wave_count;
            }
            else
            {
                // This pc instruction offset does not exist. Add it.
                entry.pc_offset_to_hung_wave_count_map_[pc_instruction_offset] = pc_wave_count;
            }
        }
    }
    else
    {
        // AddCodeObject should be called before Populate.
        assert(false);
        ret = false;
    }

    return ret;
}

bool RgdCodeObjectDatabase::Populate()
{
    bool ret = true;
    if (entries_.size() == 0)
    {
        ret = false;
    }
    else
    {
        rgd_disassembler_api_table.MajorVersion = AMD_GPU_DIS_MAJOR_VERSION_NUMBER;
        rgd_disassembler_api_table.MinorVersion = AMD_GPU_DIS_MINOR_VERSION_NUMBER;

        AmdGpuDisStatus status = AMD_GPU_DIS_STATUS_FAILED;

        if (nullptr == AmdGpuDisEntryPoints::Instance()->AmdGpuDisGetApiTable_fn)
        {
            RgdUtils::PrintMessage("unable to load and initialize disassembler.", RgdMessageType::kError, true);
            ret = false;
        }
        else
        {
            status = AmdGpuDisEntryPoints::Instance()->AmdGpuDisGetApiTable_fn(&rgd_disassembler_api_table);
            if (status == AMD_GPU_DIS_STATUS_FAILED)
            {
                RgdUtils::PrintMessage("failed to get disassembler API table.", RgdMessageType::kError, true);
                ret = false;
            }
        }

        if (ret)
        {
            ret = RgdCodeObjDbInitializeComgrHandles(this);

            for (RgdCodeObjectEntry& entry : entries_)
            {
                RgdCodeObjDbInitCodeObjEntry(&entry);
            }
        }
        is_code_obj_db_built_ = ret;
    }

    return ret;
}

bool RgdCodeObjDbCreateIsaContextAmdGpuDis(RgdCodeObjectEntry* code_obj_entry)
{
    bool ret = false;
    assert(code_obj_entry != nullptr);
    if (code_obj_entry != nullptr)
    {
        AmdGpuDisContext context;
        AmdGpuDisStatus  status   = AMD_GPU_DIS_STATUS_FAILED;

        if (nullptr != rgd_disassembler_api_table.AmdGpuDisCreateContext)
        {
            status = rgd_disassembler_api_table.AmdGpuDisCreateContext(&context);
            if (status == AMD_GPU_DIS_STATUS_SUCCESS)
            {
                status = rgd_disassembler_api_table.AmdGpuDisLoadCodeObjectBuffer(
                    context, (const char*)code_obj_entry->code_object_payload_.data(), code_obj_entry->code_obj_size_in_bytes_, false);
                if (status == AMD_GPU_DIS_STATUS_SUCCESS)
                {
                    code_obj_entry->amd_gpu_dis_context_ = context;
                    ret                                  = true;
                }
                else
                {
                    RgdUtils::PrintMessage("failed to load code object buffer.", RgdMessageType::kError, true);
                }
            }
            else
            {
                RgdUtils::PrintMessage("failed to create disassembler context.", RgdMessageType::kError, true);
            }
        }
        else
        {
            RgdUtils::PrintMessage("disassembler API table is not initialized.", RgdMessageType::kError, true);
        }
    }
    return ret;
}
