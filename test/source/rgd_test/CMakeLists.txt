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
include_directories("${PROJECT_SOURCE_DIR}/../../../external/system_info_utils/source/")
include_directories("${PROJECT_SOURCE_DIR}/../../../external/third_party/cxxopts/include/")
include_directories("${PROJECT_SOURCE_DIR}/../../../source/radeon_gpu_detective_backend/")

set(CMAKE_CXX_STANDARD 17)

# RDF.
add_definitions(-DRDF_CXX_BINDINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Source files that are common for Windows and Linux
file(GLOB RGD_TEST_SRC
    "*.cpp"
    "*.h"
    "../../../external/third_party/catch2/single_include/catch2/*.hpp"
)

add_executable(${PROJECT_NAME} ${RGD_TEST_SRC})

# Linker.
target_link_libraries(${PROJECT_NAME} rgd_backend rdf system_info)

# Warning level.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()