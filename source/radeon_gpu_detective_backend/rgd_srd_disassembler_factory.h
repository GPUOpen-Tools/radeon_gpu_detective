//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief Factory class for creating SRD disassemblers based on GPU architecture.
//============================================================================================

#ifndef RGD_SRD_DISASSEMBLER_FACTORY_H_
#define RGD_SRD_DISASSEMBLER_FACTORY_H_

// Local.
#include "rgd_srd_disassembler.h"
#include "rgd_asic_info.h"

// C++.
#include <memory>

/// @brief Factory class for creating SRD disassemblers.
class SrdDisassemblerFactory
{
public:
    /// @brief Create an SRD disassembler for the specified GPU series.
    /// @param [in] gpu_series The GPU series.
    /// @return Unique pointer to the SRD disassembler, or nullptr if unsupported.
    static std::unique_ptr<ISrdDisassembler> CreateDisassembler(ecitrace::GpuSeries gpu_series);

private:
    SrdDisassemblerFactory() = delete;
    ~SrdDisassemblerFactory() = delete;
};

#endif  // RGD_SRD_DISASSEMBLER_FACTORY_H_
