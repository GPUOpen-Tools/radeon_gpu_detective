# RGD Automated Test Kit
The RGD automated test is a set of scripts and executable files that can generate AMD GPU crash dump (.rgd) files and validate the contents of the generated files. Its primary goal is to check for driver regressions.

## Usage Instructions ##
* Download test kit and extract
* Run: python TestRunner.py
    * python TestRunner.py -h  should provide detailed help and more options

## Platform Support ##
Supported APIs: DirectX 12

Supported OS: Windows 10, Windows 11

Supported hardware: Navi2 and Navi3 GPUs


## Dependencies ##
* Python 3.x (tested with 3.6.5)

## More Details ##
* [RGD Automated Test Kit Confluence Page](https://confluence.amd.com/display/DEVTOOLS/RGD+Automated+Test+Kit)