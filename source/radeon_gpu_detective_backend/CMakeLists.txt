#=============================================================================
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  RGD Backend CMakeLists file.
#=============================================================================

cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (rgd_backend)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include cxxopts for command line parsing.
include_directories("${PROJECT_SOURCE_DIR}/../../external/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/rmv/source/parser/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/rmv/source/backend/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/third_party/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/rdf/rdf/inc/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/comgr_utils/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/comgr_package/comgr/include/")
include_directories(AFTER ${AMDGPU_DIS_INCLUDE_DIR} )
include_directories(AFTER ${AMD_COMGR_INCLUDE_DIR} )

# Add path to miniz library
include_directories("${PROJECT_SOURCE_DIR}/../../external/miniz/")

set(CMAKE_CXX_STANDARD 17)

# RDF.
add_definitions(-DSYSTEM_INFO_ENABLE_RDF)
add_definitions(-DRDF_CXX_BINDINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

IF (WIN32)
    add_definitions(-DRDF_PLATFORM_WINDOWS)
    add_link_options(/STACK:16777216)
ELSEIF(UNIX)
    add_definitions(-DRDF_PLATFORM_UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=16777216")
ENDIF(WIN32)

# RGD backend source files that are common for Windows and Linux
file (GLOB RGD_BACKEND_SOURCE
    "*.h"
    "*.cpp"
    "../../external/nlohmann/*.hpp"
)

# Find the miniz library source files
file (GLOB MINIZ_SOURCE
    "../../external/third_party/miniz/*.c"
    "../../external/third_party/miniz/*.h"
)

# Add miniz source files to the build
add_library(${PROJECT_NAME} STATIC ${RGD_BACKEND_SOURCE} ${MINIZ_SOURCE})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
