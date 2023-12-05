#=============================================================================
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  Top level CMakeLists file.
#=============================================================================

cmake_minimum_required (VERSION 3.10)
project (RGD)

# Define version information
set(RGD_MAJOR_VERSION 1)
set(RGD_MINOR_VERSION 1)
set(RGD_PATCH_NUMBER 0)
if (NOT RGD_BUILD_NUMBER)
    set(RGD_BUILD_NUMBER 0)
endif ()

string(TIMESTAMP DATE "\"%m/%d/%Y\"")
string(TIMESTAMP YEAR "\"%Y\"")

configure_file("${CMAKE_SOURCE_DIR}/Buildinfo.properties.in" "${CMAKE_SOURCE_DIR}/Buildinfo.properties")
configure_file("${CMAKE_SOURCE_DIR}/source/radeon_gpu_detective_cli/rgd_version_info.h.in" "${CMAKE_SOURCE_DIR}/source/radeon_gpu_detective_cli/rgd_version_info.h")

# Include directories for all projects.
include_directories("${PROJECT_SOURCE_DIR}/external/")
include_directories("${PROJECT_SOURCE_DIR}/external/rdf/rdf/inc/")
include_directories("${PROJECT_SOURCE_DIR}/external/third_party/")

# rgd command line tool.
add_subdirectory (source/radeon_gpu_detective_cli)

# RDF.
add_definitions(-DRDF_CXX_BINDINGS)
option(RDF_STATIC              "Build RDF as a static library" ON)
add_subdirectory(external/rdf/imported/zstd)
add_subdirectory(external/rdf/rdf)

# RMV.
add_subdirectory(external/rmv/source/parser parser)
add_subdirectory(external/rmv/source/backend backend)

# System info.
set(SYSTEM_INFO_BUILD_RDF_INTERFACES ON)
add_subdirectory(external/system_info_utils)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CPACK_PACKAGE_NAME "radeon_gpu_detective_debug")
else ()
    set(CPACK_PACKAGE_NAME "radeon_gpu_detective")
endif ()
set(CPACK_PACKAGE_VERSION "${RGD_MAJOR_VERSION}.${RGD_MINOR_VERSION}.${RGD_PATCH_NUMBER}.${RGD_BUILD_NUMBER}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# Archive configuration
install(TARGETS radeon_gpu_detective_cli
        RUNTIME
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}
        COMPONENT "${CPACK_PACKAGE_NAME}")

install(FILES RGD_NOTICES.txt
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}
        COMPONENT "${CPACK_PACKAGE_NAME}")
install(FILES README.md
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}
        COMPONENT "${CPACK_PACKAGE_NAME}")
install(FILES RGD_RELEASE_NOTES.txt
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}
        COMPONENT "${CPACK_PACKAGE_NAME}")
install(DIRECTORY documentation/build/rgd
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}/help/
        COMPONENT "${CPACK_PACKAGE_NAME}")
install(DIRECTORY samples
        DESTINATION ./${CPACK_PACKAGE_FILE_NAME}
        COMPONENT "${CPACK_PACKAGE_NAME}")


include(CPack)

cpack_add_component("${CPACK_PACKAGE_NAME}"
        DISPLAY_NAME "Radeon GPU Detective ${CPACK_PACKAGE_VERSION} ${RGD_BUILD_SUFFIX}"
        DESCRIPTION "RGD application")
