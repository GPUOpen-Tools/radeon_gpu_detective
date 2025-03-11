//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Test .rgd file structure
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_TEST_RGD_FILE_H_
#define RADEON_GPU_DETECTIVE_TEST_RGD_FILE_H_

#include <string>
#include "rgd_data_types.h"

class TestRgdFile
{
public:
    explicit TestRgdFile(const std::string& file_path);

    bool ParseRgdFile(bool is_page_fault);
    bool IsSystemInfoParsed();
    bool IsRdfParsingError();
    bool IsAppMarkersFound();
    bool IsMarkerContextFound();

private:
    std::string file_path_;
    bool is_system_info_parsed_ = false;
    bool is_rdf_parsing_error_ = false;
    bool is_umd_chunk_found_ = false;
    bool is_kmd_chunk_found_ = false;
    RgdCrashDumpContents rgd_file_contents_;
};

#endif // RADEON_GPU_DETECTIVE_TEST_RGD_FILE_H_