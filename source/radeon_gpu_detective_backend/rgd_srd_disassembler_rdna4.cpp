//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief RDNA4 SRD (Shader Resource Descriptor) disassembler implementation.
//============================================================================================

#include "rgd_srd_disassembler_rdna4.h"

// C++.
#include <iomanip>
#include <sstream>

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// Architecture name constants.
static const char* kArchitectureName = "RDNA4";

SrdBufferRdna4::SrdBufferRdna4(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdBufferRdna4::BufferFields SrdBufferRdna4::ExtractFields() const
{
    BufferFields fields;

    // Base address (bits 47:0 from dwords 0 and 1).
    fields.base_address = static_cast<uint64_t>(GetDword(0)) | 
                          (static_cast<uint64_t>(ExtractBits(1, 0, 15)) << 32);
    fields.stride                  = ExtractBitsFull(48, 14);
    fields.swizzle_enable          = ExtractBitsFull(62, 2);
    fields.num_records             = ExtractBitsFull(64, 32);
    fields.dstsel_x                = ExtractBitsFull(96, 3);
    fields.dstsel_y                = ExtractBitsFull(99, 3);
    fields.dstsel_z                = ExtractBitsFull(102, 3);
    fields.dstsel_w                = ExtractBitsFull(105, 3);
    fields.format                  = ExtractBitsFull(108, 6);
    fields.stride_scale            = ExtractBitsFull(114, 2);
    fields.index_stride            = ExtractBitsFull(117, 2);
    fields.add_tid_enable          = ExtractBitsFull(119, 1);
    fields.write_compress_en       = ExtractBitsFull(120, 1);
    fields.compression_en          = ExtractBitsFull(121, 1);
    fields.compression_access_mode = ExtractBitsFull(122, 2);
    fields.oob_select              = ExtractBitsFull(124, 2);
    return fields;
}

std::string SrdBufferRdna4::ToString() const
{
    std::stringstream ss;
    ss << "Buffer (" << kArchitectureName << "):\n";
    
    // Extract individual Buffer SRD fields.
    const auto fields = ExtractFields();
    const std::string type("Buffer");

    ss << "  " << kStrBufferBaseAddr << ": 0x" << std::hex << fields.base_address << "\n";
    ss << "  " << kStrBufferStride << ": 0x" << std::hex << fields.stride << "\n";
    ss << "  " << kStrBufferSwizzleEnable << ": " << std::dec << fields.swizzle_enable << "\n";
    ss << "  " << kStrBufferNumRecords << ": 0x" << std::hex << fields.num_records << "\n";
    ss << "  " << kStrBufferDstSelX << ": " << GetDstSelString(fields.dstsel_x) << "\n";
    ss << "  " << kStrBufferDstSelY << ": " << GetDstSelString(fields.dstsel_y) << "\n";
    ss << "  " << kStrBufferDstSelZ << ": " << GetDstSelString(fields.dstsel_z) << "\n";
    ss << "  " << kStrBufferDstSelW << ": " << GetDstSelString(fields.dstsel_w) << "\n";
    ss << "  " << kStrBufferFormat << ": " << GetBufferFormatString(fields.format) << "\n";
    ss << "  " << kStrBufferStrideScale << ": " << std::dec << fields.stride_scale << "\n";
    ss << "  " << kStrBufferIndexStride << ": " << GetIndexStrideString(fields.index_stride) << "\n";
    ss << "  " << kStrBufferAddTidEnable << ": " << (fields.add_tid_enable ? "true" : "false") << "\n";
    ss << "  " << kStrBufferWriteCompressEn << ": " << (fields.write_compress_en ? "true" : "false") << "\n";
    ss << "  " << kStrBufferCompressionEn << ": " << (fields.compression_en ? "true" : "false") << "\n";
    ss << "  " << kStrBufferCompressionAccessMode << ": " << std::dec << fields.compression_access_mode << "\n";
    ss << "  " << kStrBufferOobSelect << ": " << GetOobSelectString(fields.oob_select) << "\n";
    ss << "  " << kStrBufferType << ": " << type << "\n";

    return ss.str();
}

std::string SrdBufferRdna4::GetBufferFormatString(uint32_t format) const
{
    // Buffer format definitions for RDNA4 (GFX12).
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

std::string SrdBufferRdna4::GetIndexStrideString(uint32_t index_stride) const
{
    // Index stride definitions for RDNA4.
    switch (index_stride)
    {    
    case 0x0: return "IndexStride_8B";
    case 0x1: return "IndexStride_16B";
    case 0x2: return "IndexStride_32B";
    case 0x3: return "IndexStride_64B";
    default: return "UNKNOWN";
    }
}

std::string SrdBufferRdna4::GetDstSelString(uint32_t dst_sel) const
{
    // Destination selection definitions for RDNA4.
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

std::string SrdBufferRdna4::GetOobSelectString(uint32_t oob_select) const
{
    // Out-of-bounds selection definitions for RDNA4.
    switch (oob_select)
    {
    case 0: return "IndexAndOffset";
    case 1: return "IndexOnly";
    case 2: return "NumRecords0";
    case 3: return "Complete";
    default: return "UNKNOWN";
    }
}

nlohmann::json SrdBufferRdna4::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Buffer";
    json["architecture"] = "RDNA4";
    
    // Extract fields individual Buffer SRD fields.
    BufferFields fields = ExtractFields();

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
        {"stride_scale", fields.stride_scale},
        {"index_stride", GetIndexStrideString(fields.index_stride)},
        {"add_tid_enable", fields.add_tid_enable ? true : false},
        {"write_compress_en", fields.write_compress_en ? true : false},
        {"compression_en", fields.compression_en ? true : false},
        {"compression_access_mode", fields.compression_access_mode},
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
    return (rsrc_type == SQ_RSRC_IMG_1D 
        || rsrc_type == SQ_RSRC_IMG_2D 
        || rsrc_type == SQ_RSRC_IMG_2D_MSAA 
        || rsrc_type == SQ_RSRC_IMG_3D);
}

static bool IsDepthPitch(const uint32_t rsrc_type)
{
    return (rsrc_type == SQ_RSRC_IMG_1D || rsrc_type == SQ_RSRC_IMG_2D || rsrc_type == SQ_RSRC_IMG_2D_MSAA );
}

SrdImageRdna4::SrdImageRdna4(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdImageRdna4::ImageFields SrdImageRdna4::ExtractFields() const
{
    ImageFields fields;

    // Base address (bits 39:0 from dwords 0 and 1, shifted left by 8).
    fields.base_address = (static_cast<uint64_t>(GetDword(0)) | 
                          (static_cast<uint64_t>(ExtractBits(1, 0, 7)) << 32)) << 8;
    fields.max_mip                      = ExtractBitsFull(44, 5);
    fields.format                       = ExtractBitsFull(49, 8);
    fields.base_level                   = ExtractBitsFull(57, 5);
    fields.width                        = ExtractBitsFull(62, 16) + 1;
    fields.height                       = ExtractBitsFull(78, 16) + 1;
    fields.dstsel_x                     = ExtractBitsFull(96, 3);
    fields.dstsel_y                     = ExtractBitsFull(99, 3);
    fields.dstsel_z                     = ExtractBitsFull(102, 3);
    fields.dstsel_w                     = ExtractBitsFull(105, 3);
    fields.no_edge_clamp                = ExtractBitsFull(108, 1);
    fields.last_level                   = ExtractBitsFull(111, 5);
    fields.sw_mode                      = ExtractBitsFull(116, 5);
    fields.bc_swizzle                   = ExtractBitsFull(121, 3);
    fields.rsrcType                     = ExtractBitsFull(124, 4);

    // special processing case
    fields.depth                        = ExtractBitsFull(128, 16);
    if (IsDepthAddOne(fields.rsrcType))
    {
        fields.depth += 1;
    }
    fields.base_array                   = ExtractBitsFull(144, 14);
    fields.uav3d                        = ExtractBitsFull(164, 1);
    fields.min_lod_warn                 = ExtractBitsFull(165, 13);
    fields.perf_mod                     = ExtractBitsFull(180, 3);
    fields.corner_sample                = ExtractBitsFull(183, 1);
    fields.linked_resource              = ExtractBitsFull(184, 1);
    fields.min_lod                      = ExtractBitsFull(186, 13);
    fields.iterate_256                  = ExtractBitsFull(202, 1);
    fields.sample_pattern_offset        = ExtractBitsFull(203, 4);
    fields.max_uncompressed_block_size  = ExtractBitsFull(207, 1);
    fields.max_compressed_block_size    = ExtractBitsFull(209, 2);
    fields.write_compress_en            = ExtractBitsFull(212, 1);
    fields.compression_en               = ExtractBitsFull(213, 1);
    fields.compression_access_mode      = ExtractBitsFull(214, 2);
    fields.speculative_read             = ExtractBitsFull(216, 2);

    return fields;
}

std::string SrdImageRdna4::ToString() const
{
    std::stringstream ss;
    ss << "Image (" << kArchitectureName << "):\n";
    
    // Extract individual Image SRD fields.
    ImageFields fields = ExtractFields();
    
    ss << "  " << kStrImageBaseAddr << ": 0x" << std::hex << fields.base_address << "\n";
    ss << "  " << kStrImageMaxMip << ": 0x" << std::hex << fields.max_mip << "\n";
    ss << "  " << kStrImageFormat << ": " << GetImageFormatString(fields.format) << "\n";
    ss << "  " << kStrImageBaseLevel << ": 0x" << std::hex << fields.base_level << "\n";
    ss << "  " << kStrImageWidth << ": 0x" << std::hex << fields.width << "\n";
    ss << "  " << kStrImageHeight << ": 0x" << std::hex << fields.height << "\n";
    ss << "  " << kStrImageDstSelX << ": " << GetDstSelString(fields.dstsel_x) << "\n";
    ss << "  " << kStrImageDstSelY << ": " << GetDstSelString(fields.dstsel_y) << "\n";
    ss << "  " << kStrImageDstSelZ << ": " << GetDstSelString(fields.dstsel_z) << "\n";
    ss << "  " << kStrImageDstSelW << ": " << GetDstSelString(fields.dstsel_w) << "\n";
    ss << "  " << "No_edge_clamp" << ": " << (fields.no_edge_clamp ? "true" : "false") << "\n";
    ss << "  " << kStrImageLastLevel << ": 0x" << std::hex << fields.last_level << "\n";
    ss << "  " << "Swizzle mode" << ": " << GetSwizzleModeString(fields.sw_mode) << "\n";
    ss << "  " << kStrImageBcSwizzle << ": " << GetBcSwizzleString(fields.bc_swizzle) << "\n";
    ss << "  " << kStrImageType << ": " << GetImageTypeString(fields.rsrcType) << "\n";
    if (IsDepthPitch(fields.rsrcType))
    {
        ss << "  Pitch: 0x" << std::hex << fields.depth << "\n";
    }
    else if (fields.rsrcType == SQ_RSRC_IMG_3D)
    {
        ss << "  " << kStrImageDepth << ": 0x" << std::hex << fields.depth << "\n";
    }
    else
    {
        ss << "  " << "Last_array" << ": 0x" << std::hex << fields.depth << "\n";
    }
    ss << "  " << kStrImageBaseArray << ": 0x" << std::hex << fields.base_array << "\n";
    ss << "  " << kStrImageUav3d << ": " << (fields.uav3d ? "true" : "false") << "\n";
    ss << "  " << kStrImageMinLodWarn4 << ": 0x" << std::hex << fields.min_lod_warn << "\n";
    ss << "  Perf_mod: 0x" << std::hex << fields.perf_mod << "\n";
    ss << "  Corner_sample: " << (fields.corner_sample ? "true" : "false") << "\n";
    ss << "  Linked_resource: " << (fields.linked_resource ? "true" : "false") << "\n";
    ss << "  " << kStrImageMinLod << ": 0x" << std::hex << fields.min_lod << "\n";
    ss << "  Iterate_256: " << (fields.iterate_256 ? "true" : "false") << "\n";
    ss << "  Sample_pattern_offset: " << std::dec << fields.sample_pattern_offset << "\n";
    ss << "  Max_uncompressed_block_size: " << fields.max_uncompressed_block_size << "\n";
    ss << "  Max_compressed_block_size: " << std::dec << fields.max_compressed_block_size << "\n";
    ss << "  Write_compress_en: " << (fields.write_compress_en ? "true" : "false") << "\n";
    ss << "  Compression_en: " << (fields.compression_en ? "true" : "false") << "\n";
    ss << "  Compression_access_mode: " << std::dec << fields.compression_access_mode << "\n";
    ss << "  Speculative_read: " << std::dec << fields.speculative_read << "\n";


    return ss.str();
}

std::string SrdImageRdna4::GetImageTypeString(uint32_t type) const
{
    // Image type definitions based on GPU specifications.
    switch (type)
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

std::string SrdImageRdna4::GetImageFormatString(uint32_t format) const
{
    // Image format definitions for RDNA4.
    switch (format)
    {
    case 0x00: return "IMG_FMT_INVALID";
    case 0x01: return "IMG_FMT_8_UNORM";
    case 0x02: return "IMG_FMT_8_SNORM";
    case 0x03: return "IMG_FMT_8_USCALED";
    case 0x04: return "IMG_FMT_8_SSCALED";
    case 0x05: return "IMG_FMT_8_UINT";
    case 0x06: return "IMG_FMT_8_SINT";
    case 0x07: return "IMG_FMT_16_UNORM";
    case 0x08: return "IMG_FMT_16_SNORM";
    case 0x09: return "IMG_FMT_16_USCALED";
    case 0x0a: return "IMG_FMT_16_SSCALED";
    case 0x0b: return "IMG_FMT_16_UINT";
    case 0x0c: return "IMG_FMT_16_SINT";
    case 0x0d: return "IMG_FMT_16_FLOAT";
    case 0x0e: return "IMG_FMT_8_8_UNORM";
    case 0x0f: return "IMG_FMT_8_8_SNORM";
    case 0x10: return "IMG_FMT_8_8_USCALED";
    case 0x11: return "IMG_FMT_8_8_SSCALED";
    case 0x12: return "IMG_FMT_8_8_UINT";
    case 0x13: return "IMG_FMT_8_8_SINT";
    case 0x14: return "IMG_FMT_32_UINT";
    case 0x15: return "IMG_FMT_32_SINT";
    case 0x16: return "IMG_FMT_32_FLOAT";
    case 0x17: return "IMG_FMT_16_16_UNORM";
    case 0x18: return "IMG_FMT_16_16_SNORM";
    case 0x19: return "IMG_FMT_16_16_USCALED";
    case 0x1a: return "IMG_FMT_16_16_SSCALED";
    case 0x1b: return "IMG_FMT_16_16_UINT";
    case 0x1c: return "IMG_FMT_16_16_SINT";
    case 0x1d: return "IMG_FMT_16_16_FLOAT";
    case 0x1e: return "IMG_FMT_10_11_11_FLOAT";
    case 0x1f: return "IMG_FMT_11_11_10_FLOAT";
    case 0x20: return "IMG_FMT_10_10_10_2_UNORM";
    case 0x21: return "IMG_FMT_10_10_10_2_SNORM";
    case 0x22: return "IMG_FMT_10_10_10_2_UINT";
    case 0x23: return "IMG_FMT_10_10_10_2_SINT";
    case 0x24: return "IMG_FMT_2_10_10_10_UNORM";
    case 0x25: return "IMG_FMT_2_10_10_10_SNORM";
    case 0x26: return "IMG_FMT_2_10_10_10_USCALED";
    case 0x27: return "IMG_FMT_2_10_10_10_SSCALED";
    case 0x28: return "IMG_FMT_2_10_10_10_UINT";
    case 0x29: return "IMG_FMT_2_10_10_10_SINT";
    case 0x2A: return "IMG_FMT_8_8_8_8_UNORM";
    case 0x2B: return "IMG_FMT_8_8_8_8_SNORM";
    case 0x2C: return "IMG_FMT_8_8_8_8_USCALED";
    case 0x2D: return "IMG_FMT_8_8_8_8_SSCALED";
    case 0x2E: return "IMG_FMT_8_8_8_8_UINT";
    case 0x2F: return "IMG_FMT_8_8_8_8_SINT";
    case 0x30: return "IMG_FMT_32_32_UINT";
    case 0x31: return "IMG_FMT_32_32_SINT";
    case 0x32: return "IMG_FMT_32_32_FLOAT";
    case 0x33: return "IMG_FMT_16_16_16_16_UNORM";
    case 0x34: return "IMG_FMT_16_16_16_16_SNORM";
    case 0x35: return "IMG_FMT_16_16_16_16_USCALED";
    case 0x36: return "IMG_FMT_16_16_16_16_SSCALED";
    case 0x37: return "IMG_FMT_16_16_16_16_UINT";
    case 0x38: return "IMG_FMT_16_16_16_16_SINT";
    case 0x39: return "IMG_FMT_16_16_16_16_FLOAT";
    case 0x3A: return "IMG_FMT_32_32_32_UINT";
    case 0x3B: return "IMG_FMT_32_32_32_SINT";
    case 0x3C: return "IMG_FMT_32_32_32_FLOAT";
    case 0x3D: return "IMG_FMT_32_32_32_32_UINT";
    case 0x3E: return "IMG_FMT_32_32_32_32_SINT";
    case 0x3F: return "IMG_FMT_32_32_32_32_FLOAT";
    case 0x40: return "IMG_FMT_8_SRGB";
    case 0x41: return "IMG_FMT_8_8_SRGB";
    case 0x42: return "IMG_FMT_8_8_8_8_SRGB";
    case 0x43: return "IMG_FMT_5_9_9_9_FLOAT";
    case 0x44: return "IMG_FMT_5_6_5_UNORM";
    case 0x45: return "IMG_FMT_1_5_5_5_UNORM";
    case 0x46: return "IMG_FMT_5_5_5_1_UNORM";
    case 0x47: return "IMG_FMT_4_4_4_4_UNORM";
    case 0x48: return "IMG_FMT_4_4_UNORM";
    case 0x49: return "IMG_FMT_1_UNORM";
    case 0x4A: return "IMG_FMT_1_REVERSED_UNORM";
    case 0x4B: return "IMG_FMT_32_FLOAT_CLAMP";
    case 0x4C: return "IMG_FMT_8_24_UNORM";
    case 0x4D: return "IMG_FMT_8_24_UINT";
    case 0x4E: return "IMG_FMT_24_8_UNORM";
    case 0x4F: return "IMG_FMT_24_8_UINT";
    case 0x50: return "IMG_FMT_X24_8_32_UINT";
    case 0x51: return "IMG_FMT_X24_8_32_FLOAT";
    case 0x52: return "IMG_FMT_GB_GR_UNORM";
    case 0x53: return "IMG_FMT_GB_GR_SNORM";
    case 0x54: return "IMG_FMT_GB_GR_UINT";
    case 0x55: return "IMG_FMT_GB_GR_SRGB";
    case 0x56: return "IMG_FMT_BG_RG_UNORM";
    case 0x57: return "IMG_FMT_BG_RG_SNORM";
    case 0x58: return "IMG_FMT_BG_RG_UINT";
    case 0x59: return "IMG_FMT_BG_RG_SRGB";
    case 0x5A: return "IMG_FMT_MM_10_IN_16_UNORM";
    case 0x5B: return "IMG_FMT_MM_10_IN_16_UINT";
    case 0x5C: return "IMG_FMT_MM_10_IN_16_16_UNORM";
    case 0x5D: return "IMG_FMT_MM_10_IN_16_16_UINT";
    case 0x5E: return "IMG_FMT_MM_10_IN_16_16_16_16_UNORM";
    case 0x5F: return "IMG_FMT_MM_10_IN_16_16_16_16_UINT";
    case 0x6D: return "IMG_FMT_BC1_UNORM";
    case 0x6E: return "IMG_FMT_BC1_SRGB";
    case 0x6F: return "IMG_FMT_BC2_UNORM";
    case 0x70: return "IMG_FMT_BC2_SRGB";
    case 0x71: return "IMG_FMT_BC3_UNORM";
    case 0x72: return "IMG_FMT_BC3_SRGB";
    case 0x73: return "IMG_FMT_BC4_UNORM";
    case 0x74: return "IMG_FMT_BC4_SNORM";
    case 0x75: return "IMG_FMT_BC5_UNORM";
    case 0x76: return "IMG_FMT_BC5_SNORM";
    case 0x77: return "IMG_FMT_BC6_UFLOAT";
    case 0x78: return "IMG_FMT_BC6_SFLOAT";
    case 0x79: return "IMG_FMT_BC7_UNORM";
    case 0x7A: return "IMG_FMT_BC7_SRGB";
    case 0xCD: return "IMG_FMT_YCBCR_UNORM";
    case 0xCE: return "IMG_FMT_YCBCR_SRGB";
    case 0xCF: return "IMG_FMT_MM_8_UNORM";
    case 0xD0: return "IMG_FMT_MM_8_UINT";
    case 0xD1: return "IMG_FMT_MM_8_8_UNORM";
    case 0xD2: return "IMG_FMT_MM_8_8_UINT";
    case 0xD3: return "IMG_FMT_MM_8_8_8_8_UNORM";
    case 0xD4: return "IMG_FMT_MM_8_8_8_8_UINT";
    case 0xD5: return "IMG_FMT_MM_VYUY8_UNORM";
    case 0xD6: return "IMG_FMT_MM_VYUY8_UINT";
    case 0xD7: return "IMG_FMT_MM_10_11_11_UNORM";
    case 0xD8: return "IMG_FMT_MM_10_11_11_UINT";
    case 0xD9: return "IMG_FMT_MM_2_10_10_10_UNORM";
    case 0xDA: return "IMG_FMT_MM_2_10_10_10_UINT";
    case 0xDB: return "IMG_FMT_MM_16_16_16_16_UNORM";
    case 0xDC: return "IMG_FMT_MM_16_16_16_16_UINT";
    case 0xDD: return "IMG_FMT_MM_12_IN_16_UNORM";
    case 0xDE: return "IMG_FMT_MM_12_IN_16_UINT";
    case 0xDF: return "IMG_FMT_MM_12_IN_16_16_UNORM";
    case 0xE0: return "IMG_FMT_MM_12_IN_16_16_UINT";
    case 0xE1: return "IMG_FMT_MM_12_IN_16_16_16_16_UNORM";
    case 0xE2: return "IMG_FMT_MM_12_IN_16_16_16_16_UINT";
    case 0xE3: return "IMG_FMT_6E4_FLOAT";
    case 0xE4: return "IMG_FMT_7E3_FLOAT";
    default:   return "IMG_FMT_UNKNOWN(" + std::to_string(format) + ")";
    }
}

std::string SrdImageRdna4::GetDstSelString(uint32_t dst_sel) const
{
    // Destination selection definitions for RDNA4.
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

std::string SrdImageRdna4::GetSwizzleModeString(uint32_t sw_mode) const
{
    // Swizzle mode definitions for RDNA4.
    switch (sw_mode)
    {
    case 0x00: return "SW_LINEAR";
    case 0x01: return "SW_256B_2D";
    case 0x02: return "SW_4KB_2D";
    case 0x03: return "SW_64KB_2D";
    case 0x04: return "SW_256KB_2D";
    case 0x05: return "SW_4KB_3D";
    case 0x06: return "SW_64KB_3D";
    case 0x07: return "SW_256KB_3D";
    default:   return "SW_UNKNOWN";
    }
}

std::string SrdImageRdna4::GetBcSwizzleString(uint32_t bc_swizzle) const
{
    // BC swizzle definitions for RDNA4.
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

SrdSamplerRdna4::SrdSamplerRdna4(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdSamplerRdna4::SamplerFields SrdSamplerRdna4::ExtractFields() const
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
    fields.min_lod            = ExtractBitsFull(32, 13);
    fields.max_lod            = ExtractBitsFull(45, 13);
    fields.perf_z             = ExtractBitsFull(60, 4);
    fields.lod_bias           = ExtractBitsFull(64, 14);
    fields.lod_bias_sec       = ExtractBitsFull(78, 6);
    fields.xy_mag_filter      = ExtractBitsFull(84, 2);
    fields.xy_min_filter      = ExtractBitsFull(86, 2);
    fields.z_filter           = ExtractBitsFull(88, 2);
    fields.mip_filter         = ExtractBitsFull(90, 2);
    fields.aniso_override     = ExtractBitsFull(93, 1);
    fields.perf_mip           = ExtractBitsFull(94, 4);
    fields.border_color_ptr   = ExtractBitsFull(114, 12);
    fields.border_color_type  = ExtractBitsFull(126, 2);

    return fields;
}

std::string SrdSamplerRdna4::ToString() const
{
    std::stringstream ss;
    ss << "Sampler (RDNA4):\n";

    // Extract individual Sampler SRD fields.
    SamplerFields fields = ExtractFields();
    
    ss << "  " << kStrSamplerClampX << ": " << GetClampString(fields.clampX) << " (" << fields.clampX << ")\n";
    ss << "  " << kStrSamplerClampY << ": " << GetClampString(fields.clampY) << " (" << fields.clampY << ")\n";
    ss << "  " << kStrSamplerClampZ << ": " << GetClampString(fields.clampZ) << " (" << fields.clampZ << ")\n";
    ss << "  " << kStrSamplerMaxAnisoRatio << ": " << GetAnisoString(fields.max_aniso_ratio) << "\n";
    ss << "  " << kStrSamplerDepthCompareFunc << ": " << GetDepthCompareString(fields.depth_compare_func) << "\n";
    ss << "  " << kStrSamplerForceUnnormalized << ": " << (fields.force_unnormalized ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerAnisoThreshold << ": " << fields.aniso_threshold << "\n";
    ss << "  " << kStrSamplerMcCoordTrunc << ": " << (fields.mc_coord_trunc ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerForceDegamma << ": " << (fields.force_degamma ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerAnisoBias << ": " << fields.aniso_bias << "\n";
    ss << "  " << kStrSamplerTruncCoord << ": " << (fields.trunc_coord ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerDisableCubeWrap << ": " << (fields.disable_cube_wrap ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerFilterMode << ": " << GetFilterModeString(fields.filter_mode) << " (" << fields.filter_mode << ")\n";
    ss << "  " << kStrSamplerSkipDegamma << ": " << (fields.skip_degamma ? "true" : "false") << "\n";
    ss << "  " << kStrSamplerMinLod << ": " << fields.min_lod << "\n";
    ss << "  " << kStrSamplerMaxLod << ": " << static_cast<float>(fields.max_lod) / 256.0f << "\n";
    ss << "  " << "Perf z" << ": " << fields.perf_z << "\n";
    ss << "  " << kStrSamplerLodBias << ": " << fields.lod_bias << "\n";
    ss << "  " << kStrSamplerLodBiasSec << ": " << fields.lod_bias_sec << "\n";
    ss << "  " << kStrSamplerXyMagFilter << ": " << GetXYFilterString(fields.xy_mag_filter) << " (" << fields.xy_mag_filter << ")\n";
    ss << "  " << kStrSamplerXyMinFilter << ": " << GetXYFilterString(fields.xy_min_filter) << " (" << fields.xy_min_filter << ")\n";
    ss << "  " << kStrSamplerZFilter << ": " << GetZFilterString(fields.z_filter) << " (" << fields.z_filter << ")\n";
    ss << "  " << kStrSamplerMipFilter << ": " << GetMipFilterString(fields.mip_filter) << " (" << fields.mip_filter << ")\n";
    ss << "  " << "Aniso override" << ": " << (fields.aniso_override ? "true" : "false") << "\n";
    ss << "  " << "Perf mip" << ": " << fields.perf_mip << "\n";
    ss << "  " << kStrSamplerBorderColorPtr << ": " << fields.border_color_ptr << "\n";
    ss << "  " << kStrSamplerBorderColorType << ": " << GetBorderColorTypeString(fields.border_color_type) << " (" << fields.border_color_type << ")\n";

    return ss.str();
}

std::string SrdSamplerRdna4::GetFilterModeString(uint32_t mode) const
{
    switch (mode)
    {
    case 0x00000000: return "SQ_IMG_FILTER_MODE_BLEND";
    case 0x00000001: return "SQ_IMG_FILTER_MODE_MIN";
    case 0x00000002: return "SQ_IMG_FILTER_MODE_MAX";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna4::GetClampString(uint32_t mode) const
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

std::string SrdSamplerRdna4::GetAnisoString(uint32_t aniso) const
{
    switch (aniso)
    {
    case 0x00000000: return "SQ_TEX_ANISO_RATIO_1";
    case 0x00000001: return "SQ_TEX_ANISO_RATIO_2";
    case 0x00000002: return "SQ_TEX_ANISO_RATIO_4";
    case 0x00000003: return "SQ_TEX_ANISO_RATIO_8";
    case 0x00000004: return "SQ_TEX_ANISO_RATIO_16";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna4::GetDepthCompareString(uint32_t func) const
{
    switch (func)
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

std::string SrdSamplerRdna4::GetXYFilterString(uint32_t filter) const
{
    switch (filter)
    {
    case 0x00000000: return "TEX_XYFilter_Point";
    case 0x00000001: return "TEX_XYFilter_Linear";
    case 0x00000002: return "TEX_XYFilter_AnisoPoint";
    case 0x00000003: return "TEX_XYFilter_AnisoLinear";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna4::GetZFilterString(uint32_t filter) const
{
    switch (filter)
    {
    case 0x00000000: return "TEX_ZFilter_None";
    case 0x00000001: return "TEX_ZFilter_Point";
    case 0x00000002: return "TEX_ZFilter_Linear";
    case 0x00000003: return "TEX_ZFilter_RESERVED_3";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna4::GetMipFilterString(uint32_t filter) const
{
    switch (filter)
    {
    case 0x00000000: return "TEX_MipFilter_None";
    case 0x00000001: return "TEX_MipFilter_Point";
    case 0x00000002: return "TEX_MipFilter_Linear";
    case 0x00000003: return "TEX_MipFilter_Point_Aniso_Adj";
    default: return "UNKNOWN";
    }
}

std::string SrdSamplerRdna4::GetBorderColorTypeString(uint32_t type) const
{
    switch (type)
    {
    case 0x00000000: return "SQ_TEX_BORDER_COLOR_TRANS_BLACK";
    case 0x00000001: return "SQ_TEX_BORDER_COLOR_OPAQUE_BLACK";
    case 0x00000002: return "SQ_TEX_BORDER_COLOR_OPAQUE_WHITE";
    case 0x00000003: return "SQ_TEX_BORDER_COLOR_REGISTER";
    default: return "UNKNOWN";
    }
}

SrdBvhRdna4::SrdBvhRdna4(const std::vector<uint32_t>& data) : ShaderResourceDescriptor(data)
{
}

SrdBvhRdna4::BvhFields SrdBvhRdna4::ExtractFields() const
{
    BvhFields fields;

    // Base address (bits 47:0 from dwords 0 and 1).
    fields.base_address = (static_cast<uint64_t>(GetDword(0)) | 
                          (static_cast<uint64_t>(ExtractBits(1, 0, 15)) << 32)) << 8;
    fields.sort_triangles_first  = ExtractBitsFull(52, 1);
    fields.box_sorting_heuristic = ExtractBitsFull(53, 2);
    fields.box_grow_value        = ExtractBitsFull(55, 8);
    fields.box_sort_en           = ExtractBitsFull(63, 1);
    // Size (bits 105:64) from dword 2 and 3.
    fields.size = (static_cast<uint64_t>(GetDword(2)) | 
        (static_cast<uint64_t>(ExtractBits(3, 0, 9)) << 32)) + 1;
    fields.compressed_format_en = ExtractBitsFull(115, 1);
    fields.box_node_64b         = ExtractBitsFull(116, 1);
    fields.wide_sort_en         = ExtractBitsFull(117, 1);
    fields.instance_en          = ExtractBitsFull(118, 1);
    fields.pointer_flags        = ExtractBitsFull(119, 1);
    fields.triangle_return_mode = ExtractBitsFull(120, 1);

    return fields;
}

std::string SrdBvhRdna4::ToString() const
{
    std::stringstream ss;
    ss << "BVH (RDNA4):\n";
    
    // Extract individual BVH SRD fields.
    BvhFields fields = ExtractFields();
    const std::string type = "BVH";
    
    ss << "  " << kStrBvhBaseAddress << ": 0x" << std::hex << std::setw(16) << std::setfill('0') << fields.base_address << "\n";
    ss << "  " << kStrBvhSortTrianglesFirst << ": " << (fields.sort_triangles_first ? "true" : "false") << "\n";
    ss << "  " << kStrBvhBoxSortingHeuristic << ": " << GetBoxSortingHeuristicString(fields.box_sorting_heuristic) << "\n";
    ss << "  " << kStrBvhBoxGrowValue << ": " << std::dec << fields.box_grow_value << "\n";
    ss << "  " << kStrBvhBoxSortEn << ": " << (fields.box_sort_en ? "true" : "false") << "\n";
    ss << "  " << kStrBvhSize << ": 0x" << std::hex << std::setw(16) << std::setfill('0') << fields.size << " bytes\n";
    ss << "  " << "Compressed format enable" << ": " << (fields.compressed_format_en ? "true" : "false") << "\n";
    ss << "  " << kStrBvhBoxNode64B << ": " << (fields.box_node_64b ? "true" : "false") << "\n";
    ss << "  " << kStrBvhWideSortEn << ": " << (fields.wide_sort_en ? "true" : "false") << "\n";
    ss << "  " << kStrBvhInstanceEn << ": " << (fields.instance_en ? "true" : "false") << "\n";
    ss << "  " << kStrBvhPointerFlags << ": " << (fields.pointer_flags ? "true" : "false") << "\n";
    ss << "  " << kStrBvhTriangleReturnMode << ": " << (fields.triangle_return_mode ? "true" : "false") << "\n";
    ss << "  " << kStrBvhType << ": " << type << "\n";

    return ss.str();
}

std::string SrdBvhRdna4::GetBoxSortingHeuristicString(uint32_t heuristic) const
{
    switch (heuristic)
    {
    case 0x0: return "ClosestFirst";
    case 0x1: return "LargestFirst";
    case 0x2: return "ClosestMidPoint";
    case 0x3: return "Disabled";
    default: return "UNKNOWN";
    }
}

nlohmann::json SrdImageRdna4::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Image";
    json["architecture"] = "RDNA4";

    // Extract individual Image SRD fields.
    ImageFields fields_data = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    nlohmann::json fields = {
        {"base_address", fields_data.base_address},
        {"max_mip", fields_data.max_mip},
        {"format", GetImageFormatString(fields_data.format)},
        {"base_level", fields_data.base_level},
        {"width", fields_data.width},
        {"height", fields_data.height},
        {"dstsel_x", GetDstSelString(fields_data.dstsel_x)},
        {"dstsel_y", GetDstSelString(fields_data.dstsel_y)},
        {"dstsel_z", GetDstSelString(fields_data.dstsel_z)},
        {"dstsel_w", GetDstSelString(fields_data.dstsel_w)},
        {"no_edge_clamp", fields_data.no_edge_clamp ? true : false},
        {"last_level", fields_data.last_level},
        {"sw_mode", GetSwizzleModeString(fields_data.sw_mode)},
        {"bc_swizzle", GetBcSwizzleString(fields_data.bc_swizzle)},
        {"rsrc_type", GetImageTypeString(fields_data.rsrcType)},
        {"base_array", fields_data.base_array},
        {"uav3d", fields_data.uav3d ? true : false},
        {"min_lod_warn", fields_data.min_lod_warn},
        {"perf_mod", fields_data.perf_mod},
        {"corner_sample", fields_data.corner_sample ? true : false},
        {"linked_resource", fields_data.linked_resource ? true : false},
        {"min_lod", fields_data.min_lod},
        {"iterate_256", fields_data.iterate_256 ? true : false},
        {"sample_pattern_offset", fields_data.sample_pattern_offset},
        {"max_uncompressed_block_size", fields_data.max_uncompressed_block_size},
        {"max_compressed_block_size", fields_data.max_compressed_block_size},
        {"write_compress_en", fields_data.write_compress_en ? true : false},
        {"compression_en", fields_data.compression_en ? true : false},
        {"compression_access_mode", fields_data.compression_access_mode},
        {"speculative_read", fields_data.speculative_read}
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

nlohmann::json SrdSamplerRdna4::ToJson() const
{
    nlohmann::json json;
    json["type"] = "Sampler";
    json["architecture"] = "RDNA4";

    // Extract individual Sampler SRD fields.
    SamplerFields fields = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    json["fields"] = {
        {"clamp_x", GetClampString(fields.clampX)},
        {"clamp_y", GetClampString(fields.clampY)},
        {"clamp_z", GetClampString(fields.clampZ)},
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
        {"perf_z", fields.perf_z},
        {"lod_bias", fields.lod_bias},
        {"lod_bias_sec", fields.lod_bias_sec},
        {"xy_mag_filter", GetXYFilterString(fields.xy_mag_filter)},
        {"xy_min_filter", GetXYFilterString(fields.xy_min_filter)},
        {"z_filter", GetZFilterString(fields.z_filter)},
        {"mip_filter", GetMipFilterString(fields.mip_filter)},
        {"aniso_override", fields.aniso_override ? true : false},
        {"perf_mip", fields.perf_mip},
        {"border_color_ptr", fields.border_color_ptr},
        {"border_color_type", GetBorderColorTypeString(fields.border_color_type)}
    };

    return json;
}

nlohmann::json SrdBvhRdna4::ToJson() const
{
    nlohmann::json json;
    json["type"] = "BVH";
    json["architecture"] = "RDNA4";

    // Extract individual BVH SRD fields.
    BvhFields fields = ExtractFields();

    // Populate JSON with interpreted values (same as text output)
    json["fields"] = {
        {"base_address", fields.base_address},
        {"sort_triangles_first", fields.sort_triangles_first ? true : false},
        {"box_sorting_heuristic", GetBoxSortingHeuristicString(fields.box_sorting_heuristic)},
        {"box_grow_value", fields.box_grow_value},
        {"box_sort_en", fields.box_sort_en ? true : false},
        {"size", fields.size},
        {"compressed_format_en", fields.compressed_format_en ? true : false},
        {"box_node_64b", fields.box_node_64b ? true : false},
        {"wide_sort_en", fields.wide_sort_en ? true : false},
        {"instance_en", fields.instance_en ? true : false},
        {"pointer_flags", fields.pointer_flags ? true : false},
        {"triangle_return_mode", fields.triangle_return_mode ? true : false}
    };

    return json;
}

std::unique_ptr<ShaderResourceDescriptor> SrdDisassemblerRdna4::CreateSrd(const std::vector<uint32_t>& data, SrdType type) const
{
    switch (type)
    {
    case SrdType::kBuffer:
        return std::make_unique<SrdBufferRdna4>(data);
    case SrdType::kImage:
        return std::make_unique<SrdImageRdna4>(data);
    case SrdType::kSampler:
        return std::make_unique<SrdSamplerRdna4>(data);
    case SrdType::kBvh:
        return std::make_unique<SrdBvhRdna4>(data);
    default:
        return nullptr;
    }
}

std::string SrdDisassemblerRdna4::DisassembleSrd(const std::vector<uint32_t>& data, SrdType type) const
{
    auto srd = CreateSrd(data, type);
    if (srd != nullptr)
    {
        return srd->ToString();
    }
    return "Unknown SRD type for RDNA4";
}

nlohmann::json SrdDisassemblerRdna4::DisassembleSrdJson(const std::vector<uint32_t>& data, SrdType type) const
{
    auto srd = CreateSrd(data, type);
    if (srd != nullptr)
    {
        return srd->ToJson();
    }
    
    nlohmann::json error_json;
    error_json["error"] = "Unknown SRD type for RDNA4";
    error_json["architecture"] = "RDNA4";
    return error_json;
}
