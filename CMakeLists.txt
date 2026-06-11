#=============================================================================
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  Top level CMakeLists file.
#=============================================================================

cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project (RGD)

# Define version information
set(RGD_MAJOR_VERSION 1)
set(RGD_MINOR_VERSION 6)
set(RGD_PATCH_NUMBER 3)
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
set(AMDGPU_DIS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/amd_gpu_dis/Include/)
include_directories("${PROJECT_SOURCE_DIR}/external/")
include_directories("${PROJECT_SOURCE_DIR}/external/third_party/")
include_directories("${PROJECT_SOURCE_DIR}/external/comgr_utils/")

# RDF.
set(RDF_STATIC ON)
set(RDF_ENABLE_CXX_BINDINGS ON)
add_subdirectory(external/rdf)

# System info.
set(SYSTEM_INFO_BUILD_RDF_INTERFACES ON)
add_subdirectory(external/system_info_utils)

# RMV.
add_subdirectory(external/rmv/source/parser parser)
add_subdirectory(external/rmv/source/backend backend)
# FIXME
# RMV includes RDF and SystemInfo incorrectly and set up their include directories manually.
# RGD previously used them similarly incorrectly, which made RMV work.
# We need to tell CMake that these RMV components use RDF and SystemInfo
# as otherwise SystemInfo will be unable to find RDF's headers when building RMV.
target_link_libraries(RmvParser PRIVATE rdf)
target_link_libraries(RmvBackend PRIVATE rdf system_info)

# ISA Spec Manager.
set(EXCLUDE_ISA_CLI_EXAMPLES_TESTS ON)
add_subdirectory(external/isa_spec_manager)

# Test rgd app.
add_subdirectory(test/source/rgd_test)

# COMGR. comgr_utils provides a `comgr` (or `comgr_plugin` in dynamic mode)
# imported target that wraps the comgr DLL fetched via FetchContent.
# Enable dynamic loading so the DLL is dlopen'd at runtime (matches prior RGD behavior).
set(COMGR_UTILS_DYNAMIC_LINKING ON CACHE BOOL "Load comgr at runtime via dlopen/LoadLibrary" FORCE)
add_subdirectory(external/comgr_utils ComgrUtils)

# Microsoft PixEvents decoder (used to decode PIX runtime markers in crash dumps).
# Windows-only — the upstream PIX header pulls in <Windows.h> types (UINT64, SAL annotations).
if(WIN32)
    add_subdirectory(source/radeon_gpu_detective_backend/pix_events_decoder pix_events_decoder)
endif()

# rgd command line tool.
add_subdirectory (source/radeon_gpu_detective_backend)
add_subdirectory (source/radeon_gpu_detective_cli)

# Define the location of dependent libraries.
# Note: comgr is now provided by the `comgr_plugin` imported target from external/comgr_utils
# (created by FetchContent inside that subproject). Use $<TARGET_FILE:comgr_plugin> for paths.
IF(WIN32)
    set(AMDGPU_DIS_LIBS_DIR ${PROJECT_SOURCE_DIR}/external/amd_gpu_dis/Lib/VS2022/x64)
ELSEIF(UNIX)
    set(AMDGPU_DIS_LIBS_DIR ${PROJECT_SOURCE_DIR}/external/amd_gpu_dis/Lib/Linux/x64)
ENDIF()

set (AMDGPU_DIS_SHARED ${CMAKE_SHARED_LIBRARY_PREFIX}amdgpu_dis${CMAKE_SHARED_LIBRARY_SUFFIX})

find_library(AMDGPU_DIS_LIB amdgpu_dis PATHS ${AMDGPU_DIS_LIBS_DIR} NO_DEFAULT_PATH)
find_file(AMDGPU_DIS_SHAREDLIB ${AMDGPU_DIS_SHARED} PATHS ${AMDGPU_DIS_LIBS_DIR} NO_DEFAULT_PATH)

message ("AMDGPU_Dis found  at ${AMDGPU_DIS_LIB}")
message ("AMDGPU_Dis shared at ${AMDGPU_DIS_SHAREDLIB}")
message ("AMDGPU_Dis include at ${AMDGPU_DIS_INCLUDE_DIR}")

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

# Define the location of dependent libraries for RGD CLI.
IF(WIN32)
    install(FILES external/amd_gpu_dis/Lib/VS2022/x64/amdgpu_dis.dll
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)
    install(FILES $<TARGET_FILE:comgr_plugin>
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}
        COMPONENT rgd_cli_component)
    install(FILES external/third_party/DXC/bin/x64/dxc.exe
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/utils/dx12/dxc
        COMPONENT rgd_cli_component)
ELSEIF(UNIX)
    install(FILES ${AMDGPU_DIS_SHAREDLIB}
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/lib/
        COMPONENT rgd_cli_component)
    install(FILES $<TARGET_FILE:comgr_plugin>
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/lib/
        COMPONENT rgd_cli_component)
    install(FILES external/third_party/DXC/bin/dxc
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/utils/dx12/dxc
        COMPONENT rgd_cli_component)
ENDIF()

# Install ISA spec files preserving the utils/isa_spec folder structure
install(FILES external/isa_spec_xml/amdgpu_isa_rdna3.xml
              external/isa_spec_xml/amdgpu_isa_rdna3_5.xml
              external/isa_spec_xml/amdgpu_isa_rdna4.xml
        DESTINATION ./${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/utils/isa_spec
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

install(FILES test/README.md
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(DIRECTORY test/scripts/input_description_files
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(FILES test/scripts/TestRunner.py
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

install(FILES test/scripts/JsonTests.py
        DESTINATION ./${CPACK_RGD_TEST_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)

# Define the location of dependent libraries and exe files for RGD TEST.
IF(WIN32)
    install(FILES external/amd_gpu_dis/Lib/VS2022/x64/amdgpu_dis.dll
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)
    install(FILES $<TARGET_FILE:comgr_plugin>
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/
        COMPONENT rgd_test_component)
    install(FILES external/third_party/DXC/bin/x64/dxc.exe
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/utils/dx12/dxc
        COMPONENT rgd_test_component)
ENDIF()

# Install ISA spec files for RGD TEST component preserving the utils/isa_spec folder structure
install(FILES external/isa_spec_xml/amdgpu_isa_rdna3.xml
              external/isa_spec_xml/amdgpu_isa_rdna3_5.xml
              external/isa_spec_xml/amdgpu_isa_rdna4.xml
        DESTINATION ./${CPACK_ARCHIVE_RGD_TEST_COMPONENT_FILE_NAME}/${CPACK_RGD_CLI_FULL_PACKAGE_NAME}/utils/isa_spec
        COMPONENT rgd_test_component)

include(CPack)

cpack_add_component(rgd_cli_component
        DISPLAY_NAME "Radeon GPU Detective ${CPACK_PACKAGE_VERSION} ${RGD_BUILD_SUFFIX}"
        DESCRIPTION "RGD application")

cpack_add_component(rgd_test_component
        DISPLAY_NAME "RGD Tests ${CPACK_PACKAGE_VERSION} ${RGD_BUILD_SUFFIX}"
        DESCRIPTION "RGD backend tests and driver sanity tests")
