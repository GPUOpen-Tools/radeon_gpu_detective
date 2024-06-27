//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  main entry point of RGD Test app.
//=============================================================================

#include <iostream>
#include <cassert>

// catch2.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "test_rgd_file.h"
#include "rgd_utils.h"

struct TestConfig
{
    bool        is_page_fault = false;
    std::string file_path;
};

static TestConfig test_config;

static bool InitializeTestOptions(const char* file_name, bool is_page_fault)
{
    assert(file_name != nullptr);
    test_config.file_path.assign(file_name);
    test_config.is_page_fault = is_page_fault;
    
    if (test_config.file_path.empty())
    {
        std::cout << "ERROR: Missing path to .rgd file" << std::endl;
        return false;
    }

    if (test_config.file_path.find(".rgd") == std::string::npos)
    {
        test_config.file_path.assign("");
        return false;
    }
    
    return true;
}

bool is_empty(const std::string&rgd_file_path)
{
    std::ifstream file;
    file.open(rgd_file_path);
    bool is_empty = file.peek() == std::ifstream::traits_type::eof();

    if (is_empty)
    {
        RgdUtils::PrintMessage("crash dump file is empty.", RgdMessageType::kError, true);
    }
    return is_empty;
}

std::shared_ptr<TestRgdFile> test_rgd_file_instance = nullptr;

void SetupTestRgdFileInstance()
{
    // Parse the input .rgd file only once for all test cases.
    static bool is_test_rgd_file_instance = false;
    if (!is_test_rgd_file_instance)
    {
        test_rgd_file_instance = std::make_shared<TestRgdFile>(test_config.file_path);
    }
}

int32_t main(int argc, char* argv[])
{
    fflush(stdout);

    using namespace Catch::clara;
    Catch::Session session;

    // The crash dump file to load.
    std::string input_file_path;
    bool is_page_fault = false;

    // Add to Catch's composite command line parser.
    auto cli = Opt(input_file_path, "input_file_path")["--rgd"]("The path to the trace file")
        | Opt(is_page_fault)["--page-fault"]("Enable page fault handling")
        | session.cli();

    int test_result = -1;

    session.cli(cli);
    int result = session.applyCommandLine(argc, argv);
    if (result != 0)
    {
        return test_result;
    }

    bool initSucceeded = InitializeTestOptions(input_file_path.c_str(), is_page_fault);
    if (!initSucceeded)
    {
        return test_result;
    }
    else
    {
        std::cout << "Input file: " << input_file_path << std::endl;
        test_result = session.run();
    }
    // Test result will contain -1 if the tests were not run (some kind of initialization problem)
    // or an integer containing the number of tests that failed.
    return test_result;
}

TEST_CASE("TestEmptyFile", "[TestRgdFileStructure]")
{
    REQUIRE(is_empty(test_config.file_path) == false);
}

TEST_CASE("ParseRgdFile", "[TestRgdFileStructure, TestDDEventChunks]")
{
    SetupTestRgdFileInstance();
    SECTION("Valid file structure")
    {
        REQUIRE(test_rgd_file_instance->ParseRgdFile(test_config.is_page_fault));
        REQUIRE(test_rgd_file_instance->IsSystemInfoParsed());
        REQUIRE(!test_rgd_file_instance->IsRdfParsingError());
        REQUIRE(test_rgd_file_instance->IsAppMarkersFound());
        REQUIRE(test_rgd_file_instance->IsMarkerContextFound());
    }
}