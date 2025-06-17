//============================================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools
/// @file
/// @brief  Utility functions for working with external processes implementation.
//============================================================================================

// C++.
#include <iostream>
#include <cstring>

// Local.
#include "rgd_process_utils.h"
#include "rgd_utils.h"

// Error code constants for process execution.
namespace {
    const int kProcessErrorCreateFailed = -1;
    const int kProcessErrorStdoutHandleFailed = -2;
    const int kProcessErrorJoinFailed = -3;

    // Helper function to read from a file stream and append to an output stream
    static void ReadFromFileStream(FILE* file_stream, std::stringstream& output_stream) 
    {
        if (file_stream)
        {
            constexpr size_t  kBufferSize = 8192;
            std::vector<char> buffer(kBufferSize);

            while (!feof(file_stream))
            {
                // Clear the buffer.
                buffer[0] = '\0';

                // Try to read a line.
                if (fgets(buffer.data(), buffer.size(), file_stream) != nullptr)
                {
                    // Check if we got a complete line or if it was cut off.
                    size_t len           = strlen(buffer.data());
                    bool   line_complete = (len > 0 && buffer[len - 1] == '\n');

                    // If line was cut off (no newline at the end and not at EOF).
                    if (!line_complete && !feof(file_stream))
                    {
                        std::string long_line(buffer.data());

                        // Continue reading this line in chunks until we get a newline.
                        while (!feof(file_stream))
                        {
                            if (fgets(buffer.data(), buffer.size(), file_stream) != nullptr)
                            {
                                long_line += buffer.data();
                                if (strchr(buffer.data(), '\n') != nullptr)
                                    break;
                            }
                            else
                            {
                                break;
                            }
                        }

                        output_stream << long_line;
                    }
                    else
                    {
                        // Normal case - line fits in buffer.
                        output_stream << buffer.data();
                    }
                }
            }
        }
    }
}

std::vector<const char*> RgdProcessUtils::PrepareCommandLine(
    const std::string& executable_path,
    const std::vector<std::string>& arguments)
{
    std::vector<const char*> command_line;
    
    // Add the executable path as the first argument.
    command_line.push_back(executable_path.c_str());
    
    // Add all other arguments.
    for (const auto& arg : arguments)
    {
        command_line.push_back(arg.c_str());
    }
    
    // NULL-terminate the array as required by subprocess_create.
    command_line.push_back(NULL);
    
    return command_line;
}

int RgdProcessUtils::ExecuteAndCapture(
    const std::string& executable_path,
    const std::vector<std::string>& arguments,
    std::string& output,
    std::string& error_output,
    const std::string& working_dir,
    bool inherit_env)
{
    // Default success result.
    int result = 0;
    bool process_created = false;
    struct subprocess_s process;
    std::stringstream output_stream;
    std::stringstream error_stream;
    
    // Prepare command line arguments.
    std::vector<const char*> command_line = PrepareCommandLine(executable_path, arguments);
    
    // Build a command string for logging.
    std::stringstream command_str;
    command_str << executable_path;
    for (const auto& arg : arguments)
    {
        command_str << " " << arg;
    }
    
    // Log the command.
    std::stringstream console_msg;
    console_msg << "Executing command: " << command_str.str();
    RgdUtils::PrintMessage(console_msg.str().c_str(), RgdMessageType::kInfo, true);
    
    // Set up subprocess options.
    int options = 0;
    if (inherit_env)
    {
        options |= subprocess_option_inherit_environment;
    }
    
    // Create subprocess.
    int create_result = subprocess_create(
        command_line.data(),
        options,
        &process);
    
    if (create_result == 0)
    {
        process_created = true;
        
        // Get stdout and stderr handles
        FILE* p_stdout = subprocess_stdout(&process);
        FILE* p_stderr = subprocess_stderr(&process);
        
        // Read output from stdout
        ReadFromFileStream(p_stdout, output_stream);
        
        // Read output from stderr 
        ReadFromFileStream(p_stderr, error_stream);
        
        // Wait for process to complete and get exit code.
        int process_return;
        int join_result = subprocess_join(&process, &process_return);
        
        if (join_result == 0)
        {
            result = process_return;
        }
        else
        {
            std::cerr << "Error: Failed to join process." << std::endl;
            result = kProcessErrorJoinFailed;
        }
    }
    else
    {
        std::cerr << "Error: Failed to create process for: " << executable_path << std::endl;
        result = kProcessErrorCreateFailed;
    }
    
    // Clean up.
    if (process_created)
    {
        subprocess_destroy(&process);
    }
    
    // Store output.
    output = output_stream.str();
    error_output = error_stream.str();  // Store stderr output
    
    return result;
}
