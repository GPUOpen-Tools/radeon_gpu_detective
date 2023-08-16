#!python
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# Get RGD version from RGD/source/common/rgd_version_info.h.
#
# Usage:
#    python radeon_gpu_detective/build/utils/get_version.py [--major] [--minor] [--update] [--versionfile <file_path>]
#      --major     Return major version number
#      --minor     Return minor version number
#      --update    Return update version number
#      --versionfile Use <file_path> as the full path name of for rgd_version_info.h
#
import os
import argparse

# Get full path to script to support run from anywhere.
SCRIPTROOT = os.path.dirname(os.path.realpath(__file__))

# Handle command line arguments.
PARSER = argparse.ArgumentParser(description='Get RGD version information')
PARSER.add_argument('--major', action='store_true', default=False, help='Return value of MAJOR version string')
PARSER.add_argument('--minor', action='store_true', default=False, help='Return value of MINOR version string')
PARSER.add_argument('--update', action='store_true', default=False, help='Return the value of UPDATE version string')
PARSER.add_argument('--versionfile', action='store', default=None, help='Use alternate path for file path')
VERSIONARGS = PARSER.parse_args()

# Initialize file for search.
RGDVERSIONFILE = os.path.normpath(os.path.join(SCRIPTROOT, '../..', 'source/radeon_gpu_detective_cli/rgd_version_info.h'))
RGDVERSIONDATA = None
if not VERSIONARGS.versionfile == None:
    RGDVERSIONFILE = os.path.normpath(VERSIONARGS.versionfile)
if os.path.exists(RGDVERSIONFILE):
    RGDVERSIONDATA = open(RGDVERSIONFILE)
else:
    print("ERROR: Unable to open file: %s"%RGDVERSIONFILE)
    sys.exit(1)

# Get major, minor, and update version strings.
MAJOR = None
MINOR = None
UPDATE = None
for line in RGDVERSIONDATA:
    if 'define RGD_VERSION_MAJOR  ' in line:
        MAJOR = (line.split()[2])
    if 'define RGD_VERSION_MINOR ' in line:
        MINOR = (line.split()[2])
    if 'define RGD_VERSION_UPDATE ' in line:
        UPDATE = (line.split()[2])

if VERSIONARGS.major == True:
    print(MAJOR)
if VERSIONARGS.minor == True:
    print(MINOR)
if VERSIONARGS.update == True:
    print(UPDATE)
