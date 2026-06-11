# RGD Automated Test Kit
The RGD automated test is a set of scripts and executable files that can generate AMD GPU crash dump (.rgd) files and validate the contents of the generated files. Its primary goal is to check for driver regressions.

## Usage Instructions ##
* Download test kit and extract
* Run: python TestRunner.py
    * python TestRunner.py -h  should provide detailed help and more options

## Platform Support ##
Supported APIs: DirectX 12, Vulkan

Supported OS: Windows 10, Windows 11

Supported hardware: Navi2, Navi3, and Navi4 GPUs


## Dependencies ##
* Python 3.x (tested with 3.6.5)

# Disabled test cases
DX12 Case 6: Test disabled due to issues triggering TDR and terminating incorrectly; succeeds on first run after reboot then fails on future runs. Causes subsequent test failures. (RDNA4 device 25.10.2).
DX12 Case 6.1: Test disabled; same issue as Case 6.
VK Case 14: Test disabled due to issues triggering TDR and terminating incorrectly. Causes subsequent test failures. (RDNA4 device - 25.10.2)