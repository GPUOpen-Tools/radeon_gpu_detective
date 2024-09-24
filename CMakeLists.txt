#=============================================================================
# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  Top level CMakeLists file.
#=============================================================================

cmake_minimum_required (VERSION 3.10)
project (RGD)

# Define version information
set(RGD_MAJOR_VERSION 1)
set(RGD_MINOR_VERSION 3)
set(RGD_PATCH_NUMBER 0)
if (NOT RGD_BUILD_NUMBER)
    set(RGD_BUILD_NUMBER 0)
endif ()

string(TIMESTAMP DATE "\"%m/%d/%Y\"")
string(TIMESTAMP YEAR "\"%Y\"")

configure_file("${CMAKE_SOURCE_DIR}/Buildinfo.properties.in" "${CMAKE_SOURCE_DIR}/Buildinfo.properties")
configure_file("${CMAKE_SOURCE_DIR}/source/radeon_gpu_detective_backend/rgd_version_info.h.in" "${CMAKE_SOURCE_DIR}/source/radeon_gpu_detective_backend/rgd_version_info.h")

# Add cmake utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rmv/cmake")
include(dev_tools)

# Include directories for all projects.
include_directories("${PROJECT_SOURCE_DIR}/external/")
include_directories("${PROJECT_SOURCE_DIR}/external/rdf/rdf/inc/")
include_directories("${PROJECT_SOURCE_DIR}/external/third_party/")

# rgd command line tool.
add_subdirectory (source/radeon_gpu_detective_cli)

add_subdirectory (source/radeon_gpu_detective_backend)

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

# Test rgd app.
add_subdirectory(test/source/rgd_test)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING IGNORE)

set(CPACK_PACKAGE_VERSION "${RGD_MAJOR_VERSION}.${RGD_MINOR_VERSION}.${RGD_PATCH_NUMBER}.${RGD_BUILD_NUMBER}")

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CPACK_RGD_CLI_PACKAGE_NAME "radeon_gpu_detective_debug")
    set(CPACK_RGD_TEST_PACKAGE_NAME "rgd_test_debug")
else ()
    set(CPACK_RGD_CLI_PACKAGE_NAME "radeon_gpu_detective")
    set(CPACK_RGD_TEST_PACKAGE_NAME "rgd_test")
endif ()

set(CPACK_RGD_CLI_FULL_PACKAGE_NAME "${CPACK_RGD_CLI_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_RGD_TEST_FULL_PACKAGE_NAME "${CPACK_RGD_TEST_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_ARCHIVE_RGD_CLI_COMPONENT_FILE_NAME "${CPACK_RGD_CLI_FULL_PACKAGE_NAME}")
set(CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME "${CPACK_RGD_TEST_FULL_PACKAGE_NAME}")

# Archive RGD CLI configuration
install(TARGETS radeon_gpu_detective_cli
        RUNTIME
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)

install(FILES RGD_NOTICES.txt
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)
install(FILES README.md
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)
install(FILES RGD_RELEASE_NOTES.txt
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)
install(DIRECTORY documentation/build/rgd
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/help/
        COMPONENT rgd_cli_component)
install(DIRECTORY samples
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)

# Archive RGD TEST configuration
install(TARGETS rgd_test
        RUNTIME
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(TARGETS radeon_gpu_detective_cli
        RUNTIME
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(FILES test/RGD_TEST_README.md
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(DIRECTORY test/scripts/input_description_files
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(FILES test/scripts/TestRunner.py
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

include(CPack)

cpack_add_component(rgd_cli_component
        DISPLAY_NAME "Radeon GPU Detective ${CPACK_PACKAGE_VERSION} ${RGD_BUILD_SUFFIX}"
        DESCRIPTION "RGD application")

cpack_add_component(rgd_test_component
        DISPLAY_NAME "RGD Tests ${CPACK_PACKAGE_VERSION} ${RGD_BUILD_SUFFIX}"
        DESCRIPTION "RGD backend tests and driver sanity tests")
