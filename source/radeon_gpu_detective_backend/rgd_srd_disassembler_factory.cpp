//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief SRD (Shader Resource Descriptor) disassembler factory implementation.
//============================================================================================

#include "rgd_srd_disassembler_factory.h"

// Local.
#include "rgd_srd_disassembler_rdna3.h"
#include "rgd_srd_disassembler_rdna4.h"

std::unique_ptr<ISrdDisassembler> SrdDisassemblerFactory::CreateDisassembler(ecitrace::GpuSeries gpu_series)
{
    switch (gpu_series)
    {
    case ecitrace::GpuSeries::kNavi3:
        return std::make_unique<SrdDisassemblerRdna3>();
    case ecitrace::GpuSeries::kNavi4:
        return std::make_unique<SrdDisassemblerRdna4>();
    case ecitrace::GpuSeries::kStrix1:
        // STRIX1 is RDNA3-based architecture.
        return std::make_unique<SrdDisassemblerRdna3>();
    default:
        // For unknown GPU series, return nullptr to indicate unsupported.
        return nullptr;
    }
}
