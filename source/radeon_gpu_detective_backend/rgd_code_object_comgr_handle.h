//=============================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Code object manager handles.
//=============================================================================

#ifndef RGD_BACKEND_RGD_CODE_OBJECT_COMGR_HANDLE_H_
#define RGD_BACKEND_RGD_CODE_OBJECT_COMGR_HANDLE_H_

#include <source/comgr_utils.h>
#include "amd_comgr.h"

/// RGD style Handle to access comgr.
typedef std::unique_ptr<amdt::CodeObj> RgdComgrHandle;

#endif  // RGD_BACKEND_RGD_CODE_OBJECT_COMGR_HANDLE_H_