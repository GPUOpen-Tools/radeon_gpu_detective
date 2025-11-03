//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Performance measurement utilities for timing analysis.
//=============================================================================
#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_PERF_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_PERF_H_

// Performance measurement output prefix constant.
#define RGD_PERF_PREFIX "[PERF]"

#ifdef RGD_ENABLE_PERF

// C++ standard library includes.
#include <chrono>
#include <iostream>
#include <string>

/// @brief RAII timer class for automatic performance measurement of function execution time.
class RgdPerfTimer
{
public:
    /// @brief Constructor starts the timer.
    /// @param function_name The name of the function being measured.
    explicit RgdPerfTimer(const std::string& function_name)
        : function_name_(function_name)
        , start_time_(std::chrono::high_resolution_clock::now())
    {
        std::cout << RGD_PERF_PREFIX << " Starting " << function_name_ << "..." << std::endl;
    }

    /// @brief Destructor automatically logs the elapsed time.
    ~RgdPerfTimer()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        std::cout << RGD_PERF_PREFIX << " " << function_name_ << " completed in " << duration.count() << " ms" << std::endl;
    }

    /// @brief Get elapsed time without destroying the timer.
    /// @return Elapsed time in milliseconds.
    int64_t GetElapsedMs() const
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        return duration.count();
    }

    /// @brief Log an intermediate timing checkpoint.
    /// @param checkpoint_name Name of the checkpoint.
    void LogCheckpoint(const std::string& checkpoint_name) const
    {
        std::cout << RGD_PERF_PREFIX << " " << function_name_ << " - " << checkpoint_name << ": " 
                  << GetElapsedMs() << " ms elapsed" << std::endl;
    }

private:
    std::string function_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

/// @brief Macro for easy function performance measurement - creates a timer for the entire function scope.
#define RGD_PERF_FUNCTION(name) RgdPerfTimer perf_timer(name)

/// @brief Macro for measuring performance of a specific scope with a custom name.
/// Uses __LINE__ to make variable names unique to avoid conflicts.
#define RGD_PERF_SCOPE(name) RgdPerfTimer RGD_CONCAT(perf_scope_timer_, __LINE__)(name)

/// @brief Helper macro for token concatenation.
#define RGD_CONCAT_IMPL(x, y) x##y
#define RGD_CONCAT(x, y) RGD_CONCAT_IMPL(x, y)

/// @brief Macro for logging checkpoints during function execution.
#define RGD_PERF_CHECKPOINT(timer, name) timer.LogCheckpoint(name)

/// @brief Macro for performance logging.
#define RGD_PERF_LOG(message) std::cout << RGD_PERF_PREFIX << " " << message << std::endl

#else

// Performance measurement disabled - all macros become no-ops.
#define RGD_PERF_FUNCTION(name) ((void)0)
#define RGD_PERF_SCOPE(name) ((void)0)  
#define RGD_PERF_CHECKPOINT(timer, name) ((void)0)
#define RGD_PERF_LOG(message) ((void)0)

#endif // RGD_ENABLE_PERF

#endif // RADEON_GPU_DETECTIVE_SOURCE_RGD_PERF_H_
