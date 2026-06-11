//============================================================================================
// Copyright (c) 2019-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief This is a dynamic loader module of AMDGPUDis library.
//============================================================================================

#ifndef RGD_BACKEND_RGD_AMD_GPU_DIS_LOADER_H_
#define RGD_BACKEND_RGD_AMD_GPU_DIS_LOADER_H_

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <tchar.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <memory>
#include <mutex>
#include <string>

#include "CodeObjectDisassemblerApi.h"

/// @brief Singleton class to manage the disassembler entry points.
class AmdGpuDisEntryPoints
{
public:
    decltype(AmdGpuDisGetApiTable)* AmdGpuDisGetApiTable_fn;  ///< Type for function pointer to get entry point table.

    /// @brief Get singleton instance.
    ///
    /// @return The singleton instance of AmdGpuDisEntryPoints.
    static AmdGpuDisEntryPoints* Instance()
    {
        std::call_once(init_instance_flag_, InitInstance);

        return instance_;
    }

    /// @brief Delete singleton instance.
    static void DeleteInstance()
    {
        if (nullptr != instance_)
        {
            AmdGpuDisEntryPoints* copy_of_instance = instance_;
            instance_                              = nullptr;
            delete copy_of_instance;
        }
    }

    /// @brief Check whether the entry points are valid.
    ///
    /// @return true if all entry points were initialized.
    bool EntryPointsValid()
    {
        return entry_points_valid_;
    }

private:
    bool entry_points_valid_ = true;  ///< Flag indicating if the AMDGPUDis library entry points are valid.

#ifdef _WIN32
    HMODULE module_ = nullptr;  ///< The AMDGPUDis library module handle.
#else
    void* module_ = nullptr;  ///< The AMDGPUDis library module handle.
#endif

    /// @brief Attempts to initialize the specified AMDGPUDis library entry point.
    ///
    /// @param [in] entry_point_name The name of the entry point to initialize.
    ///
    /// @return The address of the entry point or nullptr if the entry point could not be initialized.
    void* InitEntryPoint(const char* entry_point_name)
    {
        if (nullptr != module_)
        {
#ifdef _WIN32
            return GetProcAddress(module_, entry_point_name);
#else
            return dlsym(module_, entry_point_name);
#endif
        }

        return nullptr;
    }

    /// @brief Private constructor.
    AmdGpuDisEntryPoints()
    {
#ifdef _WIN32
        // Get the module handle for the DLL containing this code (not the host executable).
        HMODULE this_module = nullptr;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCTSTR>(&AmdGpuDisEntryPoints::Instance),
                              &this_module))
        {
            TCHAR module_path[MAX_PATH] = {0};
            DWORD path_length = GetModuleFileName(this_module, module_path, MAX_PATH);
            if (path_length > 0 && path_length < MAX_PATH)
            {
                // Remove the filename to get the directory.
                TCHAR* last_separator = _tcsrchr(module_path, _T('\\'));
                if (last_separator != nullptr)
                {
                    *(last_separator + 1) = _T('\0');
                }
                // Append the DLL name.
                _tcscat_s(module_path, MAX_PATH, _T("amdgpu_dis.dll"));
                module_ = LoadLibrary(module_path);
            }
        }

        // Fallback to bare filename if path resolution failed.
        if (nullptr == module_)
        {
            module_ = LoadLibrary(_T("amdgpu_dis.dll"));
        }
#else
        // Linux: Get the directory of the shared library containing this code.
        Dl_info dl_info;
        if (dladdr(reinterpret_cast<void*>(&AmdGpuDisEntryPoints::Instance), &dl_info) != 0 && dl_info.dli_fname != nullptr)
        {
            std::string lib_path = dl_info.dli_fname;
            size_t last_separator = lib_path.find_last_of('/');
            if (last_separator != std::string::npos)
            {
                lib_path = lib_path.substr(0, last_separator + 1) + "libamdgpu_dis.so";
                module_ = dlopen(lib_path.c_str(), RTLD_LAZY | RTLD_DEEPBIND);
            }
        }

        // Fallback to bare filename if path resolution failed.
        if (nullptr == module_)
        {
            module_ = dlopen("libamdgpu_dis.so", RTLD_LAZY | RTLD_DEEPBIND);
        }
#endif
#define INIT_AMDGPUDIS_ENTRY_POINT(func)                      \
    reinterpret_cast<decltype(func)*>(InitEntryPoint(#func)); \
    entry_points_valid_ &= nullptr != func##_fn

        AmdGpuDisGetApiTable_fn = INIT_AMDGPUDIS_ENTRY_POINT(AmdGpuDisGetApiTable);
#undef INIT_AMDGPUDIS_ENTRY_POINT
    }

    /// @brief Destructor.
    virtual ~AmdGpuDisEntryPoints()
    {
        if (nullptr != module_)
        {
#ifdef _WIN32
            FreeLibrary(module_);
#else
            dlclose(module_);
#endif
        }
        DeleteInstance();
    }

    static AmdGpuDisEntryPoints* instance_;            ///< Static singleton instance.
    static std::once_flag        init_instance_flag_;  ///< Static instance init flag for multithreading.

    /// @brief Create the singleton instance.
    static void InitInstance()
    {
        instance_ = new (std::nothrow) AmdGpuDisEntryPoints;
    }
};

#endif  // RGD_BACKEND_RGD_AMD_GPU_DIS_LOADER_H_