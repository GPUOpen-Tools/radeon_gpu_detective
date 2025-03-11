//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief This is a dynamic loader module of AMDGPUDis library.
//============================================================================================

#include "rgd_amd_gpu_dis_loader.h"

AmdGpuDisEntryPoints* AmdGpuDisEntryPoints::instance_ = nullptr;
std::once_flag        AmdGpuDisEntryPoints::init_instance_flag_;