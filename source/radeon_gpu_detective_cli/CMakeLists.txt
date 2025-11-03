#=============================================================================
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  RGD CLI CMakeLists file.
#=============================================================================

cmake_minimum_required (VERSION 3.10)
project (radeon_gpu_detective_cli)

# Include cxxopts for command line parsing.
include_directories("${PROJECT_SOURCE_DIR}/../../external/third_party/cxxopts/include/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/rmv/source/parser/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/rmv/source/backend/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/third_party/")
include_directories("${PROJECT_SOURCE_DIR}/../radeon_gpu_detective_backend/")

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_USE_REAL_VA)
# RDF.
add_definitions(-DRDF_CXX_BINDINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

IF (WIN32)
    add_definitions(-DRDF_PLATFORM_WINDOWS)
    add_link_options(/STACK:16777216)
ELSEIF(UNIX)
    add_definitions(-DRDF_PLATFORM_UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=16777216")
ENDIF(WIN32)

# Source files that are common for Windows and Linux
file(GLOB RGD_CLI_SRC
    "*.cpp"
    "*.h"
    "../../external/cxxopts/*.hpp"
)

set( RGD_WINDOWS_SRC
    "windows/resource.h"
    "windows/rgd.rc"
)

IF (WIN32)
    add_executable(radeon_gpu_detective_cli ${RGD_CLI_SRC} ${RGD_WINDOWS_SRC})
ELSEIF(UNIX)
    add_executable(radeon_gpu_detective_cli ${RGD_CLI_SRC})
ENDIF(WIN32)
set_target_properties(radeon_gpu_detective_cli PROPERTIES OUTPUT_NAME rgd)

# Linker.
target_link_libraries(radeon_gpu_detective_cli rgd_backend rdf RmvBackend RmvParser system_info ComgrUtils ${CMAKE_DL_LIBS})

# Warning level.
if(MSVC)
  target_compile_options(radeon_gpu_detective_cli PRIVATE /W4)
endif()

# Setup the post-build script.
# List of all dynamically loaded libraries that need to be copied into the output
set( PLUGINS
  ${AMD_COMGR_SHAREDLIB}
  ${AMDGPU_DIS_SHAREDLIB}
)

# Copy all plugins (aka dynamically loaded shared libraries) into the appropriate output directory.
#
IF (WIN32)
    # For WIN32 shared libraries live in the same directory as the executable
    set(PLUGIN_DST $<TARGET_FILE_DIR:${PROJECT_NAME}>)
ELSEIF (UNIX)
    # For Unix, shared libraries are copied into a /lib subdirectory
    set(PLUGIN_DST $<TARGET_FILE_DIR:${PROJECT_NAME}>/lib/)
ENDIF()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_DST}
)

FOREACH (PLUGIN ${PLUGINS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "copying ${PLUGIN} to ${PLUGIN_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PLUGIN} ${PLUGIN_DST}
        )
ENDFOREACH()

# Copy the DXC binary to the output folder
set(DXC_DST "$<TARGET_FILE_DIR:${PROJECT_NAME}>/utils/dx12/dxc/")
IF (WIN32)
    set(DXC_SRC "${PROJECT_SOURCE_DIR}/../../external/third_party/DXC/bin/x64/dxc.exe")
ELSEIF (UNIX)
    set(DXC_SRC "${PROJECT_SOURCE_DIR}/../../external/third_party/DXC/bin/dxc")
ENDIF()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DXC_DST}
    COMMAND ${CMAKE_COMMAND} -E echo "copying ${DXC_SRC} to ${DXC_DST}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DXC_SRC} ${DXC_DST}
)

# Copy the ISA spec files to the output folder
set(ISA_SPEC_DST "$<TARGET_FILE_DIR:${PROJECT_NAME}>/utils/isa_spec/")
set(ISA_SPEC_SRC "${PROJECT_SOURCE_DIR}/../../external/isa_spec_xml/")

# List of all ISA spec files to copy
set(ISA_SPEC_FILES
    "amdgpu_isa_rdna3.xml"
    "amdgpu_isa_rdna3_5.xml"
    "amdgpu_isa_rdna4.xml"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISA_SPEC_DST}
)

FOREACH (ISA_SPEC_FILE ${ISA_SPEC_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "copying ${ISA_SPEC_SRC}${ISA_SPEC_FILE} to ${ISA_SPEC_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ISA_SPEC_SRC}${ISA_SPEC_FILE} ${ISA_SPEC_DST}
    )
ENDFOREACH()