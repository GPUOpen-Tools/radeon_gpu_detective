#! python3
# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#
# Definitions for project external dependencies to be fetched or updated before build.
#


import sys

# prevent generation of .pyc file
sys.dont_write_bytecode = True

####### Git Dependencies #######

# To allow for future updates where we may have cloned the project, store the root of
# the repo in a variable. In future, we can automatically calculate this based on the git config
github_root = "https://github.com/"

# Define a set of dependencies that exist as separate git projects.
# each git dependency has a desired directory where it will be cloned - along with a commit to checkout
git_mapping = {
    github_root     + "nlohmann/json"                                   : ["../external/third_party/json",    "v3.9.1"],
    github_root     + "jarro2783/cxxopts"                               : ["../external/third_party/cxxopts", "v3.0.0"],
    github_root     + "GPUOpen-Drivers/libamdrdf"                       : ["../external/rdf",                 "v1.1.2"],
    github_root     + "GPUOpen-Tools/radeon_memory_visualizer"          : ["../external/rmv",                 "9ff1cd0491ec4b040557f3e844c04018db6c3368"],
    github_root     + "GPUOpen-Tools/system_info_utils"                 : ["../external/system_info_utils",   "88a338a01949f8d8bad60a30b78b65300fd13a9f"],
    github_root     + "catchorg/Catch2"                                 : ["../external/third_party/catch2",  "v2.13.6"]
}

# Downloads required for Windows builds.
url_mapping_win = {}

# Downloads required for Linux builds.
url_mapping_linux = {}