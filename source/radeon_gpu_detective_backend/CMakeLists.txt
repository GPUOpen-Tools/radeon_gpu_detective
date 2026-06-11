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
include_directories("${PROJECT_SOURCE_DIR}/../../external/comgr_utils/")
include_directories("${PROJECT_SOURCE_DIR}/../../external/isa_spec_manager/include/")
include_directories(AFTER ${AMDGPU_DIS_INCLUDE_DIR} )

# Add path to miniz library
include_directories("${PROJECT_SOURCE_DIR}/../../external/miniz/")

set(CMAKE_CXX_STANDARD 17)

IF (WIN32)
    add_link_options(/STACK:16777216)
ELSEIF(UNIX)
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

# Link library dependencies.
# ComgrUtils is PUBLIC so the comgr include path (carried by the imported `comgr` target it
# links) propagates to consumers like rgd_test that include amd_comgr.h transitively.
target_link_libraries(${PROJECT_NAME}
    PUBLIC rdf system_info ComgrUtils
    PRIVATE isa_decoder)

# PixEventsDecoder is Windows-only (Microsoft's upstream library uses <Windows.h> types).
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE PixEventsDecoder)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Include FetchContent module for downloading ISA spec XML files
include(FetchContent)

set(ISA_DECODER_XML_URL "https://gpuopen.com/download/AMD_GPU_MR_ISA_XML_2026_03_05.zip")
set(ISA_DECODER_XML_DIR "${PROJECT_SOURCE_DIR}/../../external/isa_spec_xml")

FetchContent_Declare(
    isa_spec_xml
    URL ${ISA_DECODER_XML_URL}
    SOURCE_DIR ${ISA_DECODER_XML_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

message(STATUS "Downloading ${ISA_DECODER_XML_URL} into ${ISA_DECODER_XML_DIR}")

FetchContent_MakeAvailable(isa_spec_xml)

if (EXISTS "${ISA_DECODER_XML_DIR}")
    message(STATUS "ISA decoder XML directory created successfully at ${ISA_DECODER_XML_DIR}")
else ()
    message(FATAL_ERROR "Directory ${ISA_DECODER_XML_DIR} was not created.")
endif ()

# Validate that the downloaded XML schema version is compatible with the isa_spec_manager API version.
# The API's IsCompatible() always returns true (even for incompatible versions), so we check here
# at configure time to catch mismatches early instead of getting runtime crashes.
set(ISA_API_VERSION_HEADER "${PROJECT_SOURCE_DIR}/../../external/isa_spec_manager/include/amdisa/api_version.h")
if (EXISTS "${ISA_API_VERSION_HEADER}")
    file(READ "${ISA_API_VERSION_HEADER}" _api_version_contents)
    string(REGEX MATCH "AMDISA_DECODE_API_VERSION_MAJOR ([0-9]+)" _api_version_match "${_api_version_contents}")
    if (_api_version_match)
        set(ISA_API_MAJOR "${CMAKE_MATCH_1}")

        # Read schema version from first XML file found.
        file(GLOB _isa_xml_files "${ISA_DECODER_XML_DIR}/*.xml")
        list(LENGTH _isa_xml_files _xml_count)
        if (_xml_count GREATER 0)
            list(GET _isa_xml_files 0 _first_xml)
            file(READ "${_first_xml}" _xml_contents)
            string(REGEX MATCH "<SchemaVersion>([0-9]+)\\.([0-9]+)\\.([0-9]+)</SchemaVersion>" _xml_schema_match "${_xml_contents}")
            if (_xml_schema_match)
                set(XML_SCHEMA_MAJOR "${CMAKE_MATCH_1}")
                set(XML_SCHEMA_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
                message(STATUS "ISA spec XML schema version: ${XML_SCHEMA_VERSION}, API major version: ${ISA_API_MAJOR}")
                if (NOT "${XML_SCHEMA_MAJOR}" STREQUAL "${ISA_API_MAJOR}")
                    message(FATAL_ERROR
                        "ISA spec XML schema major version (${XML_SCHEMA_MAJOR}) does not match "
                        "isa_spec_manager API major version (${ISA_API_MAJOR}). "
                        "The downloaded XML files from ${ISA_DECODER_XML_URL} may be incompatible. "
                        "Update the isa_spec_manager dependency or check for a compatible XML release.")
                endif ()
            else ()
                message(WARNING
                    "Could not parse SchemaVersion from XML file ${_first_xml}; "
                    "skipping ISA schema/API compatibility validation.")
            endif ()
        else ()
            message(WARNING "No XML files found in ${ISA_DECODER_XML_DIR} to validate schema version.")
        endif ()
    else ()
        message(WARNING
            "Could not parse AMDISA_DECODE_API_VERSION_MAJOR from ${ISA_API_VERSION_HEADER}; "
            "skipping ISA schema/API compatibility validation.")
    endif ()
endif ()
