//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief 
//============================================================================================

#ifndef RGD_CODE_OBJECT_DATABASE_H_
#define RGD_CODE_OBJECT_DATABASE_H_

#include <set>
#include <unordered_map>

#include <CodeObjectDisassemblerApi.h>
#include <source/comgr_utils.h>

#include "rgd_hash.h"
#include "rgd_data_types.h"
#include "rgd_code_object_comgr_handle.h"
#include "rgd_dxbc_parser.h"

// Holds information about individual shader in a code object disassembly.
struct RgdShaderInfo
{
    // Crashing shader sequence number.
    uint64_t crashing_shader_id = 0;

    // 128 bit hash lower bits.
    uint64_t api_shader_hash_lo = 0;

    // 128 bit hash higher bits.
    uint64_t api_shader_hash_hi = 0;

    // Start offset address for the shader.
    uint64_t symbol_size = 0;

    // API Stage - Shader type from amdt::ShaderInfoType.
    std::string api_stage;

    // Hardware stage from amdt::HwStageType.
    std::string hw_stage;

    // Symbol name.
    std::string entry_point_symbol_name;

    // Alphanumeric shader info id.
    std::string str_shader_info_id;

    // Disassembled instructions block for the shader (used for text output - post processed later for adding annotations for instructions pointed by the wave PCs).
    std::vector<std::pair<uint64_t, std::string>> instructions;

    // Disassembled instructions block for the shader (used for the JSON output - no post processing required).
    std::vector<std::pair<uint64_t, std::string>> instructions_json_output;

    // DXC dumpbin output for the shader.
    std::string dxc_dumpbin_output;

    // High-level shader source code extracted from debug information.
    std::string high_level_source = kStrNotAvailable;

    // Entry name of the shader.
    std::string entry_point_name = kStrNotAvailable;

    // Shader source file name.
    std::string source_file_name = kStrNotAvailable;

    // Shader IO and resource binding information.
    std::string shader_io_and_resource_bindings = kStrNotAvailable;

    // PDB file path for the shader.
    std::string pdb_file_path;

    // Is this a crashing shader.
    bool is_in_flight_shader = false;

    // Was debug info found for this shader.
    bool has_debug_info = false;
};

class RgdCodeObjectEntry
{
public:
    RgdCodeObjectEntry()
        : code_obj_size_in_bytes_(0)
        , api_pso_hash_(0)
        , code_object_payload_({})
        , internal_pipeline_hash_({})
        , symbol_info_({})
        , pipeline_data_({})
        , comgr_handle_(nullptr)
        , amd_gpu_dis_context_({})
    {
    }

    ~RgdCodeObjectEntry()
    {
    }

    RgdCodeObjectEntry(RgdCodeObjectEntry&& entry) = default;

    RgdCodeObjectEntry& operator=(RgdCodeObjectEntry&& entry)
    {
        if (this != &entry)
        {
            code_obj_size_in_bytes_       = entry.code_obj_size_in_bytes_;
            api_pso_hash_                 = entry.api_pso_hash_;
            internal_pipeline_hash_       = entry.internal_pipeline_hash_;
            symbol_info_                  = entry.symbol_info_;
            pipeline_data_                = entry.pipeline_data_;
            comgr_handle_                 = std::move(entry.comgr_handle_);
            amd_gpu_dis_context_          = entry.amd_gpu_dis_context_;
            entry.code_obj_size_in_bytes_ = 0;
            entry.api_pso_hash_           = 0;
            entry.internal_pipeline_hash_ = {};
            entry.symbol_info_            = {};
            entry.pipeline_data_          = {};
            entry.comgr_handle_           = nullptr;
            entry.amd_gpu_dis_context_    = {};
        }
        return *this;
    }

    // Size of the code object in bytes.
    uint64_t code_obj_size_in_bytes_ = 0;

    // API PSO Hash.
    uint64_t api_pso_hash_ = 0;

    // A hash of the Code object binary to correlate the loader entry with the CODE_OBJECT_DATABASE entry it refers to.
    Rgd128bitHash internal_pipeline_hash_;

    // Code object binary payload.
    std::vector<uint8_t> code_object_payload_;

    // Symbol information.
    amdt::CodeObjSymbolInfo symbol_info_;

    // Pipeline metadata.
    amdt::PalPipelineData pipeline_data_;

    // COMGR handle.
    RgdComgrHandle   comgr_handle_;

    // Current ISA context handle.
    AmdGpuDisContext amd_gpu_dis_context_;

    // Map of the program counters offset and the count of waves that have the same program counter offset.
    std::map<uint64_t, size_t> pc_offset_to_hung_wave_count_map_;

    // Map of the hardware stage and the corresponding shader information.
    std::map<amdt::HwStageType, RgdShaderInfo> hw_stage_to_shader_info_map_;

    // Disassembly string for the code object.
    std::string disassembly_;
};

/// @brief Stores information about the crashing code objects.
class RgdCodeObjectDatabase
{
public:
    /// @brief Constructor.
    RgdCodeObjectDatabase() = default;

    /// @brief Destructor.
    ~RgdCodeObjectDatabase() = default;

    /// @brief Add a code object to the database.
    /// This method can only be called on a code object DB before Populate is called.
    bool AddCodeObject(uint64_t pc_instruction_offset, uint64_t api_pso_hash, size_t pc_wave_count, Rgd128bitHash internal_pipeline_hash, std::vector<uint8_t>&& code_object_payload);

    /// @brief Fill in the code object based on the buffer that is assigned to it previously.
    bool Populate();

    /// @brief Extract debug information for shaders if available
    /// This method will utilize the provided user configuration along with the directories specified.
    /// @param user_config Configuration for user-specific settings
    /// @param debug_info_dirs Vector of directories containing debug info files
    /// @return true if debug info extraction was successful, false otherwise
    bool ExtractDebugInfo(const Config& user_config, const std::vector<std::string>& debug_info_dirs);

    // vector to hold the RGD crashing code object entries.
    std::vector<RgdCodeObjectEntry>              entries_                             = {};
    std::map<Rgd128bitHash, RgdCodeObjectEntry*> internal_pipeline_hash_to_entry_map_ = {};
    bool                                         is_code_obj_db_built_                = false;
};

bool RgdCodeObjDbCreateIsaContextAmdGpuDis(RgdCodeObjectEntry* code_obj_entry);

#endif  // RGD_CODE_OBJECT_DATABASE_H_