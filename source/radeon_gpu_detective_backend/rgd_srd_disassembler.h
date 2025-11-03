//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief The interface class for SRD (Shader Resource Descriptor) disassembly.
//============================================================================================

#ifndef RGD_SRD_DISASSEMBLER_H_
#define RGD_SRD_DISASSEMBLER_H_

// Standard.
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// JSON.
#include "json/single_include/nlohmann/json.hpp"

/// @brief SRD type enumeration.
enum class SrdType : uint32_t
{
    kBuffer,
    kImage,
    kSampler,
    kBvh
};

/// @brief Base class for SRD data representation.
class ShaderResourceDescriptor
{
public:
    /// @brief Constructor.
    /// @param [in] data The SRD raw data as vector of 32-bit words.
    explicit ShaderResourceDescriptor(const std::vector<uint32_t>& data);

    /// @brief Destructor.
    virtual ~ShaderResourceDescriptor() = default;

    /// @brief Print the SRD fields as text.
    /// @return The string representation of the SRD.
    virtual std::string ToString() const = 0;

    /// @brief Serialize the SRD fields to JSON.
    /// @return The JSON representation of the SRD.
    virtual nlohmann::json ToJson() const = 0;

    /// @brief Get the SRD type.
    /// @return The SRD type.
    virtual SrdType GetType() const = 0;

protected:
    /// @brief Extract a bit field from the raw data.
    /// @param [in] dword_index The index of the 32-bit word.
    /// @param [in] start_bit The starting bit position.
    /// @param [in] end_bit The ending bit position.
    /// @return The extracted value.
    uint32_t ExtractBits(uint32_t dword_index, uint32_t start_bit, uint32_t end_bit) const;

    /// @brief Extract a bit field from the raw data.
    /// @param [in] start_bit The starting bit position, relative to full data.
    /// @param [in] n_bits The number of bits to extract
    /// @return The extracted value.
    uint32_t ExtractBitsFull(uint32_t start_bit, uint32_t n_bits) const;

    /// @brief Get the raw DWORD at the specified index.
    /// @param [in] index The index of the DWORD value.
    /// @return DWORD value.
    uint32_t GetDword(uint32_t index) const;

    /// @brief Get the raw data as vector of 32-bit values.
    /// @return The raw data vector.
    const std::vector<uint32_t>& GetData() const;

private:
    std::vector<uint32_t> data_;  ///< Raw SRD data as 32-bit values.
};

/// @brief Interface for hardware-specific SRD disassemblers.
class ISrdDisassembler
{
public:
    /// @brief Destructor.
    virtual ~ISrdDisassembler() = default;

    /// @brief Create an SRD object from raw data.
    /// @param [in] data The SRD raw data as vector of 32-bit words.
    /// @param [in] type The SRD type.
    /// @return Unique pointer to the created SRD object.
    virtual std::unique_ptr<ShaderResourceDescriptor> CreateSrd(const std::vector<uint32_t>& data, SrdType type) const = 0;

    /// @brief Disassemble SRD data to human-readable text.
    /// @param [in] data The SRD raw data as vector of 32-bit words.
    /// @param [in] type The SRD type.
    /// @return The disassembled SRD as text.
    virtual std::string DisassembleSrd(const std::vector<uint32_t>& data, SrdType type) const = 0;

    /// @brief Disassemble SRD data to JSON format.
    /// @param [in] data The SRD raw data as vector of 32-bit words.
    /// @param [in] type The SRD type.
    /// @return The disassembled SRD as JSON.
    virtual nlohmann::json DisassembleSrdJson(const std::vector<uint32_t>& data, SrdType type) const = 0;
};

// From public ISA spec documents.
static const char* kStrBufferBaseAddr              = "Base address";
static const char* kStrBufferStride                = "Stride";
static const char* kStrBufferSwizzleEnable         = "Swizzle enable";
static const char* kStrBufferNumRecords            = "Num_records";
static const char* kStrBufferDstSelX               = "Dst_sel_x";
static const char* kStrBufferDstSelY               = "Dst_sel_y";
static const char* kStrBufferDstSelZ               = "Dst_sel_z";
static const char* kStrBufferDstSelW               = "Dst_sel_w";
static const char* kStrBufferFormat                = "Format";
static const char* kStrBufferStrideScale           = "Stride scale";
static const char* kStrBufferIndexStride           = "Index stride";
static const char* kStrBufferAddTidEnable          = "Add tid enable";
static const char* kStrBufferWriteCompressEn       = "Write compression enable";
static const char* kStrBufferCompressionEn         = "Compression enable";
static const char* kStrBufferCompressionAccessMode = "Compression access mode";
static const char* kStrBufferOobSelect             = "OOB_SELECT";
static const char* kStrBufferType                  = "Type";

static const char* kStrImageBaseAddr          = "Base address";
static const char* kStrImageBigPage           = "Big page";
static const char* kStrImageMaxMip            = "Max mip";
static const char* kStrImageFormat            = "Format";
static const char* kStrImageBaseLevel         = "Base level";
static const char* kStrImageWidth             = "Width";
static const char* kStrImageHeight            = "Height";
static const char* kStrImageDstSelX           = "Dst_sel_x";
static const char* kStrImageDstSelY           = "Dst_sel_y";
static const char* kStrImageDstSelZ           = "Dst_sel_z";
static const char* kStrImageDstSelW           = "Dst_sel_w";
static const char* kStrImageLastLevel         = "Last level";
static const char* kStrImageBcSwizzle         = "BC swizzle";
static const char* kStrImageType              = "Type";
static const char* kStrImageDepth             = "Depth";
static const char* kStrImagePitchMsb          = "Pitch_msb";
static const char* kStrImageBaseArray         = "Base array";
static const char* kStrImageArrayPitch        = "Array pitch";
static const char* kStrImageUav3d             = "UAV3D";
static const char* kStrImageMinLodWarn4       = "Min_lod_warn";
static const char* kStrImageMinLodWarn3       = "Min lod warn";
static const char* kStrImageCornerSamples     = "Corner samples mod";
static const char* kStrImageMinLod            = "Min LOD";
static const char* kStrImageIterate256        = "Iterate 256";
static const char* kStrImageMetaPipeAligned   = "Meta pipe aligned";
static const char* kStrImageCompressionEn     = "Compression enable";
static const char* kStrImageAlphaIsOnMsb      = "Alpha is on MSB";
static const char* kStrImageColorTransform    = "Color transform";
static const char* kStrImageMetaDataAddress   = "Meta data address";

static const char* kStrSamplerClampX            = "Clamp x";
static const char* kStrSamplerClampY            = "Clamp y";
static const char* kStrSamplerClampZ            = "Clamp z";
static const char* kStrSamplerMaxAnisoRatio     = "Max aniso ratio";
static const char* kStrSamplerDepthCompareFunc  = "Depth compare func";
static const char* kStrSamplerForceUnnormalized = "Force unnormalized";
static const char* kStrSamplerAnisoThreshold    = "Aniso threshold";
static const char* kStrSamplerMcCoordTrunc      = "Mc coord trunc";
static const char* kStrSamplerForceDegamma      = "Force degamma";
static const char* kStrSamplerAnisoBias         = "Aniso bias";
static const char* kStrSamplerTruncCoord        = "Trunc coord";
static const char* kStrSamplerDisableCubeWrap   = "Disable cube wrap";
static const char* kStrSamplerFilterMode        = "Filter_mode";
static const char* kStrSamplerSkipDegamma       = "Skip degamma";
static const char* kStrSamplerMinLod            = "Min lod";
static const char* kStrSamplerMaxLod            = "Max lod";
static const char* kStrSamplerLodBias           = "Lod bias";
static const char* kStrSamplerLodBiasSec        = "Lod bias sec";
static const char* kStrSamplerXyMagFilter       = "Xy mag filter";
static const char* kStrSamplerXyMinFilter       = "Xy min filter";
static const char* kStrSamplerZFilter           = "Z filter";
static const char* kStrSamplerMipFilter         = "Mip filter";
static const char* kStrSamplerBorderColorPtr    = "Border color ptr";
static const char* kStrSamplerBorderColorType   = "Border color type";

static const char* kStrBvhBaseAddress         = "Base_address";
static const char* kStrBvhSortTrianglesFirst  = "Sort_triangles_first";
static const char* kStrBvhBoxSortingHeuristic = "Box_sorting_heuristic";
static const char* kStrBvhBoxGrowValue        = "Box_grow_value";
static const char* kStrBvhBoxSortEn           = "Box_sort_en";
static const char* kStrBvhSize                = "Size";
static const char* kStrBvhBoxNode64B          = "Box_node_64B";
static const char* kStrBvhWideSortEn          = "Wide_sort_en";
static const char* kStrBvhInstanceEn          = "Instance_en";
static const char* kStrBvhPointerFlags        = "Pointer_flags";
static const char* kStrBvhTriangleReturnMode  = "Triangle_return_mode";
static const char* kStrBvhType                = "Type";

#endif  // RGD_SRD_DISASSEMBLER_H_
