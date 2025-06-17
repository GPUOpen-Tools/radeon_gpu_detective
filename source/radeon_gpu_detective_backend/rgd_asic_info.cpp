//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation for ASIC info utilities.
//=============================================================================

#include "rgd_asic_info.h"

namespace ecitrace
{
    // See amdgpu_asic.h and Device::DetermineGpuIpLevels() in PAL
    static constexpr uint32_t kFamilyNavi            = 0x8F;
    static constexpr uint32_t kNavi2XMinimumRevision = 0x28;

    static constexpr uint32_t kFamilyNavi3     = 0x91;

    static constexpr uint32_t kFamilyNavi4     = 0x98;
    static constexpr uint32_t kFamilyStrix1    = 0x96;

    GpuSeries ecitrace::AsicInfo::GetGpuSeries(const uint32_t asic_family, const uint32_t asic_e_rev)
    {
        if (asic_family == kFamilyNavi)
        {
            return asic_e_rev < kNavi2XMinimumRevision ? GpuSeries::kNavi1 : GpuSeries::kNavi2;
        }

        // This is derived from Gfx9::DetermineIpLevel() in PAL
        switch (asic_family)
        {
        case kFamilyNavi3:
            return GpuSeries::kNavi3;
        case kFamilyNavi4:
            return GpuSeries::kNavi4;
        case kFamilyStrix1:
            return GpuSeries::kStrix1;

        default:
            return GpuSeries::kUnknown;
        }
    }
};  // namespace ecitrace