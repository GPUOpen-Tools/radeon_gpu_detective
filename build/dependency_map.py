#! python3
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
    github_root + "nlohmann/json"                          : ["../external/third_party/json",      "v3.9.1"],
    github_root + "jarro2783/cxxopts"                      : ["../external/third_party/cxxopts",   "v3.0.0"],
    github_root + "GPUOpen-Drivers/libamdrdf"              : ["../external/rdf",                   "v1.4.2"],
    github_root + "GPUOpen-Tools/radeon_memory_visualizer" : ["../external/rmv",                   "bdc5116f61212713e067b395236fa527422c956d"],
    github_root + "GPUOpen-Tools/system_info_utils"        : ["../external/system_info_utils",     "928d2a2ff77fef14c155a2730bf796ab3ca85cfe"],
    github_root + "GPUOpen-Tools/isa_spec_manager"         : ["../external/isa_spec_manager",      "v1.1.0"],
    github_root + "GPUOpen-Tools/comgr_utils"              : ["../external/comgr_utils",           "2b35675e46f64806c410bd55f1125958fe5d92e0"],
    github_root + "catchorg/Catch2"                        : ["../external/third_party/catch2",    "v2.13.6"],
    github_root + "sheredom/subprocess.h"                  : ["../external/third_party/subprocess","b49c56e9fe214488493021017bf3954b91c7c1f5"]
}

####### URL Dependencies #######

if sys.platform == "win32":
    comgr_package = "https://github.com/GPUOpen-Tools/comgr_utils/releases/download/rdts-2025-11-03/COMGR_windows_93.zip"
    dxc_package = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2502/dxc_2025_02_20.zip"
elif sys.platform.startswith('linux') == True:
    comgr_package = "https://github.com/GPUOpen-Tools/comgr_utils/releases/download/rdts-2025-11-03/COMGR_linux_93.tar.gz"
    dxc_package = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2502/linux_dxc_2025_02_20.x86_64.tar.gz"
else:
    log_print("Unsupported Platform")
    sys.exit(-1)
    
# miniz package link (Windows and Linux).
miniz_package = "https://github.com/richgel999/miniz/releases/download/3.0.2/miniz-3.0.2.zip"

# Downloads required for Windows builds.
url_mapping_win = {
    comgr_package :  "../external/comgr",
    dxc_package : "../external/third_party/DXC",
    miniz_package : "../external/third_party/miniz",
}

# Downloads required for Linux builds.
url_mapping_linux = {
    comgr_package :  "../external/comgr",
    dxc_package : "../external/third_party/DXC",
    miniz_package : "../external/third_party/miniz",
}
