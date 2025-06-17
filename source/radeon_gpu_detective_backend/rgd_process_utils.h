//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief  Utility functions for working with external processes.
//============================================================================================

#ifndef RGD_PROCESS_UTILS_H_
#define RGD_PROCESS_UTILS_H_

// C++.
#include <string>
#include <vector>
#include <sstream>

// Include the subprocess.h single header library.
#include "subprocess/subprocess.h"

class RgdProcessUtils 
{
public:
    // Execute a command and capture its output.
    // Parameters:
    //   executable_path: Path to the executable
    //   arguments: Vector of command-line arguments
    //   output: String to store the command output
    //   error_output: String to store the command error output
    //   working_dir: Working directory for the process (empty for current directory)
    //   inherit_env: Whether to inherit environment variables from the parent
    // Returns:
    //   Process exit code if successful, negative value if process creation failed
    static int ExecuteAndCapture(const std::string&              executable_path,
                                 const std::vector<std::string>& arguments,
                                 std::string&                    output,
                                 std::string&                    error_output,
                                 const std::string&              working_dir = "",
                                 bool                            inherit_env = true);

    // Convert a vector of string arguments to the format required by subprocess.h
    static std::vector<const char*> PrepareCommandLine(
        const std::string& executable_path,
        const std::vector<std::string>& arguments);

private:
    RgdProcessUtils() = delete;
    ~RgdProcessUtils() = delete;
};

#endif // RGD_PROCESS_UTILS_H_
