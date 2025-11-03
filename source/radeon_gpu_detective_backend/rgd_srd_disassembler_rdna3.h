//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief RDNA3 SRD (Shader Resource Descriptor) disassembler implementation.
//============================================================================================

#ifndef RGD_SRD_DISASSEMBLER_RDNA3_H_
#define RGD_SRD_DISASSEMBLER_RDNA3_H_

// Local.
#include "rgd_srd_disassembler.h"

/// @brief RDNA3 Buffer SRD implementation.
class SrdBufferRdna3 : public ShaderResourceDescriptor
{
public:
    explicit SrdBufferRdna3(const std::vector<uint32_t>& data);
    std::string ToString() const override;
    nlohmann::json ToJson() const override;
    SrdType GetType() const override { return SrdType::kBuffer; }

private:
    /// @brief Structure to hold all extracted buffer fields.
    struct BufferFields
    {
        uint64_t base_address;
        uint32_t stride;
        uint32_t swizzle_enable;
        uint32_t num_records;
        uint32_t dstsel_x;
        uint32_t dstsel_y;
        uint32_t dstsel_z;
        uint32_t dstsel_w;
        uint32_t format;
        uint32_t index_stride;
        uint32_t add_tid_enable;
        uint32_t llc_noalloc;
        uint32_t oob_select;
    };

    /// @brief Extract all fields from the SRD data.
    /// @return Structure containing all extracted fields.
    BufferFields ExtractFields() const;

    std::string GetBufferFormatString(uint32_t format) const;
    std::string GetDstSelString(uint32_t dst_sel) const;
    std::string GetOobSelectString(uint32_t oob_select) const;
    std::string GetIndexStrideString(uint32_t index_stride) const;
};

/// @brief RDNA3 Image SRD implementation.
class SrdImageRdna3 : public ShaderResourceDescriptor
{
public:
    explicit SrdImageRdna3(const std::vector<uint32_t>& data);
    std::string ToString() const override;
    nlohmann::json ToJson() const override;
    SrdType GetType() const override { return SrdType::kImage; }

private:
    /// @brief Structure to hold all extracted image fields.
    struct ImageFields
    {
        uint64_t base_address;
        uint32_t llc_noalloc;
        uint32_t big_page;
        uint32_t max_mip;
        uint32_t format;
        uint32_t width;
        uint32_t height;
        uint32_t dstsel_x;
        uint32_t dstsel_y;
        uint32_t dstsel_z;
        uint32_t dstsel_w;
        uint32_t base_level;
        uint32_t last_level;
        uint32_t sw_mode;
        uint32_t bc_swizzle;
        uint32_t rsrcType;
        uint32_t depth;
        uint32_t base_array;
        uint32_t array_pitch;
        uint32_t min_lod_warn;
        uint32_t perf_mod;
        uint32_t corner_sample;
        uint32_t linked_resource;
        uint32_t prt_default;
        uint32_t min_lod;
        uint32_t iterate_256;
        uint32_t sample_pattern_offset;
        uint32_t max_uncompressed_block_size;
        uint32_t max_compressed_block_size;
        uint32_t meta_pipe_aligned;
        uint32_t write_compress_en;
        uint32_t compression_en;
        uint32_t alpha_is_on_msb;
        uint32_t color_transform;
        uint64_t meta_data_addr;
    };

    /// @brief Extract all fields from the SRD data.
    /// @return Structure containing all extracted fields.
    ImageFields ExtractFields() const;

    std::string GetImageTypeString(uint32_t rsrcType) const;
    std::string GetImageFormatString(uint32_t format) const;
    std::string GetDstSelString(uint32_t dst_sel) const;
    std::string GetSwizzleModeString(uint32_t sw_mode) const;
    std::string GetBcSwizzleString(uint32_t bc_swizzle) const;
};

/// @brief RDNA3 Sampler SRD implementation.
class SrdSamplerRdna3 : public ShaderResourceDescriptor
{
public:
    explicit SrdSamplerRdna3(const std::vector<uint32_t>& data);
    std::string ToString() const override;
    nlohmann::json ToJson() const override;
    SrdType GetType() const override { return SrdType::kSampler; }

private:
    /// @brief Structure to hold all extracted sampler fields.
    struct SamplerFields
    {
        uint32_t clampX;
        uint32_t clampY;
        uint32_t clampZ;
        uint32_t max_aniso_ratio;
        uint32_t depth_compare_func;
        uint32_t force_unnormalized;
        uint32_t aniso_threshold;
        uint32_t mc_coord_trunc;
        uint32_t force_degamma;
        uint32_t aniso_bias;
        uint32_t trunc_coord;
        uint32_t disable_cube_wrap;
        uint32_t filter_mode;
        uint32_t skip_degamma;
        uint32_t min_lod;
        uint32_t max_lod;
        uint32_t perf_mip;
        uint32_t perf_z;
        uint32_t lod_bias;
        uint32_t lod_bias_sec;
        uint32_t xy_mag_filter;
        uint32_t xy_min_filter;
        uint32_t z_filter;
        uint32_t mip_filter;
        uint32_t aniso_override;
        uint32_t blend_zero_prt;
        uint32_t border_color_ptr;
        uint32_t border_color_type;
    };

    /// @brief Extract all fields from the SRD data.
    /// @return Structure containing all extracted fields.
    SamplerFields ExtractFields() const;

    std::string GetClampString(uint32_t mode) const;
    std::string GetFilterModeString(uint32_t mode) const;
    std::string GetAnisoString(uint32_t aniso) const;
    std::string GetDepthCompareString(uint32_t func) const;
    std::string GetXYFilterString(uint32_t filter) const;
    std::string GetZFilterString(uint32_t filter) const;
    std::string GetMipFilterString(uint32_t filter) const;
    std::string GetBorderColorTypeString(uint32_t type) const;

};

/// @brief RDNA3 BVH SRD implementation.
class SrdBvhRdna3 : public ShaderResourceDescriptor
{
public:
    explicit SrdBvhRdna3(const std::vector<uint32_t>& data);
    std::string ToString() const override;
    nlohmann::json ToJson() const override;
    SrdType GetType() const override { return SrdType::kBvh; }

private:
    /// @brief Structure to hold all extracted BVH fields.
    struct BvhFields
    {
        uint64_t base_address;
        uint32_t box_sorting_heuristic;
        uint32_t box_grow_value;
        uint32_t box_sort_en;
        uint64_t size;
        uint32_t pointer_flags;
        uint32_t triangle_return_mode;
        uint32_t llc_noalloc;
        uint32_t big_page;
    };

    /// @brief Extract all fields from the SRD data.
    /// @return Structure containing all extracted fields.
    BvhFields ExtractFields() const;

    std::string GetBoxSortingHeuristicString(uint32_t box_sorting_heuristic) const;
};

/// @brief RDNA3 SRD disassembler implementation.
class SrdDisassemblerRdna3 : public ISrdDisassembler
{
public:
    std::unique_ptr<ShaderResourceDescriptor> CreateSrd(const std::vector<uint32_t>& data, SrdType type) const override;
    std::string DisassembleSrd(const std::vector<uint32_t>& data, SrdType type) const override;
    nlohmann::json DisassembleSrdJson(const std::vector<uint32_t>& data, SrdType type) const override;
};

#endif  // RGD_SRD_DISASSEMBLER_RDNA3_H_
