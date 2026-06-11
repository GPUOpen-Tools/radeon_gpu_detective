//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Unit tests for SRD type classification logic in SrdInstructionAnalyzer.
///
/// These tests exercise ClassifySrdTypeFromSubgroups() directly with synthetic
/// InstructionInfo metadata so that every ISA functional-subgroup category
/// (BVH, Buffer/Load/Store, Texture/Sample, Atomic) is covered.  Running these
/// tests after an isa_spec_manager update quickly confirms that the subgroup
/// names have not been renamed or renumbered in a way that would silently break
/// SRD type classification.
//=============================================================================

// Catch2.
#include <catch.hpp>

// Backend: minimal classifier header - avoids pulling in comgr / code-object-database deps.
#include "rgd_srd_type_classifier.h"

// Standard.
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a subgroup list containing exactly one entry.
static std::vector<amdisa::FunctionalSubgroups> SingleSubgroup(amdisa::FunctionalSubgroups sg)
{
    return {sg};
}

/// Build a subgroup list containing two entries (e.g. Texture + Sample).
static std::vector<amdisa::FunctionalSubgroups> TwoSubgroups(amdisa::FunctionalSubgroups sg1,
                                                              amdisa::FunctionalSubgroups sg2)
{
    return {sg1, sg2};
}

// ---------------------------------------------------------------------------
// BVH subgroup
// ---------------------------------------------------------------------------

TEST_CASE("SRD Classification - BVH subgroup", "[SrdClassification][Bvh]")
{
    SECTION("BVH subgroup maps to kBvh regardless of other parameters")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupBvh);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_BVH", false, false, 0, 4)
              == SrdType::kBvh);
    }

    SECTION("BVH subgroup takes priority even when combined with Texture")
    {
        const auto subgroups = TwoSubgroups(amdisa::FunctionalSubgroups::kFunctionalSubgroupBvh,
                                            amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_BVH", true, false, 0, 8)
              == SrdType::kBvh);
    }
}

// ---------------------------------------------------------------------------
// Buffer / Load / Store subgroups
// ---------------------------------------------------------------------------

TEST_CASE("SRD Classification - Buffer subgroup", "[SrdClassification][Buffer]")
{
    SECTION("kFunctionalSubgroupBuffer maps to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupBuffer);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MTBUF", true, false, 0, 4)
              == SrdType::kBuffer);
    }

    SECTION("kFunctionalSubgroupLoad maps to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupLoad);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MUBUF", true, false, 0, 4)
              == SrdType::kBuffer);
    }

    SECTION("kFunctionalSubgroupStore maps to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupStore);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MUBUF", true, false, 0, 4)
              == SrdType::kBuffer);
    }
}

// ---------------------------------------------------------------------------
// Texture / Sample subgroups
// ---------------------------------------------------------------------------

TEST_CASE("SRD Classification - Texture subgroup", "[SrdClassification][Texture]")
{
    SECTION("Texture + RSRC only (8 dwords) maps to kImage")
    {
        // A texture fetch with only an RSRC field (image without sampler) and 8 SGPR dwords
        // should classify as an image descriptor.
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, false, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Texture + RSRC only (4 dwords) maps to kBuffer (small texture load)")
    {
        // A 4-dword RSRC-only texture operand falls back to buffer classification.
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, false, 0, 4)
              == SrdType::kBuffer);
    }

    SECTION("Texture + RSRC+SAMP (8 dwords, operand 0) maps to kImage")
    {
        // Combined texture+sampler instruction: the first operand (8 dwords) is the image RSRC.
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, true, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Texture + RSRC+SAMP (4 dwords, operand 1) maps to kSampler")
    {
        // Combined texture+sampler instruction: the second operand (4 dwords) is the sampler SAMP.
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, true, 1, 4)
              == SrdType::kSampler);
    }
}

TEST_CASE("SRD Classification - Sample subgroup", "[SrdClassification][Sample]")
{
    SECTION("Sample + RSRC only (8 dwords) maps to kImage")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupSample);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, false, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Sample + RSRC+SAMP (4 dwords, operand 1) maps to kSampler")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupSample);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, true, 1, 4)
              == SrdType::kSampler);
    }

    SECTION("Texture+Sample combined subgroups with RSRC+SAMP (8 dwords) maps to kImage")
    {
        // Real instructions may carry both Texture and Sample subgroups simultaneously.
        const auto subgroups = TwoSubgroups(amdisa::FunctionalSubgroups::kFunctionalSubgroupTexture,
                                            amdisa::FunctionalSubgroups::kFunctionalSubgroupSample);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", true, true, 0, 8)
              == SrdType::kImage);
    }
}

// ---------------------------------------------------------------------------
// Atomic subgroup
// ---------------------------------------------------------------------------

TEST_CASE("SRD Classification - Atomic subgroup", "[SrdClassification][Atomic]")
{
    SECTION("Atomic with ENC_MIMG encoding maps to kImage")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupAtomic);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MIMG", false, false, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Atomic with MIMG_NSA1 encoding maps to kImage")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupAtomic);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "MIMG_NSA1", false, false, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Atomic with ENC_VIMAGE encoding maps to kImage")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupAtomic);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_VIMAGE", false, false, 0, 8)
              == SrdType::kImage);
    }

    SECTION("Atomic with buffer encoding (ENC_MUBUF) maps to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupAtomic);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MUBUF", false, false, 0, 4)
              == SrdType::kBuffer);
    }

    SECTION("Atomic with unknown encoding defaults to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupAtomic);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "ENC_MTBUF", false, false, 0, 4)
              == SrdType::kBuffer);
    }
}

// ---------------------------------------------------------------------------
// Default / unknown subgroup fallback
// ---------------------------------------------------------------------------

TEST_CASE("SRD Classification - Default fallback", "[SrdClassification]")
{
    SECTION("Empty subgroup list defaults to kBuffer")
    {
        const std::vector<amdisa::FunctionalSubgroups> empty_subgroups;
        CHECK(ClassifySrdTypeFromSubgroups(empty_subgroups, "", false, false, 0, 4)
              == SrdType::kBuffer);
    }

    SECTION("Unknown subgroup defaults to kBuffer")
    {
        const auto subgroups = SingleSubgroup(amdisa::FunctionalSubgroups::kFunctionalSubgroupUnknown);
        CHECK(ClassifySrdTypeFromSubgroups(subgroups, "", false, false, 0, 4)
              == SrdType::kBuffer);
    }
}
