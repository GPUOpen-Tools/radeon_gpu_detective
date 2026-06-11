#=============================================================================
# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  RGD Test CMakeLists file.
#=============================================================================

cmake_minimum_required(VERSION 3.10)
project(rgd_test)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include cxxopts for command line parsing.
include_directories("${PROJECT_SOURCE_DIR}/../../../external/third_party/catch2/single_include/catch2/")
include_directories("${PROJECT_SOURCE_DIR}/../../../external/third_party/cxxopts/include/")
include_directories("${PROJECT_SOURCE_DIR}/../../../source/radeon_gpu_detective_backend/")
include_directories("${PROJECT_SOURCE_DIR}/../../../external/isa_spec_manager/include/")

set(CMAKE_CXX_STANDARD 17)

# Source files that are common for Windows and Linux
file(GLOB RGD_TEST_SRC
    "*.cpp"
    "*.h"
    "../../../external/third_party/catch2/single_include/catch2/*.hpp"
)

add_executable(${PROJECT_NAME} ${RGD_TEST_SRC})

# Linker.
target_link_libraries(${PROJECT_NAME} rgd_backend)

# Warning level.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

#=============================================================================
# CTest integration
#=============================================================================
enable_testing()

# Copy sample files next to test executable for testing.
if(EXISTS "${PROJECT_SOURCE_DIR}/../../../samples")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/../../../samples"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/samples"
    )
endif()

# Add backend validation test with sample crash dump.
add_test(NAME rgd_backend_tests
         COMMAND ${PROJECT_NAME} --rgd "${PROJECT_SOURCE_DIR}/../../../samples/sample_crash_dump.rgd"
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Add page fault test case.
add_test(NAME rgd_backend_page_fault_tests
         COMMAND ${PROJECT_NAME} --rgd "${PROJECT_SOURCE_DIR}/../../../samples/sample_crash_dump.rgd" --page-fault
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Add SRD classification unit tests.
# These tests exercise the subgroup-to-SrdType mapping directly using synthetic metadata
# and do not read the crash dump content. The --rgd argument is required by the test
# runner's argument parser (InitializeTestOptions) to allow the session to start.
add_test(NAME rgd_srd_classification_tests
         COMMAND ${PROJECT_NAME} "[SrdClassification]" --rgd "${PROJECT_SOURCE_DIR}/../../../samples/sample_crash_dump.rgd"
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
