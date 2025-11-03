//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief RDNA3 SRD (Shader Resource Descriptor) disassembler implementation.
//============================================================================================

#include "rgd_srd_disassembler_rdna3.h"

// C++.
#include <iomanip>
#include <sstream>

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// Architecture name constants.
static const char* kArchitectureName = "RDNA3";

SrdBufferRdna3::SrdBufferRdna3(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdBufferRdna3::BufferFields SrdBufferRdna3::ExtractFields() const
{
    BufferFields fields;

    // Base address (bits 47:0 from dwords 0 and 1).
    fields.base_address = static_cast<uint64_t>(GetDword(0)) | 
                          (static_cast<uint64_t>(ExtractBits(1, 0, 15)) << 32);
    fields.stride         = ExtractBitsFull(48, 14);
    fields.swizzle_enable = ExtractBitsFull(62, 2);
    fields.num_records    = ExtractBitsFull(64, 32);
    fields.dstsel_x       = ExtractBitsFull(96, 3);
    fields.dstsel_y       = ExtractBitsFull(99, 3);
    fields.dstsel_z       = ExtractBitsFull(102, 3);
    fields.dstsel_w       = ExtractBitsFull(105, 3);
    fields.format         = ExtractBitsFull(108, 6);
    fields.index_stride   = ExtractBitsFull(117, 2);
    fields.add_tid_enable = ExtractBitsFull(119, 1);
    fields.llc_noalloc    = ExtractBitsFull(122, 2);
    fields.oob_select     = ExtractBitsFull(124, 2);
    return fields;
}

std::string SrdBufferRdna3::ToString() const
{
    std::stringstream ss;
    ss << "Buffer (" << kArchitectureName << "):\n";
    
    const auto fields = ExtractFields();
    const std::string type = "Buffer";

    ss << "  " << kStrBufferBaseAddr << ": 0x" << std::hex << fields.base_address << "\n";
    ss << "  " << kStrBufferStride << ": 0x" << std::hex << fields.stride << "\n";
    ss << "  " << kStrBufferSwizzleEnable << ": " << std::dec << fields.swizzle_enable << "\n";
    ss << "  " << kStrBufferNumRecords << ": 0x" << std::hex << fields.num_records << "\n";
    ss << "  " << kStrBufferDstSelX << ": " << GetDstSelString(fields.dstsel_x) << "\n";
    ss << "  " << kStrBufferDstSelY << ": " << GetDstSelString(fields.dstsel_y) << "\n";
    ss << "  " << kStrBufferDstSelZ << ": " << GetDstSelString(fields.dstsel_z) << "\n";
    ss << "  " << kStrBufferDstSelW << ": " << GetDstSelString(fields.dstsel_w) << "\n";
    ss << "  " << kStrBufferFormat << ": " << GetBufferFormatString(fields.format) << "\n";
    ss << "  " << kStrBufferIndexStride << ": " << GetIndexStrideString(fields.index_stride) << "\n";
    ss << "  " << kStrBufferAddTidEnable << ": " << (fields.add_tid_enable ? "true" : "false") << "\n";
    ss << "  " << "LLC NoAlloc" << ": " << std::dec << fields.llc_noalloc << "\n";
    ss << "  " << kStrBufferOobSelect << ": " << GetOobSelectString(fields.oob_select) << "\n";
    ss << "  " << kStrBufferType << ": " << type << "\n";

    return ss.str();
}

std::string SrdBufferRdna3::GetBufferFormatString(uint32_t format) const
{
    // Buffer format definitions for RDNA3 (GFX11).
    switch (format)
    {    
    case 0x00000000: return "BUF_FMT_INVALID";
    case 0x00000001: return "BUF_FMT_8_UNORM";
    case 0x00000002: return "BUF_FMT_8_SNORM";
    case 0x00000003: return "BUF_FMT_8_USCALED";
    case 0x00000004: return "BUF_FMT_8_SSCALED";
    case 0x00000005: return "BUF_FMT_8_UINT";
    case 0x00000006: return "BUF_FMT_8_SINT";
    case 0x00000007: return "BUF_FMT_16_UNORM";
    case 0x00000008: return "BUF_FMT_16_SNORM";
    case 0x00000009: return "BUF_FMT_16_USCALED";
    case 0x0000000a: return "BUF_FMT_16_SSCALED";
    case 0x0000000b: return "BUF_FMT_16_UINT";
    case 0x0000000c: return "BUF_FMT_16_SINT";
    case 0x0000000d: return "BUF_FMT_16_FLOAT";
    case 0x0000000e: return "BUF_FMT_8_8_UNORM";
    case 0x0000000f: return "BUF_FMT_8_8_SNORM";
    case 0x00000010: return "BUF_FMT_8_8_USCALED";
    case 0x00000011: return "BUF_FMT_8_8_SSCALED";
    case 0x00000012: return "BUF_FMT_8_8_UINT";
    case 0x00000013: return "BUF_FMT_8_8_SINT";
    case 0x00000014: return "BUF_FMT_32_UINT";
    case 0x00000015: return "BUF_FMT_32_SINT";
    case 0x00000016: return "BUF_FMT_32_FLOAT";
    case 0x00000017: return "BUF_FMT_16_16_UNORM";
    case 0x00000018: return "BUF_FMT_16_16_SNORM";
    case 0x00000019: return "BUF_FMT_16_16_USCALED";
    case 0x0000001a: return "BUF_FMT_16_16_SSCALED";
    case 0x0000001b: return "BUF_FMT_16_16_UINT";
    case 0x0000001c: return "BUF_FMT_16_16_SINT";
    case 0x0000001d: return "BUF_FMT_16_16_FLOAT";
    case 0x0000001e: return "BUF_FMT_10_11_11_FLOAT";
    case 0x0000001f: return "BUF_FMT_11_11_10_FLOAT";
    case 0x00000020: return "BUF_FMT_10_10_10_2_UNORM";
    case 0x00000021: return "BUF_FMT_10_10_10_2_SNORM";
    case 0x00000022: return "BUF_FMT_10_10_10_2_UINT";
    case 0x00000023: return "BUF_FMT_10_10_10_2_SINT";
    case 0x00000024: return "BUF_FMT_2_10_10_10_UNORM";
    case 0x00000025: return "BUF_FMT_2_10_10_10_SNORM";
    case 0x00000026: return "BUF_FMT_2_10_10_10_USCALED";
    case 0x00000027: return "BUF_FMT_2_10_10_10_SSCALED";
    case 0x00000028: return "BUF_FMT_2_10_10_10_UINT";
    case 0x00000029: return "BUF_FMT_2_10_10_10_SINT";
    case 0x0000002a: return "BUF_FMT_8_8_8_8_UNORM";
    case 0x0000002b: return "BUF_FMT_8_8_8_8_SNORM";
    case 0x0000002c: return "BUF_FMT_8_8_8_8_USCALED";
    case 0x0000002d: return "BUF_FMT_8_8_8_8_SSCALED";
    case 0x0000002e: return "BUF_FMT_8_8_8_8_UINT";
    case 0x0000002f: return "BUF_FMT_8_8_8_8_SINT";
    case 0x00000030: return "BUF_FMT_32_32_UINT";
    case 0x00000031: return "BUF_FMT_32_32_SINT";
    case 0x00000032: return "BUF_FMT_32_32_FLOAT";
    case 0x00000033: return "BUF_FMT_16_16_16_16_UNORM";
    case 0x00000034: return "BUF_FMT_16_16_16_16_SNORM";
    case 0x00000035: return "BUF_FMT_16_16_16_16_USCALED";
    case 0x00000036: return "BUF_FMT_16_16_16_16_SSCALED";
    case 0x00000037: return "BUF_FMT_16_16_16_16_UINT";
    case 0x00000038: return "BUF_FMT_16_16_16_16_SINT";
    case 0x00000039: return "BUF_FMT_16_16_16_16_FLOAT";
    case 0x0000003a: return "BUF_FMT_32_32_32_UINT";
    case 0x0000003b: return "BUF_FMT_32_32_32_SINT";
    case 0x0000003c: return "BUF_FMT_32_32_32_FLOAT";
    case 0x0000003d: return "BUF_FMT_32_32_32_32_UINT";
    case 0x0000003e: return "BUF_FMT_32_32_32_32_SINT";
    case 0x0000003f: return "BUF_FMT_32_32_32_32_FLOAT";
    default:   return "BUF_FMT_UNKNOWN(" + std::to_string(format) + ")";
    }
}

std::string SrdBufferRdna3::GetIndexStrideString(uint32_t index_stride) const
{
    // Index stride definitions for RDNA3.
    switch (index_stride)
    {
    case 0: return "IndexStride_8B";
    case 1: return "IndexStride_16B";
    case 2: return "IndexStride_32B";
    case 3: return "IndexStride_64B";
    default: return "IndexStride_UNKNOWN";
    }
}

std::string SrdBufferRdna3::GetDstSelString(uint32_t dst_sel) const
{
    // Destination selection field definitions.
    switch (dst_sel)
    {
    case 0: return "DstSel.0";
    case 1: return "DstSel.1";
    case 4: return "DstSel.X";
    case 5: return "DstSel.Y";
    case 6: return "DstSel.Z";
    case 7: return "DstSel.W";
    default: return "DstSel.UNKNOWN";
    }
}

std::string SrdBufferRdna3::GetOobSelectString(uint32_t oob_select) const
{
    // Out-of-bounds select field definitions.
    switch (oob_select)
    {
    case 0: return "IndexAndOffset";
    case 1: return "IndexOnly";
    case 2: return "NumRecords0";
    case 3: return "Complete";
    default: return "UNKNOWN";
    }
}

nlohmann::json SrdBufferRdna3::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Buffer";
    json["architecture"] = "RDNA3";
    
    const auto fields = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    json["fields"] = {
        {"base_address", fields.base_address},
        {"stride", fields.stride},
        {"swizzle_enable", fields.swizzle_enable},
        {"num_records", fields.num_records},
        {"dstsel_x", GetDstSelString(fields.dstsel_x)},
        {"dstsel_y", GetDstSelString(fields.dstsel_y)},
        {"dstsel_z", GetDstSelString(fields.dstsel_z)},
        {"dstsel_w", GetDstSelString(fields.dstsel_w)},
        {"format", GetBufferFormatString(fields.format)},
        {"index_stride", GetIndexStrideString(fields.index_stride)},
        {"add_tid_enable", fields.add_tid_enable ? true : false},
        {"llc_noalloc", fields.llc_noalloc},
        {"oob_select", GetOobSelectString(fields.oob_select)}
    };

    return json;
}

enum SQ_RSRC_IMG_TYPE
{
    SQ_RSRC_IMG_1D            = 0x00000008,
    SQ_RSRC_IMG_2D            = 0x00000009,
    SQ_RSRC_IMG_3D            = 0x0000000a,
    SQ_RSRC_IMG_CUBE          = 0x0000000b,
    SQ_RSRC_IMG_1D_ARRAY      = 0x0000000c,
    SQ_RSRC_IMG_2D_ARRAY      = 0x0000000d,
    SQ_RSRC_IMG_2D_MSAA       = 0x0000000e,
    SQ_RSRC_IMG_2D_MSAA_ARRAY = 0x0000000f
};

static bool IsDepthAddOne(const uint32_t rsrc_type)
{
    return (rsrc_type == SQ_RSRC_IMG_1D || rsrc_type == SQ_RSRC_IMG_2D || rsrc_type == SQ_RSRC_IMG_2D_MSAA || rsrc_type == SQ_RSRC_IMG_3D);
}

static bool IsDepthPitch(const uint32_t rsrc_type)
{
    return (rsrc_type == SQ_RSRC_IMG_1D || rsrc_type == SQ_RSRC_IMG_2D || rsrc_type == SQ_RSRC_IMG_2D_MSAA);
}

SrdImageRdna3::SrdImageRdna3(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdImageRdna3::ImageFields SrdImageRdna3::ExtractFields() const
{
    ImageFields fields;
    
    // Base address (bits 39:0 from dwords 0 and 1, shifted left by 8).
    fields.base_address = ((static_cast<uint64_t>(GetDword(0)) | 
                           (static_cast<uint64_t>(ExtractBits(1, 0, 7)) << 32)) << 8);
    
    fields.llc_noalloc = ExtractBitsFull(45, 2);
    fields.big_page    = ExtractBitsFull(47, 1);
    fields.max_mip     = ExtractBitsFull(48, 4);
    fields.format      = ExtractBitsFull(52, 8);
    fields.width       = ExtractBitsFull(62, 14) + 1;
    fields.height      = ExtractBitsFull(78, 14) + 1;
    fields.dstsel_x    = ExtractBitsFull(96, 3);
    fields.dstsel_y    = ExtractBitsFull(99, 3);
    fields.dstsel_z    = ExtractBitsFull(102, 3);
    fields.dstsel_w    = ExtractBitsFull(105, 3);
    fields.base_level  = ExtractBitsFull(108, 4);
    fields.last_level  = ExtractBitsFull(112, 4);
    fields.sw_mode     = ExtractBitsFull(116, 5);
    fields.bc_swizzle  = ExtractBitsFull(121, 3);
    fields.rsrcType    = ExtractBitsFull(124, 4);
    
    // Special processing case; certain resource types add one.
    fields.depth       = ExtractBitsFull(128, 14);
    if (IsDepthAddOne(fields.rsrcType))
    {
        fields.depth += 1;
    }
    
    fields.base_array                  = ExtractBitsFull(144, 13);
    fields.array_pitch                 = ExtractBitsFull(160, 4);
    fields.min_lod_warn                = ExtractBitsFull(168, 12);
    fields.perf_mod                    = ExtractBitsFull(180, 3);
    fields.corner_sample               = ExtractBitsFull(183, 1);
    fields.linked_resource             = ExtractBitsFull(184, 1);
    fields.prt_default                 = ExtractBitsFull(186, 1);
    fields.min_lod                     = ExtractBitsFull(187, 12);
    fields.iterate_256                 = ExtractBitsFull(202, 1);
    fields.sample_pattern_offset       = ExtractBitsFull(203, 4);
    fields.max_uncompressed_block_size = ExtractBitsFull(207, 2);
    fields.max_compressed_block_size   = ExtractBitsFull(209, 2);
    fields.meta_pipe_aligned           = ExtractBitsFull(211, 1);
    fields.write_compress_en           = ExtractBitsFull(212, 1);
    fields.compression_en              = ExtractBitsFull(213, 1);
    fields.alpha_is_on_msb             = ExtractBitsFull(214, 1);
    fields.color_transform             = ExtractBitsFull(215, 1);
    
    // Metadata address (bits 255:216 from dwords 6 and 7, shifted left by 8).
    fields.meta_data_addr = ((static_cast<uint64_t>(ExtractBits(6, 24, 31)) |
                              (static_cast<uint64_t>(GetDword(7)) << 8))
                             << 8);

    return fields;
}

std::string SrdImageRdna3::ToString() const 
{
    std::stringstream ss;
    ss << "Image (" << kArchitectureName << "):\n";
    const auto fields = ExtractFields();
    
    ss << "  " << kStrImageBaseAddr << ": 0x" << std::hex << fields.base_address << "\n";

    ss << "  Llc_NoAlloc: " << std::dec << fields.llc_noalloc << "\n";
    ss << "  " << kStrImageBigPage << ": " << (fields.big_page ? "true" : "false")
       << "\n";
    ss << "  " << kStrImageMaxMip << ": 0x" << std::hex << fields.max_mip << "\n";
    ss << "  " << kStrImageFormat << ": " << GetImageFormatString(fields.format) << "\n";
    ss << "  " << kStrImageWidth << ": 0x" << std::hex << fields.width << "\n";
    ss << "  " << kStrImageHeight << ": 0x" << std::hex << fields.height << "\n";
    ss << "  " << kStrImageDstSelX << ": " << GetDstSelString(fields.dstsel_x) << "\n";
    ss << "  " << kStrImageDstSelY << ": " << GetDstSelString(fields.dstsel_y) << "\n";
    ss << "  " << kStrImageDstSelZ << ": " << GetDstSelString(fields.dstsel_z) << "\n";
    ss << "  " << kStrImageDstSelW << ": " << GetDstSelString(fields.dstsel_w) << "\n";
    ss << "  " << kStrImageBaseLevel << ": 0x" << std::hex << fields.base_level << "\n";
    ss << "  " << kStrImageLastLevel << ": 0x" << std::hex << fields.last_level << "\n";
    ss << "  " << "Swizzle mode" << ": " << GetSwizzleModeString(fields.sw_mode) << "\n";
    ss << "  " << kStrImageBcSwizzle << ": " << GetBcSwizzleString(fields.bc_swizzle)
       << "\n";
    ss << "  " << kStrImageType << ": " << GetImageTypeString(fields.rsrcType) << "\n";
    if (IsDepthPitch(fields.rsrcType)) {
        ss << "  Pitch: 0x" << std::hex << fields.depth << "\n";
    }
    else if (fields.rsrcType == SQ_RSRC_IMG_3D)
    {
        ss << "  " << kStrImageDepth << ": 0x" << std::hex << fields.depth << "\n";
    }
    else
    {
        ss << "  Last_Array: 0x" << std::hex << fields.depth << "\n";
    }
    ss << "  " << kStrImageBaseArray << ": 0x" << std::hex << fields.base_array << "\n";
    ss << "  " << kStrImageArrayPitch << ": 0x" << std::hex << fields.array_pitch << "\n";
    ss << "  " << kStrImageMinLodWarn3 << ": 0x" << std::hex << fields.min_lod_warn << "\n";
    ss << "  Perf_mod: 0x" << std::hex << fields.perf_mod << "\n";
    ss << "  " << kStrImageCornerSamples << ": " << (fields.corner_sample ? "true" : "false") << "\n";
    ss << "  " << "Linked_resource" << ": " << (fields.linked_resource ? "true" : "false") << "\n";
    ss << "  " << "PRT_default" << ": " << (fields.prt_default ? "true" : "false") << "\n";
    ss << "  " << kStrImageMinLod << ": " << std::dec << fields.min_lod << "\n";
    ss << "  " << kStrImageIterate256 << ": " << (fields.iterate_256 ? "true" : "false") << "\n";
    ss << "  " << "Sample_pattern_offset" << ": " << std::dec << fields.sample_pattern_offset << "\n";
    ss << "  " << "Max_uncompressed_block_Size" << ": 0x" << std::hex << fields.max_uncompressed_block_size << "\n";
    ss << "  " << "Max_compressed_block_Size" << ": 0x" << std::hex << fields.max_compressed_block_size << "\n";
    ss << "  " << kStrImageMetaPipeAligned << ": " << (fields.meta_pipe_aligned ? "true" : "false") << "\n";
    ss << "  Write_compress_en: " << (fields.write_compress_en ? "true" : "false") << "\n";
    ss << "  " << kStrImageCompressionEn << ": " << (fields.compression_en ? "true" : "false") << "\n";
    ss << "  " << kStrImageAlphaIsOnMsb << ": " << (fields.alpha_is_on_msb ? "true" : "false") << "\n";
    ss << "  " << kStrImageColorTransform << ": " << (fields.color_transform ? "true" : "false") << "\n";
    ss << "  " << kStrImageMetaDataAddress << ": 0x" << std::hex << fields.meta_data_addr << "\n";
    
    return ss.str();
}

std::string SrdImageRdna3::GetImageTypeString(uint32_t rsrcType) const
{
    // Image type definitions based on GPU specifications.
    switch (rsrcType)
    {
    case 8:  return "SQ_RSRC_IMG_1D";
    case 9:  return "SQ_RSRC_IMG_2D";
    case 10: return "SQ_RSRC_IMG_3D";
    case 11: return "SQ_RSRC_IMG_CUBE";
    case 12: return "SQ_RSRC_IMG_1D_ARRAY";
    case 13: return "SQ_RSRC_IMG_2D_ARRAY";
    case 14: return "SQ_RSRC_IMG_2D_MSAA";
    case 15: return "SQ_RSRC_IMG_2D_MSAA_ARRAY";
    default: return "SQ_RSRC_IMG_UNKNOWN";
    }
}

std::string SrdImageRdna3::GetImageFormatString(uint32_t format) const
{
    // Image format definitions for RDNA3.
    switch (format)
    {
    case 0x00000000: return "IMG_FMT_INVALID";
    case 0x00000001: return "IMG_FMT_8_UNORM";
    case 0x00000002: return "IMG_FMT_8_SNORM";
    case 0x00000003: return "IMG_FMT_8_USCALED";
    case 0x00000004: return "IMG_FMT_8_SSCALED";
    case 0x00000005: return "IMG_FMT_8_UINT";
    case 0x00000006: return "IMG_FMT_8_SINT";
    case 0x00000007: return "IMG_FMT_16_UNORM";
    case 0x00000008: return "IMG_FMT_16_SNORM";
    case 0x00000009: return "IMG_FMT_16_USCALED";
    case 0x0000000a: return "IMG_FMT_16_SSCALED";
    case 0x0000000b: return "IMG_FMT_16_UINT";
    case 0x0000000c: return "IMG_FMT_16_SINT";
    case 0x0000000d: return "IMG_FMT_16_FLOAT";
    case 0x0000000e: return "IMG_FMT_8_8_UNORM";
    case 0x0000000f: return "IMG_FMT_8_8_SNORM";
    case 0x00000010: return "IMG_FMT_8_8_USCALED";
    case 0x00000011: return "IMG_FMT_8_8_SSCALED";
    case 0x00000012: return "IMG_FMT_8_8_UINT";
    case 0x00000013: return "IMG_FMT_8_8_SINT";
    case 0x00000014: return "IMG_FMT_32_UINT";
    case 0x00000015: return "IMG_FMT_32_SINT";
    case 0x00000016: return "IMG_FMT_32_FLOAT";
    case 0x00000017: return "IMG_FMT_16_16_UNORM";
    case 0x00000018: return "IMG_FMT_16_16_SNORM";
    case 0x00000019: return "IMG_FMT_16_16_USCALED";
    case 0x0000001a: return "IMG_FMT_16_16_SSCALED";
    case 0x0000001b: return "IMG_FMT_16_16_UINT";
    case 0x0000001c: return "IMG_FMT_16_16_SINT";
    case 0x0000001d: return "IMG_FMT_16_16_FLOAT";
    case 0x0000001e: return "IMG_FMT_10_11_11_FLOAT";
    case 0x0000001f: return "IMG_FMT_11_11_10_FLOAT";
    case 0x00000020: return "IMG_FMT_10_10_10_2_UNORM";
    case 0x00000021: return "IMG_FMT_10_10_10_2_SNORM";
    case 0x00000022: return "IMG_FMT_10_10_10_2_UINT";
    case 0x00000023: return "IMG_FMT_10_10_10_2_SINT";
    case 0x00000024: return "IMG_FMT_2_10_10_10_UNORM";
    case 0x00000025: return "IMG_FMT_2_10_10_10_SNORM";
    case 0x00000026: return "IMG_FMT_2_10_10_10_USCALED";
    case 0x00000027: return "IMG_FMT_2_10_10_10_SSCALED";
    case 0x00000028: return "IMG_FMT_2_10_10_10_UINT";
    case 0x00000029: return "IMG_FMT_2_10_10_10_SINT";
    case 0x0000002a: return "IMG_FMT_8_8_8_8_UNORM";
    case 0x0000002b: return "IMG_FMT_8_8_8_8_SNORM";
    case 0x0000002c: return "IMG_FMT_8_8_8_8_USCALED";
    case 0x0000002d: return "IMG_FMT_8_8_8_8_SSCALED";
    case 0x0000002e: return "IMG_FMT_8_8_8_8_UINT";
    case 0x0000002f: return "IMG_FMT_8_8_8_8_SINT";
    case 0x00000030: return "IMG_FMT_32_32_UINT";
    case 0x00000031: return "IMG_FMT_32_32_SINT";
    case 0x00000032: return "IMG_FMT_32_32_FLOAT";
    case 0x00000033: return "IMG_FMT_16_16_16_16_UNORM";
    case 0x00000034: return "IMG_FMT_16_16_16_16_SNORM";
    case 0x00000035: return "IMG_FMT_16_16_16_16_USCALED";
    case 0x00000036: return "IMG_FMT_16_16_16_16_SSCALED";
    case 0x00000037: return "IMG_FMT_16_16_16_16_UINT";
    case 0x00000038: return "IMG_FMT_16_16_16_16_SINT";
    case 0x00000039: return "IMG_FMT_16_16_16_16_FLOAT";
    case 0x0000003a: return "IMG_FMT_32_32_32_UINT";
    case 0x0000003b: return "IMG_FMT_32_32_32_SINT";
    case 0x0000003c: return "IMG_FMT_32_32_32_FLOAT";
    case 0x0000003d: return "IMG_FMT_32_32_32_32_UINT";
    case 0x0000003e: return "IMG_FMT_32_32_32_32_SINT";
    case 0x0000003f: return "IMG_FMT_32_32_32_32_FLOAT";
    case 0x00000040: return "IMG_FMT_8_SRGB";
    case 0x00000041: return "IMG_FMT_8_8_SRGB";
    case 0x00000042: return "IMG_FMT_8_8_8_8_SRGB";
    case 0x00000043: return "IMG_FMT_5_9_9_9_FLOAT";
    case 0x00000044: return "IMG_FMT_5_6_5_UNORM";
    case 0x00000045: return "IMG_FMT_1_5_5_5_UNORM";
    case 0x00000046: return "IMG_FMT_5_5_5_1_UNORM";
    case 0x00000047: return "IMG_FMT_4_4_4_4_UNORM";
    case 0x00000048: return "IMG_FMT_4_4_UNORM";
    case 0x00000049: return "IMG_FMT_1_UNORM";
    case 0x0000004a: return "IMG_FMT_1_REVERSED_UNORM";
    case 0x0000004b: return "IMG_FMT_32_FLOAT_CLAMP";
    case 0x0000004c: return "IMG_FMT_8_24_UNORM";
    case 0x0000004d: return "IMG_FMT_8_24_UINT";
    case 0x0000004e: return "IMG_FMT_24_8_UNORM";
    case 0x0000004f: return "IMG_FMT_24_8_UINT";
    case 0x00000050: return "IMG_FMT_X24_8_32_UINT";
    case 0x00000051: return "IMG_FMT_X24_8_32_FLOAT";
    case 0x00000052: return "IMG_FMT_GB_GR_UNORM";
    case 0x00000053: return "IMG_FMT_GB_GR_SNORM";
    case 0x00000054: return "IMG_FMT_GB_GR_UINT";
    case 0x00000055: return "IMG_FMT_GB_GR_SRGB";
    case 0x00000056: return "IMG_FMT_BG_RG_UNORM";
    case 0x00000057: return "IMG_FMT_BG_RG_SNORM";
    case 0x00000058: return "IMG_FMT_BG_RG_UINT";
    case 0x00000059: return "IMG_FMT_BG_RG_SRGB";
    case 0x0000005a: return "IMG_FMT_MM_10_IN_16_UNORM";
    case 0x0000005b: return "IMG_FMT_MM_10_IN_16_UINT";
    case 0x0000005c: return "IMG_FMT_MM_10_IN_16_16_UNORM";
    case 0x0000005d: return "IMG_FMT_MM_10_IN_16_16_UINT";
    case 0x0000005e: return "IMG_FMT_MM_10_IN_16_16_16_16_UNORM";
    case 0x0000005f: return "IMG_FMT_MM_10_IN_16_16_16_16_UINT";
    case 0x0000006d: return "IMG_FMT_BC1_UNORM";
    case 0x0000006e: return "IMG_FMT_BC1_SRGB";
    case 0x0000006f: return "IMG_FMT_BC2_UNORM";
    case 0x00000070: return "IMG_FMT_BC2_SRGB";
    case 0x00000071: return "IMG_FMT_BC3_UNORM";
    case 0x00000072: return "IMG_FMT_BC3_SRGB";
    case 0x00000073: return "IMG_FMT_BC4_UNORM";
    case 0x00000074: return "IMG_FMT_BC4_SNORM";
    case 0x00000075: return "IMG_FMT_BC5_UNORM";
    case 0x00000076: return "IMG_FMT_BC5_SNORM";
    case 0x00000077: return "IMG_FMT_BC6_UFLOAT";
    case 0x00000078: return "IMG_FMT_BC6_SFLOAT";
    case 0x00000079: return "IMG_FMT_BC7_UNORM";
    case 0x0000007a: return "IMG_FMT_BC7_SRGB";
    case 0x000000cd: return "IMG_FMT_YCBCR_UNORM";
    case 0x000000ce: return "IMG_FMT_YCBCR_SRGB";
    case 0x000000cf: return "IMG_FMT_MM_8_UNORM";
    case 0x000000d0: return "IMG_FMT_MM_8_UINT";
    case 0x000000d1: return "IMG_FMT_MM_8_8_UNORM";
    case 0x000000d2: return "IMG_FMT_MM_8_8_UINT";
    case 0x000000d3: return "IMG_FMT_MM_8_8_8_8_UNORM";
    case 0x000000d4: return "IMG_FMT_MM_8_8_8_8_UINT";
    case 0x000000d5: return "IMG_FMT_MM_VYUY8_UNORM";
    case 0x000000d6: return "IMG_FMT_MM_VYUY8_UINT";
    case 0x000000d7: return "IMG_FMT_MM_10_11_11_UNORM";
    case 0x000000d8: return "IMG_FMT_MM_10_11_11_UINT";
    case 0x000000d9: return "IMG_FMT_MM_2_10_10_10_UNORM";
    case 0x000000da: return "IMG_FMT_MM_2_10_10_10_UINT";
    case 0x000000db: return "IMG_FMT_MM_16_16_16_16_UNORM";
    case 0x000000dc: return "IMG_FMT_MM_16_16_16_16_UINT";
    case 0x000000dd: return "IMG_FMT_MM_12_IN_16_UNORM";
    case 0x000000de: return "IMG_FMT_MM_12_IN_16_UINT";
    case 0x000000df: return "IMG_FMT_MM_12_IN_16_16_UNORM";
    case 0x000000e0: return "IMG_FMT_MM_12_IN_16_16_UINT";
    case 0x000000e1: return "IMG_FMT_MM_12_IN_16_16_16_16_UNORM";
    case 0x000000e2: return "IMG_FMT_MM_12_IN_16_16_16_16_UINT";
    case 0x000000e3: return "IMG_FMT_6E4_FLOAT";
    case 0x000000e4: return "IMG_FMT_7E3_FLOAT";
    default:   return "IMG_FMT_UNKNOWN(" + std::to_string(format) + ")";
    }
}

std::string SrdImageRdna3::GetDstSelString(uint32_t dst_sel) const
{
    // Destination selection definitions for RDNA3.
    switch (dst_sel)
    {
    case 0: return "DstSel.0";
    case 1: return "DstSel.1";
    case 4: return "DstSel.X";
    case 5: return "DstSel.Y";
    case 6: return "DstSel.Z";
    case 7: return "DstSel.W";
    default: return "DstSel.UNKNOWN";
    }
}

std::string SrdImageRdna3::GetSwizzleModeString(uint32_t sw_mode) const
{
    // Swizzle mode definitions for RDNA3.
    switch (sw_mode)
    {
    case 0x00000000: return "SW_LINEAR";
    case 0x00000001: return "SW_256B_S";
    case 0x00000002: return "SW_256B_D";
    case 0x00000003: return "SW_256B_R";
    case 0x00000004: return "SW_4KB_Z";
    case 0x00000005: return "SW_4KB_S";
    case 0x00000006: return "SW_4KB_D";
    case 0x00000007: return "SW_4KB_R";
    case 0x00000008: return "SW_64KB_Z";
    case 0x00000009: return "SW_64KB_S";
    case 0x0000000a: return "SW_64KB_D";
    case 0x0000000b: return "SW_64KB_R";
    case 0x0000000c: return "SW_256KB_Z";
    case 0x0000000d: return "SW_256KB_S";
    case 0x0000000e: return "SW_256KB_D";
    case 0x0000000f: return "SW_256KB_R";
    case 0x00000010: return "SW_64KB_Z_T";
    case 0x00000011: return "SW_64KB_S_T";
    case 0x00000012: return "SW_64KB_D_T";
    case 0x00000013: return "SW_64KB_R_T";
    case 0x00000014: return "SW_4KB_Z_X";
    case 0x00000015: return "SW_4KB_S_X";
    case 0x00000016: return "SW_4KB_D_X";
    case 0x00000017: return "SW_4KB_R_X";
    case 0x00000018: return "SW_64KB_Z_X";
    case 0x00000019: return "SW_64KB_S_X";
    case 0x0000001a: return "SW_64KB_D_X";
    case 0x0000001b: return "SW_64KB_R_X";
    case 0x0000001c: return "SW_256KB_Z_X";
    case 0x0000001d: return "SW_256KB_S_X";
    case 0x0000001e: return "SW_256KB_D_X";
    case 0x0000001f: return "SW_256KB_R_X";
    default:   return "SW_UNKNOWN";
    }
}

std::string SrdImageRdna3::GetBcSwizzleString(uint32_t bc_swizzle) const
{
    // BC swizzle definitions for RDNA3.
    switch (bc_swizzle)
    {
    case 0: return "TEX_BC_Swizzle_XYZW";
    case 1: return "TEX_BC_Swizzle_XWYZ";
    case 2: return "TEX_BC_Swizzle_WZYX";
    case 3: return "TEX_BC_Swizzle_WXYZ";
    case 4: return "TEX_BC_Swizzle_ZYXW";
    case 5: return "TEX_BC_Swizzle_YXWZ";
    default: return "TEX_BC_Swizzle_UNKNOWN";
    }
}

nlohmann::json SrdImageRdna3::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Image";
    json["architecture"] = "RDNA3";
    
    const auto fields_data = ExtractFields();
    
    // Populate JSON with interpreted values (same as text output)
    nlohmann::json fields = {
        {"base_address", fields_data.base_address},
        {"llc_noalloc", fields_data.llc_noalloc},
        {"big_page", fields_data.big_page ? true : false},
        {"max_mip", fields_data.max_mip},
        {"format", GetImageFormatString(fields_data.format)},
        {"width", fields_data.width},
        {"height", fields_data.height},
        {"dstsel_x", GetDstSelString(fields_data.dstsel_x)},
        {"dstsel_y", GetDstSelString(fields_data.dstsel_y)},
        {"dstsel_z", GetDstSelString(fields_data.dstsel_z)},
        {"dstsel_w", GetDstSelString(fields_data.dstsel_w)},
        {"base_level", fields_data.base_level},
        {"last_level", fields_data.last_level},
        {"sw_mode", GetSwizzleModeString(fields_data.sw_mode)},
        {"bc_swizzle", GetBcSwizzleString(fields_data.bc_swizzle)},
        {"rsrc_type", GetImageTypeString(fields_data.rsrcType)},
        {"base_array", fields_data.base_array},
        {"array_pitch", fields_data.array_pitch},
        {"min_lod_warn", fields_data.min_lod_warn},
        {"perf_mod", fields_data.perf_mod},
        {"corner_sample", fields_data.corner_sample ? true : false},
        {"linked_resource", fields_data.linked_resource ? true : false},
        {"prt_default", fields_data.prt_default ? true : false},
        {"min_lod", fields_data.min_lod},
        {"iterate_256", fields_data.iterate_256 ? true : false},
        {"sample_pattern_offset", fields_data.sample_pattern_offset},
        {"max_uncompressed_block_size", fields_data.max_uncompressed_block_size},
        {"max_compressed_block_size", fields_data.max_compressed_block_size},
        {"meta_pipe_aligned", fields_data.meta_pipe_aligned ? true : false},
        {"write_compress_en", fields_data.write_compress_en ? true : false},
        {"compression_en", fields_data.compression_en ? true : false},
        {"alpha_is_on_msb", fields_data.alpha_is_on_msb ? true : false},
        {"color_transform", fields_data.color_transform ? true : false},
        {"meta_data_addr", fields_data.meta_data_addr}
    };

    // Add depth field with appropriate interpretation based on resource type
    if (IsDepthPitch(fields_data.rsrcType))
    {
        fields["pitch"] = fields_data.depth;
    }
    else if (fields_data.rsrcType == SQ_RSRC_IMG_3D)
    {
        fields["depth_of_mip0"] = fields_data.depth;
    }
    else
    {
        fields["last_array"] = fields_data.depth;
    }

    json["fields"] = fields;
    return json;
}

SrdSamplerRdna3::SrdSamplerRdna3(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdSamplerRdna3::SamplerFields SrdSamplerRdna3::ExtractFields() const
{
    SamplerFields fields;
    fields.clampX             = ExtractBitsFull(0, 3);
    fields.clampY             = ExtractBitsFull(3, 3);
    fields.clampZ             = ExtractBitsFull(6, 3);
    fields.max_aniso_ratio    = ExtractBitsFull(9, 3);
    fields.depth_compare_func = ExtractBitsFull(12, 3);
    fields.force_unnormalized = ExtractBitsFull(15, 1);
    fields.aniso_threshold    = ExtractBitsFull(16, 3);
    fields.mc_coord_trunc     = ExtractBitsFull(19, 1);
    fields.force_degamma      = ExtractBitsFull(20, 1);
    fields.aniso_bias         = ExtractBitsFull(21, 6);
    fields.trunc_coord        = ExtractBitsFull(27, 1);
    fields.disable_cube_wrap  = ExtractBitsFull(28, 1);
    fields.filter_mode        = ExtractBitsFull(29, 2);
    fields.skip_degamma       = ExtractBitsFull(31, 1);
    fields.min_lod            = ExtractBitsFull(32, 12);
    fields.max_lod            = ExtractBitsFull(44, 12);
    fields.perf_mip           = ExtractBitsFull(56, 4);
    fields.perf_z             = ExtractBitsFull(60, 4);
    fields.lod_bias           = ExtractBitsFull(64, 14);
    fields.lod_bias_sec       = ExtractBitsFull(78, 6);
    fields.xy_mag_filter      = ExtractBitsFull(84, 2);
    fields.xy_min_filter      = ExtractBitsFull(86, 2);
    fields.z_filter           = ExtractBitsFull(88, 2);
    fields.mip_filter         = ExtractBitsFull(90, 2);
    fields.aniso_override     = ExtractBitsFull(93, 1);
    fields.blend_zero_prt     = ExtractBitsFull(94, 1);
    fields.border_color_ptr   = ExtractBitsFull(114, 12);
    fields.border_color_type  = ExtractBitsFull(126, 2);
    return fields;
}

std::string SrdSamplerRdna3::ToString() const
{
    std::stringstream ss;
    ss << "Sampler (" << kArchitectureName << "):\n";
    
    const auto fields = ExtractFields();

    ss << "  " << kStrSamplerClampX << ": " << GetClampString(fields.clampX) << "\n";
    ss << "  " << kStrSamplerClampY << ": " << GetClampString(fields.clampY) << "\n";
    ss << "  " << kStrSamplerClampZ << ": " << GetClampString(fields.clampZ) << "\n";
    ss << "  " << kStrSamplerMaxAnisoRatio << ": " << GetAnisoString(fields.max_aniso_ratio) << "\n";
    ss << "  " << kStrSamplerDepthCompareFunc << ": " << GetDepthCompareString(fields.depth_compare_func) << "\n";
    ss << "  " << kStrSamplerForceUnnormalized << ": " << (fields.force_unnormalized ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerAnisoThreshold << ": " << std::dec << fields.aniso_threshold << "\n";
    ss << "  " << kStrSamplerMcCoordTrunc << ": " << (fields.mc_coord_trunc ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerForceDegamma << ": " << (fields.force_degamma ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerAnisoBias << ": " << fields.aniso_bias << "\n";
    ss << "  " << kStrSamplerTruncCoord << ": " << (fields.trunc_coord ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerDisableCubeWrap << ": " << (fields.disable_cube_wrap ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerFilterMode << ": " << GetFilterModeString(fields.filter_mode) << "\n";
    ss << "  " << kStrSamplerSkipDegamma << ": " << (fields.skip_degamma ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerMinLod << ": " << std::dec << fields.min_lod << "\n";
    ss << "  " << kStrSamplerMaxLod << ": " << static_cast<float>(fields.max_lod) / 256.0f << "\n";
    ss << "  " << "Perf_mip" << ": " << std::dec << fields.perf_mip << "\n";
    ss << "  " << "Perf_z" << ": " << std::dec << fields.perf_z << "\n";
    ss << "  " << kStrSamplerLodBias << ": " << std::dec << fields.lod_bias << "\n";
    ss << "  " << kStrSamplerLodBiasSec << ": " << std::dec << fields.lod_bias_sec << "\n";
    ss << "  " << kStrSamplerXyMagFilter << ": " << GetXYFilterString(fields.xy_mag_filter) << "\n";
    ss << "  " << kStrSamplerXyMinFilter << ": " << GetXYFilterString(fields.xy_min_filter) << "\n";
    ss << "  " << kStrSamplerZFilter << ": " << GetZFilterString(fields.z_filter) << "\n";
    ss << "  " << kStrSamplerMipFilter << ": " << GetMipFilterString(fields.mip_filter) << "\n";
    ss << "  " << "Aniso_override" << ": " << (fields.aniso_override ? "true" : "false") << "\n";
    ss << "  " << "Blend_zero_PRT" << ": " << (fields.blend_zero_prt ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerBorderColorPtr << ": 0x" << std::hex << fields.border_color_ptr << "\n";
    ss << "  " << kStrSamplerBorderColorType << ": " << GetBorderColorTypeString(fields.border_color_type) << "\n";

    return ss.str();
}

std::string SrdSamplerRdna3::GetClampString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_WRAP";
    case 0x00000001: return "SQ_TEX_MIRROR";
    case 0x00000002: return "SQ_TEX_CLAMP_LAST_TEXEL";
    case 0x00000003: return "SQ_TEX_MIRROR_ONCE_LAST_TEXEL";
    case 0x00000004: return "SQ_TEX_CLAMP_HALF_BORDER";
    case 0x00000005: return "SQ_TEX_MIRROR_ONCE_HALF_BORDER";
    case 0x00000006: return "SQ_TEX_CLAMP_BORDER";
    case 0x00000007: return "SQ_TEX_MIRROR_ONCE_BORDER";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetAnisoString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_ANISO_RATIO_1";
    case 0x00000001: return "SQ_TEX_ANISO_RATIO_2";
    case 0x00000002: return "SQ_TEX_ANISO_RATIO_4";
    case 0x00000003: return "SQ_TEX_ANISO_RATIO_8";
    case 0x00000004: return "SQ_TEX_ANISO_RATIO_16";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetDepthCompareString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "TEX_DepthCompareFunction_Never";
    case 0x00000001: return "TEX_DepthCompareFunction_Less";
    case 0x00000002: return "TEX_DepthCompareFunction_Equal";
    case 0x00000003: return "TEX_DepthCompareFunction_LessEqual";
    case 0x00000004: return "TEX_DepthCompareFunction_Greater";
    case 0x00000005: return "TEX_DepthCompareFunction_NotEqual";
    case 0x00000006: return "TEX_DepthCompareFunction_GreaterEqual";
    case 0x00000007: return "TEX_DepthCompareFunction_Always";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetXYFilterString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_XY_FILTER_POINT";
    case 0x00000001: return "SQ_TEX_XY_FILTER_BILINEAR";
    case 0x00000002: return "SQ_TEX_XY_FILTER_ANISO_POINT";
    case 0x00000003: return "SQ_TEX_XY_FILTER_ANISO_BILINEAR";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetZFilterString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_Z_FILTER_NONE";
    case 0x00000001: return "SQ_TEX_Z_FILTER_POINT";
    case 0x00000002: return "SQ_TEX_Z_FILTER_LINEAR";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetMipFilterString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_MIP_FILTER_NONE";
    case 0x00000001: return "SQ_TEX_MIP_FILTER_POINT";
    case 0x00000002: return "SQ_TEX_MIP_FILTER_LINEAR";
    case 0x00000003: return "SQ_TEX_MIP_FILTER_POINT_ANISO_ADJ";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetBorderColorTypeString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_TEX_BORDER_COLOR_TRANS_BLACK";
    case 0x00000001: return "SQ_TEX_BORDER_COLOR_OPAQUE_BLACK";
    case 0x00000002: return "SQ_TEX_BORDER_COLOR_OPAQUE_WHITE";
    case 0x00000003: return "SQ_TEX_BORDER_COLOR_REGISTER";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna3::GetFilterModeString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_IMG_FILTER_MODE_BLEND";
    case 0x00000001: return "SQ_IMG_FILTER_MODE_MIN";
    case 0x00000002: return "SQ_IMG_FILTER_MODE_MAX";
    default: return "UNKNOWN";
    }
}

nlohmann::json SrdSamplerRdna3::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Sampler";
    json["architecture"] = "RDNA3";
    
    const auto fields = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    json["fields"] = {
        {"clampX", GetClampString(fields.clampX)},
        {"clampY", GetClampString(fields.clampY)},
        {"clampZ", GetClampString(fields.clampZ)},
        {"max_aniso_ratio", GetAnisoString(fields.max_aniso_ratio)},
        {"depth_compare_func", GetDepthCompareString(fields.depth_compare_func)},
        {"force_unnormalized", fields.force_unnormalized ? true : false},
        {"aniso_threshold", fields.aniso_threshold},
        {"mc_coord_trunc", fields.mc_coord_trunc ? true : false},
        {"force_degamma", fields.force_degamma ? true : false},
        {"aniso_bias", fields.aniso_bias},
        {"trunc_coord", fields.trunc_coord ? true : false},
        {"disable_cube_wrap", fields.disable_cube_wrap ? true : false},
        {"filter_mode", GetFilterModeString(fields.filter_mode)},
        {"skip_degamma", fields.skip_degamma ? true : false},
        {"min_lod", fields.min_lod},
        {"max_lod", static_cast<float>(fields.max_lod) / 256.0f},
        {"perf_mip", fields.perf_mip},
        {"perf_z", fields.perf_z},
        {"lod_bias", fields.lod_bias},
        {"lod_bias_sec", fields.lod_bias_sec},
        {"xy_mag_filter", GetXYFilterString(fields.xy_mag_filter)},
        {"xy_min_filter", GetXYFilterString(fields.xy_min_filter)},
        {"z_filter", GetZFilterString(fields.z_filter)},
        {"mip_filter", GetMipFilterString(fields.mip_filter)},
        {"aniso_override", fields.aniso_override ? true : false},
        {"blend_zero_prt", fields.blend_zero_prt ? true : false},
        {"border_color_ptr", fields.border_color_ptr},
        {"border_color_type", GetBorderColorTypeString(fields.border_color_type)}
    };

    return json;
}

SrdBvhRdna3::SrdBvhRdna3(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdBvhRdna3::BvhFields SrdBvhRdna3::ExtractFields() const
{
    BvhFields fields;
    
    // Base address (bits 47:0 from dwords 0 and 1).
    fields.base_address = (static_cast<uint64_t>(GetDword(0)) | 
                           (static_cast<uint64_t>(ExtractBits(1, 0, 15)) << 32)) << 8;
    fields.box_sorting_heuristic = ExtractBitsFull(53, 2);
    fields.box_grow_value        = ExtractBitsFull(55, 8);
    fields.box_sort_en           = ExtractBitsFull(63, 1);
    
    // Size (bits 105:64) from dword 2 and 3.
    fields.size = (static_cast<uint64_t>(GetDword(2)) | 
                   (static_cast<uint64_t>(ExtractBits(3, 0, 9)) << 32)) + 1;
    fields.pointer_flags        = ExtractBitsFull(119, 1);
    fields.triangle_return_mode = ExtractBitsFull(120, 1);
    fields.llc_noalloc          = ExtractBitsFull(121, 2);
    fields.big_page             = ExtractBitsFull(123, 1);
    return fields;
}

std::string SrdBvhRdna3::ToString() const
{
    std::stringstream ss;
    ss << "BVH (" << kArchitectureName << "):\n";
    
    const auto fields = ExtractFields();
    const std::string type = "BVH";

    ss << "  " << kStrBvhBaseAddress << ": 0x" << std::hex << std::setw(16) << std::setfill('0') << fields.base_address << "\n";
    ss << "  " << kStrBvhBoxSortingHeuristic << ": " << GetBoxSortingHeuristicString(fields.box_sorting_heuristic) << "\n";
    ss << "  " << kStrBvhBoxGrowValue << ": " << std::dec << fields.box_grow_value << "\n";
    ss << "  " << kStrBvhBoxSortEn << ": " << (fields.box_sort_en ? "true" : "false") << "\n";
    ss << "  " << kStrBvhSize << ": 0x" << std::hex << fields.size << " bytes\n";
    ss << "  " << kStrBvhPointerFlags << ": " << (fields.pointer_flags ? "true" : "false") << "\n";
    ss << "  " << kStrBvhTriangleReturnMode << ": " << (fields.triangle_return_mode ? "true" : "false") << "\n";
    ss << "  " << "LLC_NoAlloc" << ": " << fields.llc_noalloc << "\n";
    ss << "  " << "Big_page" << ": " << (fields.big_page ? "true" : "false") << "\n";
    ss << "  " << kStrBvhType << ": " << type << "\n";

    return ss.str();
}

std::string SrdBvhRdna3::GetBoxSortingHeuristicString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x0: return "ClosestFirst";
    case 0x1: return "LargestFirst";
    case 0x2: return "ClosestMidPoint";
    case 0x3: return "Disabled";
    default: return "UNKNOWN";
    }
}

nlohmann::json SrdBvhRdna3::ToJson() const
{
    nlohmann::json json;
    json["type"] = "BVH";
    json["architecture"] = "RDNA3";
    
    const auto fields = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    json["fields"] = {
        {"base_address", fields.base_address},
        {"box_sorting_heuristic", GetBoxSortingHeuristicString(fields.box_sorting_heuristic)},
        {"box_grow_value", fields.box_grow_value},
        {"box_sort_en", fields.box_sort_en ? true : false},
        {"size", fields.size},
        {"pointer_flags", fields.pointer_flags ? true : false},
        {"triangle_return_mode", fields.triangle_return_mode ? true : false},
        {"llc_noalloc", fields.llc_noalloc},
        {"big_page", fields.big_page ? true : false}
    };

    return json;
}

std::unique_ptr<ShaderResourceDescriptor> SrdDisassemblerRdna3::CreateSrd(const std::vector<uint32_t>& data, SrdType type) const
{
    switch (type)
    {
    case SrdType::kBuffer:
        return std::make_unique<SrdBufferRdna3>(data);
    case SrdType::kImage:
        return std::make_unique<SrdImageRdna3>(data);
    case SrdType::kSampler:
        return std::make_unique<SrdSamplerRdna3>(data);
    case SrdType::kBvh:
        return std::make_unique<SrdBvhRdna3>(data);
    default:
        return nullptr;
    }
}

std::string SrdDisassemblerRdna3::DisassembleSrd(const std::vector<uint32_t>& data, SrdType type) const
{
    auto srd = CreateSrd(data, type);
    if (srd != nullptr)
    {
        return srd->ToString();
    }
    return "Unknown SRD type for RDNA3";
}

nlohmann::json SrdDisassemblerRdna3::DisassembleSrdJson(const std::vector<uint32_t>& data, SrdType type) const
{
    auto srd = CreateSrd(data, type);
    if (srd != nullptr)
    {
        return srd->ToJson();
    }
    
    nlohmann::json error_json;
    error_json["error"] = "Unknown SRD type for RDNA3";
    error_json["architecture"] = "RDNA3";
    return error_json;
}
