#=============================================================================
# Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file
#  @brief  Wraps Microsoft's open-source PixEvents decoder as a static library.
#=============================================================================

cmake_minimum_required (VERSION 3.10)
project(PixEventsDecoder)

set(PIXEVENTS_DIR ${PROJECT_SOURCE_DIR}/../../../external/third_party/pix_events)

set(SOURCES
    "${PIXEVENTS_DIR}/decoder/lib/BlockParser.cpp"
    "${PIXEVENTS_DIR}/decoder/lib/EventReading.cpp"
    "${PIXEVENTS_DIR}/decoder/lib/PixEventDecoder.cpp"
)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Silence upstream warnings (we treat warnings as errors):
#   EventReading.cpp / PixEventDecoder.cpp emit C4244 / C4267 conversion warnings.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4244 /wd4267)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${PIXEVENTS_DIR}/decoder/include/
    PRIVATE ${PIXEVENTS_DIR}
            ${PIXEVENTS_DIR}/include/
)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
