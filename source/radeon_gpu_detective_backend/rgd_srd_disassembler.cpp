//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief Base implementation for SRD (Shader Resource Descriptor) disassembly.
//============================================================================================

#include "rgd_srd_disassembler.h"

// Standard.
#include <cassert>

// Local.
#include "rgd_utils.h"

ShaderResourceDescriptor::ShaderResourceDescriptor(const std::vector<uint32_t>& data) : data_(data)
{
}

uint32_t ShaderResourceDescriptor::ExtractBits(uint32_t dword_index, uint32_t start_bit, uint32_t end_bit) const
{
    uint32_t result = 0;
    
    assert(dword_index < data_.size());
    assert(start_bit <= end_bit);
    assert(end_bit < 32);

    // Check if dword_index is within bounds and all inputs are valid.
    if (dword_index < data_.size() && start_bit <= end_bit && end_bit < 32)
    {
        const uint32_t word = data_[dword_index];
        const uint32_t mask = ((1u << (end_bit - start_bit + 1)) - 1);
        result = (word >> start_bit) & mask;
    }
    else
    {
        // if (dword_index >= data_.size()){} Not an error; may have bitfields beyond provided data, in which case return 0.

        if (start_bit > end_bit || end_bit >= 32)
        {
            RgdUtils::PrintMessage("SRD ExtractBits: invalid bit indices", RgdMessageType::kError, true);
        }
    }

    return result;
}

uint32_t ShaderResourceDescriptor::ExtractBitsFull(uint32_t start_bit, uint32_t n_bits) const
{
    uint32_t result = 0;
    uint32_t last_bit = start_bit + n_bits - 1;
    assert(n_bits <= 32);

    uint32_t start_dword = start_bit / 32;
    uint32_t last_dword   = last_bit / 32;

    // If retrieving bits beyond the available data, return 0.
    if(last_dword >= data_.size())
    {
        return 0;
    }

    // Check if dword_index is within bounds and all inputs are valid.
    if (last_dword < data_.size() && n_bits <= 32 )
    {
        start_bit = start_bit % 32;
        last_bit   = last_bit % 32;
        if (last_dword == start_dword)
        {
            const uint32_t word = data_[start_dword];
            const uint64_t mask = ((1ull << (n_bits)) - 1);
            result = (word >> start_bit) & mask;
        }
        else
        {
            const uint32_t word0 = data_[start_dword];
            const uint32_t word0_n_bits = 32 - start_bit;
            const uint32_t mask0 = ((1u << (word0_n_bits)) - 1);

            const uint32_t word1 = data_[last_dword];
            const uint32_t word1_n_bits = last_bit + 1;
            const uint32_t mask1 = ((1u << (word1_n_bits)) - 1);

            result = ((word0 >> start_bit) & mask0) | (((word1 & mask1)) << word0_n_bits);
        }
    }
    else
    {
        if (last_dword >= data_.size() * 32 || n_bits > 32)
        {
            RgdUtils::PrintMessage("SRD ExtractBits: invalid bit index range", RgdMessageType::kError, true);
        }
    }

    return result;
}

uint32_t ShaderResourceDescriptor::GetDword(uint32_t index) const
{
    uint32_t result = 0;

    // Check if index is within bounds.
    assert(index < data_.size());
    if (index < data_.size()) {
        result = data_[index];
    }
    else
    {
        // Report error using the project's standard error reporting.
        RgdUtils::PrintMessage("SRD GetDword: index out of bounds", RgdMessageType::kError, true);
    }

    return result;
}

const std::vector<uint32_t>& ShaderResourceDescriptor::GetData() const
{
    return data_;
}
