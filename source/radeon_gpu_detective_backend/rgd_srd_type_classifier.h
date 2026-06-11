//============================================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Lightweight SRD type classification function with minimal header dependencies.
///
/// This header is intentionally kept minimal (only amdisa/isa_decoder.h +
/// rgd_srd_disassembler.h) so that unit tests can include it directly without
/// pulling in the heavyweight comgr / code-object-database headers that
/// rgd_srd_instruction_analyzer.h requires.
//============================================================================================

#ifndef RGD_SRD_TYPE_CLASSIFIER_H_
#define RGD_SRD_TYPE_CLASSIFIER_H_

// ISA Decoder (isa_spec_manager) – only enum + struct declarations, no comgr dependency.
#include "amdisa/isa_decoder.h"

// SrdType enum (kBuffer / kImage / kSampler / kBvh). Pulls in nlohmann/json but not comgr.
#include "rgd_srd_disassembler.h"

// Standard.
#include <string>
#include <vector>

/// @brief Classify the SRD type for a single SGPR group from ISA functional subgroup metadata.
///
/// @details Pure classification logic extracted from SrdInstructionAnalyzer so that it can be
///          exercised by unit tests without initialising a full ISA decoder or crash dump.
///
/// @param [in] subgroups      List of functional subgroups reported by the ISA decoder.
/// @param [in] encoding_name  Encoding name of the instruction (e.g. "ENC_MIMG", "ENC_MUBUF").
/// @param [in] has_rsrc_field True if the instruction encoding contains an RSRC field.
/// @param [in] has_samp_field True if the instruction encoding contains a SAMP field.
/// @param [in] operand_index  Zero-based operand index within the instruction.
/// @param [in] group_size     Number of SGPR dwords in this group.
/// @return The inferred SrdType for this operand group.
SrdType ClassifySrdTypeFromSubgroups(const std::vector<amdisa::FunctionalSubgroups>& subgroups,
                                     const std::string&                              encoding_name,
                                     bool                                            has_rsrc_field,
                                     bool                                            has_samp_field,
                                     size_t                                          operand_index,
                                     uint32_t                                        group_size);

#endif  // RGD_SRD_TYPE_CLASSIFIER_H_
