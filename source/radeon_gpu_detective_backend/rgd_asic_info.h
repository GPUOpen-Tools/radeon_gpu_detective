//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Declaration for ASIC info utilities.
//=============================================================================

#ifndef RGD_SOURCE_TRACE_INC_ASIC_INFO_H_
#define RGD_SOURCE_TRACE_INC_ASIC_INFO_H_

#include <cstdint>

namespace ecitrace
{
    /// @brief The different releases of GPUs.
    enum class GpuSeries : uint8_t
    {
        kUnknown = 0,
        kNavi1,  ///< The Navi1 series of cards.
        kNavi2,  ///< The Navi2 series of cards.
        kNavi3,  ///< The Navi3 series of cards.
        kNavi4,  ///< The Navi4 series of cards.
        kStrix1  ///< The Strix1 APUs.
    };

    /// @brief An utility function that help with determining info about the ASIC.
    struct AsicInfo
    {
        /// @brief Provides the GPU series for an ASIC.
        /// @param [in] asic_family The family of the ASIC.
        /// @param [in] asic_e_rev The eRevision of the ASIC.
        /// @return The series for the ASIC.
        static GpuSeries GetGpuSeries(uint32_t asic_family, uint32_t asic_e_rev);

    private:
        /// @brief Constructor.
        AsicInfo() = default;
    };
}  // namespace ecitrace
#endif //RGD_SOURCE_TRACE_INC_ASIC_INFO_H_