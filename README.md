# radeon_gpu_detective
RGD is a tool for post-mortem analysis of GPU crashes. 

The tool performs offline processing of AMD GPU crash dump files and generates crash analysis reports in text and JSON formats.

To generate AMD GPU crash dumps for your application, use Radeon Developer Panel (RDP) and follow the Crash Analysis help manual.

## Build Instructions ##
It is recommended to build the tool using the "pre_build.py" script which can be found under the "build" subdirectory.

**Steps:**

``
cd build
``


``
python pre_build.py
``

The script supports different options such as using different MSVC toolsets versions. For the list of options run the script with `-h`.

By default, a solution is generated for VS 2019. To generate a solution for a different VS version or to use a different MSVC toolchain use the `--vs` argument.
For example, to generate the solution for VS 2022 with the VS 2022 toolchain (MSVC 17), run:

``
python pre_build.py --vs 2022
``

## Running ##
Basic usage (text output):

``
rgd --parse <input .rgd crash dump file> -o <output text file>
``

Basic usage (JSON output):

``
rgd --parse <input .rgd crash dump file> --json <output JSON file>
``

For more options, run `rgd -h` to print the help manual.


## Usage ##
The rgd command line tool accepts AMD driver GPU crash dump files as an input (.rgd files) and generates crash analysis report files with summarized information that can assist in debugging GPU crashes.

The basic usage is:

``
rgd --parse <full path to input .rgd file> -o <full path to text output file>
``

The rgd command line tool's crash analysis output files include the following information by default:
* System information (CPUs, GPUs, driver, OS etc.)
* Execution marker tree for each command buffer which was in flight during the crash.
* Summary of the markers that were in progress during the crash (similar to the marker tree, just  without the hierarchy and only including markers that were in progress).
* Page fault summary (for crashes that were determined to be caused by a page fault)

Both text and JSON output files include the same information, in different representation. For simplicity, we will refer here to the human-readable textual output. Here are some more details about the crash analysis file's contents:

### Crash Analysis File Information ###
* Input crash dump file name: the full path to the .rgd file that was used to generate this file.
* Input crash dump file creation time
* RGD CLI version used

### System Information ###
This section is titled `SYSTEM INFO` and includes information about:
* Driver
* OS
* CPUs
* GPUs

### Markers in Progress ###

This section is titled `MARKERS IN PROGRESS` and contains information *only* about the execution markers that were in progress during the crash for each command buffer which was determined to be in flight during the crash. Here is the matching output for the tree below (see [EXECUTION MARKER TREE](#execution-marker-tree)):

```
Command Buffer ID: 0x2e
=======================
Frame 268 CL0/DownSamplePS/CmdDraw
Frame 268 CL0/DownSamplePS/CmdDraw
Frame 268 CL0/DownSamplePS/CmdDraw
Frame 268 CL0/DownSamplePS/CmdDraw
Frame 268 CL0/DownSamplePS/CmdDraw
Frame 268 CL0/Bloom/BlurPS/CmdDraw
```

Note that marker hierarchy is denoted by "/".

### Execution Marker Tree ###
This section is titled `EXECUTION MARKER TREE` and contains a tree describing the marker status for each command buffer that was determined to be in flight during the crash.

User-provided marker strings will be wrapped in "double quotes". Here is an example marker tree:

```
Command Buffer ID: 0x2e
=======================
[>] "Frame 268 CL0"
 ├─[X] "Depth + Normal + Motion Vector PrePass"
 ├─[X] "Shadow Cascade Pass"
 ├─[X] "TLAS Build"
 ├─[X] "Classify tiles"
 ├─[X] "Trace shadows"
 ├─[X] "Denoise shadows"
 ├─[X] CmdDispatch
 ├─[X] CmdDispatch
 ├─[X] "GltfPbrPass::DrawBatchList"
 ├─[X] "Skydome Proc"
 ├─[X] "GltfPbrPass::DrawBatchList"
 ├─[>] "DownSamplePS"
 └─[>] "Bloom"
    ├─[>] "BlurPS"
    │  ├─[>] CmdDraw
    │  └─[ ] CmdDraw
    ├─[ ] CmdDraw
    ├─[ ] "BlurPS"
    ├─[ ] CmdDraw
    ├─[ ] "BlurPS"
    ├─[ ] CmdDraw
    ├─[ ] "BlurPS"
    ├─[ ] CmdDraw
    ├─[ ] "BlurPS"
    └─[ ] CmdDraw
```


#### Configuring the Execution Marker Output with RGD CLI ####

RGD CLI exposes a few options that impact how the marker tree is generated:

* ``--marker-src`` include a suffix tag for each node in the tree indicating its origin (the component that submitted the marker). The supported components are:
    * [APP] for application marker.
    * [Driver-PAL] for markers originating from PAL.
    * [Driver-DX12] for markers originating from DXCP non-PAL code.

*  ``--expand-markers``: expand all parent nodes in the tree (RGD will collapse all nodes which do not have any sub-nodes in progress as these would generally be considered "noise" when trying to find the culprit marker).

#### Configuring the Page Fault Summary with RGD CLI ####

* ``--va-timeline``: print a table with all events that impacted the offending VA, sorted chronologically (note that this is only applicable for crashes that are caused by a page fault). Since this table can be extremely verbose, and since in most cases this table is not required for analyzing the crash, it is not included by default in the output file.

* ``--all-resources``: If specified, the tool's output will include all the resources regardless of their virtual address from the input crash dump file.


#### Page Fault Summary ####
If the crash was determined to be caused by a page fault, a section titled `PAGE FAULT SUMMARY` will include useful details about the page fault such as:

* `Offending VA`: the virtual address which triggered the page fault.
* `Resource timeline`: a timeline of the associated resources (all resources that resided in the offending VA) with relevant events such as Create, Bind and Destroy. These events will be sorted chronologically. Each line includes:
    * Time of event
    * Event type
    * Type of resource
    * Resource ID
    * Resource size
    * Resource name (if named, otherwise NULL)
* `Associated resources`: a list of all associated resources with their details, including:
    * Resource ID
    * Name (if named by the user)
    * Type and creation flags
    * Size
    * Virtual address and parent allocation base address
    * Commit type
    * Resource timeline that shows all events that are relevant to this specific resource in chronological order


*A note about time tracking:*

The general time format used by RGD is `<hh:mm:ss:clks>` which stands for `<hours, minutes, seconds, CPU clks>`. Beginning of time (`00:00:00.00`) is when the crash analysis session started (note that there is an expected lag between the start of the crashing process and the beginning of the crash analysis session, due to the time that takes to initialize crash analysis in the driver).

## Capturing AMD GPU Crash Dump Files ##

* To learn how to get started with RGD, see the [RGD quickstart guide](documentation/source/quickstart.rst). More information can be found on the [RGD help manual](documentation/source/help_manual.rst).
* The complete documentation can be found in the [Radeon Developer Tool Suite archive](https://gpuopen.com/rdts-windows/) under ``help/rgd/index.html``.

