#! python3
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Script to fetch all external git and/or downloadable dependencies needed to build the project
#
#   fetch_dependencies.py
#
# Each git repo will be updated to the commit specified in the "gitMapping" table.

import os
import subprocess
import sys
import zipfile
import tarfile
import platform
import argparse
import stat
import shutil
import errno


# Check for the python 3.x name and import it as the 2.x name
try:
    import urllib.request as urllib
# if that failed, then try the 2.x name
except ImportError:
    import urllib

# to allow the script to be run from anywhere - not just the cwd - store the absolute path to the script file
script_root = os.path.dirname(os.path.realpath(__file__))

# also store the basename of the file
script_name = os.path.basename(__file__)

# Print a message to the console with appropriate pre-amble
def log_print(message):
    print ("\n" + script_name + ": " + message)
    sys.stdout.flush()
    
# add script root to support import of URL and git maps
sys.path.append(script_root)
from dependency_map import git_mapping
from dependency_map import url_mapping_win
from dependency_map import url_mapping_linux

# Delete files and directory when update is requested
def handle_remove_readonly(func, path, exc):
    excvalue = exc[1]
    if func in (os.rmdir, os.remove, os.unlink) and excvalue.errno == errno.EACCES:
        # Set the permission to 0777 - allowing the script to remove the file or directory even if it was previously read-only.
        os.chmod(path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
        func(path)
    else:
        raise

# Download a zip or tgz file from the specified URL and unzip into the directory defined by destination.
# The destination directory will be created if it doesn't exist
# if the 'update' parameter is true then the existing file and output directory will be deleted and re-created
# TODO - this function needs to handle errors gracefully when URL is incorrect or inaccessible
def download_url_dependencies(url_mapping, update, retry_count = 10):
    for url in url_mapping:
        # convert targetPath to OS specific format
        tmp_path = os.path.join(script_root, url_mapping[url])
        # clean up path, collapsing any ../ and converting / to \ for Windows
        target_path = os.path.normpath(tmp_path)
        
        # if update is defined - delete files and directory if they exist
        if update and os.path.exists(target_path):
            shutil.rmtree(target_path, onerror=handle_remove_readonly)

        # make target directory if it doesn't exist
        if not os.path.isdir(target_path):
            os.makedirs(target_path)
        # generate the target zip file name from the source URL filename and the target path
        # note - this rule currently handles URLs that contain # and ? characters at the end
        # those currently used by Jenkins don't have this style
        zip_file_name = url.split('/')[-1].split('#')[0].split('?')[0]
        zip_path = os.path.join(target_path, zip_file_name)
        if os.path.isfile(zip_path):
            # File exists - print message and continue
            log_print("URL Dependency %s found and not updated" % zip_path)
        else:
            # File doesn't exist - download and unpack it
            log_print("Downloading " + url + " into " + zip_path)
            try:
                urllib.urlretrieve(url, zip_path)
            except urllib.ContentTooShortError:
                os.remove(zip_path)
                if retry_count > 0:
                    log_print("URL content too short. Retrying. Retries remaining: %d" % retry_count)
                    download_url_dependencies(url_mapping, update, retry_count - 1)
                return;
            # Unpack the downloaded file into the target directory
            if os.path.splitext(zip_path)[1] == ".zip":
                # if file extension is .zip then unzip it
                log_print("Extracting in " + target_path)
                zipfile.ZipFile(zip_path).extractall(target_path)
            elif os.path.splitext(zip_path)[1] == ".tgz" or os.path.splitext(zip_path)[1] == ".gz":
                # if file extension is .tgz then untar it
                log_print("Extracting in " + target_path)
                tarfile.open(zip_path).extractall(target_path)

# Clone or update a git repo
def update_git_dependencies(git_mapping, update):
    for git_repo in git_mapping:
        # add script directory to path
        tmp_path = os.path.join(script_root, git_mapping[git_repo][0])

        # clean up path, collapsing any ../ and converting / to \ for Windows
        path = os.path.normpath(tmp_path)

        # required commit
        reqd_commit = git_mapping[git_repo][1]

        do_checkout = False

        if update and os.path.exists(path):
            # directory exists and update requested - delete existing files and directory
            shutil.rmtree(path, onerror=handle_remove_readonly)

        if not os.path.isdir(path):
            # directory doesn't exist - clone from git
            log_print("Directory %s does not exist, using 'git clone' to get latest from %s" % (path, git_repo))
            p = subprocess.Popen((["git", "clone", git_repo ,path]), stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode == 0):
                do_checkout = True
            else:
                log_print("git clone failed with return code: %d" % p.returncode)
                return False
        else:
            # Directory exists and update not requested
            log_print("Git Dependency %s found and not updated" % git_repo)

        if do_checkout == True:
            log_print("Checking out required commit: %s" % reqd_commit)
            p = subprocess.Popen((["git", "checkout", reqd_commit]), cwd=path, stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode != 0):
                log_print("git checkout failed with return code: %d" % p.returncode)
                return False
            log_print("Ensuring any branch is on the head using git pull --ff-only origin %s" % reqd_commit)
            p = subprocess.Popen((["git", "pull", "--ff-only", "origin", reqd_commit]), cwd=path, stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode != 0):
                log_print("git merge failed with return code: %d" % p.returncode)
                return False

    return True

# Main body of update functionality
def do_fetch_dependencies(update, internal):
    # Print git version being used
    git_cmd = ["git", "--version"]
    git_output = subprocess.check_output(git_cmd, stderr=subprocess.STDOUT)
    log_print("%s" % git_output)

    # Update all git dependencies
    if update_git_dependencies(git_mapping, update):
        if sys.platform == "win32":
            download_url_dependencies(url_mapping_win, update)
        elif sys.platform.startswith('linux') == True:
            download_url_dependencies(url_mapping_linux, update)
        return True
    else:
        return False

if __name__ == '__main__':
    # fetch_dependencies.py executed as a script

    # parse the command line arguments
    parser = argparse.ArgumentParser(description="A script that fetches all the necessary build dependencies for the project")
    args = parser.parse_args()

    do_fetch_dependencies(True, args.internal)
