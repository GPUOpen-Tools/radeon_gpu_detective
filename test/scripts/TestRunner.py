# Copyright © Advanced Micro Devices, Inc. All rights reserved.
#
#!/usr/bin/python

import sys
import argparse
import json
import subprocess
import logging
import os
import shutil
import glob
import platform
from pathlib import Path
from datetime import datetime
from datetime import timedelta
import time
import ctypes
import ctypes.wintypes as wintypes

script_folder = os.path.dirname(os.path.abspath(__file__))

# Time stamp used to generate unique folder and file names
def get_current_timestamp():
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    return timestamp

time_stamp = get_current_timestamp()

def get_test_kit_version_string():
    version_file_name = 'Version.txt'
    version_file_path = os.path.join(script_folder, version_file_name)
    version_string = []
    if os.path.exists(version_file_path):
        with open(version_file_path, 'r') as h:
            version_string = h.readlines()
        h.close()
    return version_string

def close_bug_report_popup_window():
    # Define constants for Windows API
    WM_CLOSE = 0x0010
    ERROR_REPORTING_TITLE = "AMD Bug Report Tool"

    # Define the callback function type for EnumWindows
    EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    
    # Define the callback function for EnumWindows
    def enum_windows_callback(hwnd, lParam):
        window_title = ctypes.create_string_buffer(1024)
        ctypes.windll.user32.GetWindowTextA(hwnd, window_title, 1024)
        if ERROR_REPORTING_TITLE in window_title.value.decode():
            ctypes.windll.user32.PostMessageA(hwnd, WM_CLOSE, 0, 0)
            return False
        return True
    
    # Convert the callback function to the proper type
    callback_func = EnumWindowsProc(enum_windows_callback)
    
    # Wait for the pop-up window to appear
    time.sleep(5)

    # Enumerate windows and close the pop-up if found
    ctypes.windll.user32.EnumWindows(callback_func, 0)

# Script options.
STR_SCRIPT_OPT_RETAIN_OUTPUT         = '--retain'
STR_SCRIPT_OPT_RGD_PATH              = '--rgd-cli'
STR_SCRIPT_OPT_RGD_TEST_PATH         = '--rgd-test'
STR_SCRIPT_OPT_CRASH_GENERATOR_PATH  = '--crash-generator'
STR_SCRIPT_OPT_TEST_DESC_FILE        = '--test'
STR_SCRIPT_OPT_TEST_API              = '--api'
STR_SCRIPT_OPT_VERBOSE               = '--verbose'
STR_SCRIPT_OPT_MODERN_OUTPUT         = '--modern-output'

# Default values
STR_DEFAULT_TEST_DESC_FILE = os.path.join('input_description_files', 'RgdDriverSanity.json')

# Supported API's
SUPPORTED_APIS   = ["DX12"]

# String constants
STR_RGD_TEST_SUFFIX_PASSED  = "......[PASSED]"
STR_RGD_TEST_SUFFIX_FAILED  = "......[FAILED]"
STR_RGD_TEST_SUFFIX_WARNING = "......[WARNING]"
STR_ERROR                   = "ERROR:"

def get_supported_api_str() -> str:
    supported_apis = ''
    for api in SUPPORTED_APIS:
        if supported_apis == '':
            supported_apis += api
        else:
            supported_apis += ', '
            supported_apis += api
    
    return supported_apis

# Logging levels
DEBUG       = 10 # Default logging level for file.
TEST_INFO   = 15
INFO        = 20 # --verbose logging level for console.
TEST_MSG    = 22 
TEST_PASS   = 32 # Default logging level for console.
WARNING     = 30
TEST_FAIL   = 37
ERROR       = 40
TEST_RESULT = 45
CRITICAL    = 100

logging.addLevelName(TEST_INFO, "")
logging.addLevelName(TEST_MSG, "")
logging.addLevelName(TEST_PASS, "PASS")
logging.addLevelName(TEST_FAIL, "*FAIL*")
logging.addLevelName(TEST_RESULT, "RESULT")

def test_info(self, message, *args, **kws):
    if self.isEnabledFor(TEST_INFO):
        self._log(TEST_INFO, message, args, **kws)

def test_msg(self, message, *args, **kws):
    if self.isEnabledFor(TEST_MSG):
        self._log(TEST_MSG, message, args, **kws)

def test_pass(self, message, *args, **kws):
    if self.isEnabledFor(TEST_PASS):
        self._log(TEST_PASS, message, args, **kws)

def test_fail(self, message, *args, **kws):
    if self.isEnabledFor(TEST_FAIL):
        self._log(TEST_FAIL, message, args, **kws)

def test_result(self, message, *args, **kws):
    if self.isEnabledFor(TEST_RESULT):
        self._log(TEST_RESULT, message, args, **kws)

logging.Logger.test_info   = test_info
logging.Logger.test_msg    = test_msg
logging.Logger.test_pass   = test_pass
logging.Logger.test_fail   = test_fail
logging.Logger.test_result = test_result

# DEBUG TEST_INFO INFO PASS FAIL WARNING ERROR CRITICAL

class Logger:

    def __init__(self) -> None:
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.DEBUG)
        self.log_file = ""
        # Console handler
        self.console_handler = logging.StreamHandler()
        self.console_handler.setLevel(logging.WARNING)
        formatter = logging.Formatter('%(levelname)+8s: %(message)s')
        self.console_handler.setFormatter(formatter)
        self.logger.addHandler(self.console_handler)

    def debug(self, message):
        self.logger.debug(message)

    def test_info(self, message):
        self.logger.test_info(message)

    def info(self, message):
        self.logger.info(message)
    
    def test_msg(self, message):
        self.logger.test_msg(message)
    
    def test_pass(self, message):
        self.logger.test_pass(message)
    
    def test_fail(self, message):
        self.logger.test_fail(message)

    def warning(self, message):
        self.logger.warning(message)
        
    def error(self, message):
        self.logger.error(message)
    
    def critical(self, message):
        self.logger.critical(message)

    def test_result(self, message):
        self.logger.test_result(message)

    def set_file_handler(self, log_dir):

        # Add file handler for the logger
        self.log_file = os.path.join(log_dir, f"Log_{time_stamp}.txt")
        file_handler = logging.FileHandler(self.log_file)
        file_handler.setLevel(logging.DEBUG)
        len = 8
        file_formatter = logging.Formatter('%(levelname)+8s: %(message)s')
        file_handler.setFormatter(file_formatter)
        self.logger.addHandler(file_handler)

    def set_console_verbosity(self, is_verbose: 'bool'):
        if is_verbose:
            self.console_handler.setLevel(logging.INFO)

    def get_longest_level_name_length(self):
        level_name = max(logging._levelToName.values(), key=len)
        return len(level_name)
    
    def disable_modern_logger(self):
        self.logger.setLevel(logging.CRITICAL + 1)
        self.console_handler.setLevel(logging.CRITICAL + 1)

class LegacyLogger:

    def __init__(self) -> None:
        self.capture_logger = logging.getLogger('capture_logger')
        self.backend_logger = logging.getLogger('backend_logger')
        self.summary_logger = logging.getLogger('summary_logger')
        self.capture_logger.setLevel(logging.INFO)
        self.backend_logger.setLevel(logging.INFO)
        self.summary_logger.setLevel(logging.INFO)
        self.capture_test_log_file = ""
        self.backend_test_log_file = ""
        self.summary_log_file = ""
        # Console handler
        self.console_handler = logging.StreamHandler()
        self.console_handler.setLevel(logging.INFO)
        self.console_formatter = logging.Formatter('%(message)s')
        self.console_handler.setFormatter(self.console_formatter)
        self.summary_logger.addHandler(self.console_handler)
        self.case_logger_dict = {}
        self.dir_extended_log = ""

    def capture_info(self, message):
        self.capture_logger.info(message)
    
    def backend_info(self, message):
        self.backend_logger.info(message)
    
    def summary_info(self, message):
        self.summary_logger.info(message)

    def set_dir_extended_log(self, log_dir):
        self.dir_extended_log = log_dir
    
    def set_file_handler(self, log_dir):

        # Add file handler for the logger
        self.capture_test_log_file = os.path.join(log_dir, f"CaptureTest_output_{time_stamp}.txt")
        self.backend_test_log_file = os.path.join(log_dir, f"BackendTest_output_{time_stamp}.txt")
        self.summary_log_file = os.path.join(log_dir, f"TestSummary_{time_stamp}.txt")
        capture_file_handler = logging.FileHandler(self.capture_test_log_file)
        backend_file_handler = logging.FileHandler(self.backend_test_log_file)
        summary_file_handler = logging.FileHandler(self.summary_log_file)

        file_formatter_capture = logging.Formatter('%(message)s')
        capture_file_handler.setFormatter(file_formatter_capture)

        file_formatter_backend = logging.Formatter('%(message)s')
        backend_file_handler.setFormatter(file_formatter_backend)

        # write console content to summary file output
        summary_file_handler.setFormatter(self.console_formatter)
        self.capture_logger.addHandler(capture_file_handler)
        self.backend_logger.addHandler(backend_file_handler)
        self.summary_logger.addHandler(summary_file_handler)
    
    def disable_legacy_logger(self):
        self.capture_logger.setLevel(logging.CRITICAL + 1)
        self.backend_logger.setLevel(logging.CRITICAL + 1)
        self.summary_logger.setLevel(logging.CRITICAL + 1)
        self.console_handler.setLevel(logging.CRITICAL + 1)

    def get_failure_logger_for_test_case(self, case_no):
        if case_no not in self.case_logger_dict:
            case_str = "case" + str(case_no)
            failed_case_logger = logging.getLogger(f"{case_str}_logger")
            failed_case_logger.setLevel(logging.INFO)
            failed_case_log_file = os.path.join(self.dir_extended_log, f"RGDTest-{time_stamp}-{case_str}-failure_extended_log.txt")
            failure_log_file_handler = logging.FileHandler(failed_case_log_file)
            file_formatter_failre_log = logging.Formatter('%(message)s')
            failure_log_file_handler.setFormatter(file_formatter_failre_log)
            failed_case_logger.addHandler(failure_log_file_handler)
            self.case_logger_dict[case_no] = failed_case_logger
            
        return self.case_logger_dict[case_no]
    
    def log_failure_info(self, case_no, message):
        failure_logger_test_case = self.get_failure_logger_for_test_case(case_no)
        failure_logger_test_case.info(message)

# Logger
logger = Logger()
legacy_logger = LegacyLogger()

# There should be only one radeon_gpu_detective-<version> folder in the testkit, but glob returns a list.
rgd_cli_folder = glob.glob(os.path.join(script_folder,"radeon_gpu_detective-*"))
crash_generator_folder = glob.glob(os.path.join(script_folder,"RGDCaptureTests*"))

# RGD CLI Default path
if len(rgd_cli_folder) == 0:
    logger.error("Unable to locate the default RGD package")
    exit(1)
else:
    # Set default path for rgd.exe
    rgd_cli_exe = "rgd"
    if platform.system() == "Windows":
        rgd_cli_exe = "rgd.exe"
    STR_DEFAULT_RGD_CLI_PATH = os.path.join(rgd_cli_folder[0], rgd_cli_exe)

    # Set default path for rgd_test.exe
    rgd_test_exe = "rgd_test"
    if platform.system() == "Windows":
        rgd_test_exe = "rgd_test.exe"
    STR_DEFAULT_RGD_TEST_PATH = os.path.join(rgd_cli_folder[0], rgd_test_exe)    

# Crash Generator App Default path
if len(crash_generator_folder) == 0:
    logger.error("Unable to locate the default Crash Generator package")
    exit(1)
else:
    crash_generator_exe = "RGDIntegrationTests"
    if platform.system() == "Windows":
        crash_generator_exe = "RGDIntegrationTests.exe"
    STR_DEFAULT_CRASH_GENERATOR_PATH = os.path.join(crash_generator_folder[0], crash_generator_exe)

# Config for the test environment
class TestConfig:
    def __init__(self):
        self.test_api = []
        self.retain_output_files = True
        self.crash_generator_exe_path = None
        self.gpu_trasher_path = None
        self.rgd_test_exe_path = None
        self.rgd_cli_exe_path = None
        self.verbose_console_output = False
        self.test_descriptors = []
        self.out_dir = os.path.join(script_folder,"Output-" + time_stamp)
        self.test_output_files_dir = os.path.join(self.out_dir, "RGDFiles")
    
    def get_gpu_trasher_path(self):

        ret_path = None
        if self.gpu_trasher_path is not None:
            ret_path = self.gpu_trasher_path
        else:
            ret_path = self.set_gpu_trasher_path()
        
        return ret_path
            

    def set_gpu_trasher_path(self):
        crash_generator_folder = Path(self.crash_generator_exe_path).parent
        sample_apps_folder = glob.glob(os.path.join(crash_generator_folder,"sample_apps"))
        _STR_EXITING_SUFFIX = "Exiting..."
        if len(sample_apps_folder) == 0:
            _STR_UNABLE_TO_LOCATE_SAMPLE_APPS_FOLDER_MSG = "Unable to locate the sample apps folder"
            logger.error(f"{_STR_UNABLE_TO_LOCATE_SAMPLE_APPS_FOLDER_MSG} inside {crash_generator_folder}.")
            legacy_logger.summary_info(f"{_STR_UNABLE_TO_LOCATE_SAMPLE_APPS_FOLDER_MSG} inside {crash_generator_folder}. {_STR_EXITING_SUFFIX}")
            exit(1)
        else:
            # DX12 - GPUTrasher path
            gpu_trasher_folder = glob.glob(os.path.join(sample_apps_folder[0],"GpuTrasher"))
            _STR_UNABLE_TO_LOCATE_GPUTRASHER_FOLDER_MSG = "Unable to locate the GPUTrasher folder"
            if len(gpu_trasher_folder) == 0:
                logger.error(f"{_STR_UNABLE_TO_LOCATE_GPUTRASHER_FOLDER_MSG} inside {sample_apps_folder[0]}.")
                legacy_logger.summary_info(f"{_STR_UNABLE_TO_LOCATE_GPUTRASHER_FOLDER_MSG} inside {sample_apps_folder[0]}. {_STR_EXITING_SUFFIX}")
                exit(1)
            else:
                self.gpu_trasher_path = gpu_trasher_folder[0]
        
        return self.gpu_trasher_path
                
    def set_test_config(self, args):
        
        self.retain_output_files = args.retain
        self.verbose_console_output = args.verbose
        self.crash_generator_exe_path = args.crash_generator                                                                     
        self.rgd_test_exe_path = args.rgd_test
        self.rgd_cli_exe_path = args.rgd_cli

        if args.test:
            self.test_descriptors = args.test
        else:
            self.test_descriptors.append(STR_DEFAULT_TEST_DESC_FILE)

        if args.api:
            for api in args.api:
                if api in SUPPORTED_APIS:
                    self.test_api.append(api)
        else:
            self.test_api.append(SUPPORTED_APIS[0])

        if not os.path.exists(self.out_dir):
            os.mkdir(self.out_dir)

        if not os.path.exists(self.test_output_files_dir):
            os.mkdir(self.test_output_files_dir)
        
        self.set_gpu_trasher_path()
        legacy_logger.set_dir_extended_log(self.test_output_files_dir)

class TestCrashCase:
    def __init__(self, test_api, test_name, case_no, verify_crash_dump=True, verify_rgd_output=False) -> None:
        self.test_api = test_api
        self.case_no = case_no
        self.test_name = test_name
        self.verify_crash_dump = verify_crash_dump
        self.verify_rgd_output = verify_rgd_output
        self.generated_rgd_txt_output_file_path = None
        self.generated_rgd_json_output_file_path = None
        self.generated_rgd_crash_dump_file_path = None
        self.is_page_fault_case = False
        self.is_app_crashed = True
        self.is_capture_test_passed = True
        self.is_backend_tests_passed = True
        self.is_crash_dump_generated = False
        self.capture_test_duration = timedelta(0)
        self.backend_test_duration = timedelta(0)
        self.crashing_app_path = ""
        self.capture_output_console = ""
        self.backend_test_output_console = ""
        self.rgd_cli_output_console = ""
        self.capture_output_error = ""
        self.backend_test_output_error = ""
        self.rgd_cli_output_error = ""
        
    def is_non_empty_file(self, file_path)->bool:
        return Path(file_path).is_file() and Path(file_path).stat().st_size > 0
    
    def execute_command(self, command):
        # Execute a command and returns its output
        working_dir = Path(command[0]).resolve().parent
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd = working_dir)
        return result.stdout.decode('utf-8'), result.stderr.decode('utf-8'), result.returncode
    
    def get_gtest_filter(self) -> str:
        filter = "--gtest_filter=*Case" + str(self.case_no)
        filter = filter.replace('.','_')
        return filter
    
    def move_crash_generator_log(self, test_output_files_dir):
        is_app_crashed = False
        crash_generator_log_file_path =glob.glob(os.path.join(self.crashing_app_path,"GpuTrasher_DX12*.txt"))
        case_string = "case" + str(self.case_no)

        #Append 'log' to the GPUTrasher log file name.
        for log_file in crash_generator_log_file_path:
            file_strings = log_file.split('\\')
            log_file_name = file_strings[-1]
            is_app_crashed = True if case_string in log_file_name else False
            directory, file_name = os.path.split(log_file)
            name, extension = os.path.splitext(file_name)
            new_filename = name + '-log' + extension
            new_file_path = os.path.join(directory, new_filename)
            os.rename(log_file, new_file_path)

        crash_generator_log_file_path = []
        crash_generator_log_file_path =glob.glob(os.path.join(self.crashing_app_path,"GpuTrasher_DX12*.txt"))
        # Though glob returns a list of files, it is expected to include only one crash generator log file.
        for log_file in crash_generator_log_file_path:
            is_move_successful = move_folder(source_path=log_file, destination_path=test_output_files_dir)
            if not is_move_successful:
                legacy_logger.summary_info(f"{STR_ERROR} Unable to move crash generator log file {log_file} to {self.test_config.test_output_files_dir}")
        
        return is_app_crashed
    
    def move_generated_crash_dump(self, crash_generator_dir, test_output_files_dir):
        is_crash_dump = False
        crash_dump_folder_list = glob.glob(os.path.join(crash_generator_dir,"rgd-dumps-run*"))

        if crash_dump_folder_list:

            # Only one rgd-dumps-run* folder is expected to be preset in the directory. In case there are more than one, latest folder is picked.
            generated_dump_files_path = glob.glob(os.path.join(crash_dump_folder_list[-1],"*.rgd"))

            if generated_dump_files_path:

                # Only one crash dump file should be generated.
                for file in generated_dump_files_path:
                    file_strings = file.split('\\')
                    rgd_file_name = file_strings[-1]
                    case_string = "case" + str(self.case_no)
                    if case_string in rgd_file_name:
                        move_folder(file, test_output_files_dir)
                        rgd_dump_files_list = glob.glob(os.path.join(test_output_files_dir, rgd_file_name))
                        if len(rgd_dump_files_list) != 1:
                            
                            # There should be only one matching .rgd file.
                            legacy_logger.summary_info(f"{STR_ERROR} Invalid crash dump file count for file {rgd_file_name}. Count: {len(rgd_dump_files_list)}")
                            sys.exit(1)
                        else:
                            self.generated_rgd_crash_dump_file_path = rgd_dump_files_list[-1]
                            is_crash_dump = True
            # Delete rgd-dumps-run* folders as crash dumps are moved to the 'test_output_files_dir'.
            for dir in crash_dump_folder_list:
                shutil.rmtree(dir)
            
        return is_crash_dump

    def launch_crash_generator_exe(self, crash_generator_exe_path, test_output_files_dir):
        
        # Run Crash Generator App
        start_time = datetime.now()
        logger.test_info("")
        command_list = [crash_generator_exe_path, self.get_gtest_filter()]
        
        # Log legacy output.
        command_str = ' '.join([str for str in command_list])
        legacy_logger.capture_info("[TestRunner] Start: " + command_str)
        legacy_logger.summary_info(command_str)
        
        self.capture_output_console, self.capture_output_error, return_code = self.execute_command(command_list)
        error_output_lines = self.capture_output_error.splitlines()

        # If app is crashed succesfully, a log file is generated under crashing app folder.
        self.is_app_crashed = self.move_crash_generator_log(test_output_files_dir)

        # Crash dump is generated under crash_generator_dir/rgd-dumps-run-timestamp dir
        crash_generator_dir = Path(crash_generator_exe_path).resolve().parent
        self.is_crash_dump_generated = self.move_generated_crash_dump(crash_generator_dir, test_output_files_dir)
        
        _STR_CAPTURE_TEST = "Capture"
        _STR_CAPTURE_TEST_FAILED_MSG = f"Capture test failed for case {str(self.case_no)}"

        # Some cases are expected not to crash on certain windows OS + GPU config. Then return code will be 0 but no crash dump is generated.
        # Sometimes the crashing app is not launched on time and return code is 1 after the timeout but app crashes.
        # GPUTrasher can be considered to be crashed successfully when a GPUTrasher log file is generated.
        # Capture test can be considered a success when a crash dump is generated.
        if return_code == 0 or self.is_crash_dump_generated:
            legacy_logger.summary_info(f"{_STR_CAPTURE_TEST}:{STR_RGD_TEST_SUFFIX_PASSED}")
            self.is_capture_test_passed = True
            if self.is_crash_dump_generated:
                logger.test_msg(f"Crash dump file generated successfully for test \"{self.test_name}\".")
            
        elif self.is_app_crashed:
            # GPUTrasher crashed but no crash dump file generated - capture test failure.
            self.is_capture_test_passed = False
            legacy_logger.summary_info(f"{_STR_CAPTURE_TEST}:{STR_RGD_TEST_SUFFIX_FAILED} (Return code: {str(return_code)})")
            legacy_logger.capture_info(f"{_STR_CAPTURE_TEST_FAILED_MSG} - GPUTrasher crashed but no crash dump generated.")
            if self.capture_output_error:
                for line in error_output_lines:
                    if STR_ERROR in line:
                        legacy_logger.capture_info(line)
            logger.test_fail(f"Crash dump file not generated for test \"{self.test_name}\".")
        
        else:
            # GPUTrasher could not be launched or it was launched but did not crash when it was expected to crash.
            self.is_capture_test_passed = False
            _STR_GPUTRASHER_DID_NOT_CRASH_MSG = "GPUTrasher could not be launched or it was launched but did not crash as expected"
            legacy_logger.summary_info(f"{_STR_CAPTURE_TEST}:{STR_RGD_TEST_SUFFIX_FAILED} (Return code: {str(return_code)})")
            legacy_logger.capture_info(f"{_STR_CAPTURE_TEST_FAILED_MSG}.")
            if self.capture_output_error:
                for line in error_output_lines:
                    if STR_ERROR in line:
                        legacy_logger.capture_info(line)

        end_time = datetime.now()
        self.capture_test_duration = end_time - start_time
    
    def launch_rgd_test_exe(self, rgd_test_exe_path):
        # Run Rgd Test
        logger.test_info("")
        logger.test_info("Running RGD Tests...")

        command_list = [rgd_test_exe_path, '--rgd', self.generated_rgd_crash_dump_file_path]
        if self.is_page_fault_case:
            command_list.append('--page-fault')
        # Log legacy output.
        command_str = ' '.join([str for str in command_list])
        legacy_logger.backend_info(f"[TestRunner] Start {command_str}")
        legacy_logger.backend_info("Running RGD Tests...")

        # Run rgd_test.exe
        self.backend_test_output_console, self.backend_test_output_error, rc_failed_assertions_count = self.execute_command(command_list)
        
        legacy_logger.backend_info(f"Return code from Catch2 {rc_failed_assertions_count}")
        _STR_BACKEND_TEST = "BackendTest"
        _STR_API          = "API"
        if rc_failed_assertions_count == 0:
            legacy_logger.summary_info(f"{_STR_BACKEND_TEST}: {_STR_API}: {self.test_api}    {command_str}{STR_RGD_TEST_SUFFIX_PASSED}")
            legacy_logger.backend_info("All tests passed.")
            self.is_backend_tests_passed = True
        else:
            legacy_logger.summary_info(f"{_STR_BACKEND_TEST}: {_STR_API}: {self.test_api}    {command_str}{STR_RGD_TEST_SUFFIX_FAILED}")
            legacy_logger.backend_info(f"{rc_failed_assertions_count} backend tests failed.")
            self.is_backend_tests_passed = False

        self.post_process_rgd_test_output()
    
    def post_process_rgd_test_output(self):
        output_lines = self.backend_test_output_error.splitlines()
        is_include_line = False

        for line in output_lines:
            if STR_ERROR in line:
                legacy_logger.backend_info(line)

        output_lines = self.backend_test_output_console.splitlines()
        for line in output_lines:
            
            if '----' in line:
                is_include_line = True
            
            if is_include_line:
                logger.test_info(line)
    
    def launch_rgd_cli_exe(self, rgd_cli_exe_path, test_output_files_dir):
        # Generate Rgd output for crash dump files
        logger.test_info("")
        logger.test_info("Running RGD CLI...")

        file_strings = self.generated_rgd_crash_dump_file_path.split('\\')
        rgd_file_name = file_strings[-1]
        logger.test_info("Input crash dump file: " + rgd_file_name)
        rgd_output_file_name_txt = os.path.splitext(rgd_file_name)[0] + "-summary"
        rgd_output_file_name_json = os.path.splitext(rgd_file_name)[0] + "-summary"
        rgd_output_file_name_txt = generate_file_name(test_output_files_dir, rgd_output_file_name_txt, "txt")
        rgd_output_file_name_json = generate_file_name(test_output_files_dir, rgd_output_file_name_json, "json")
        self.rgd_cli_output_console, self.rgd_cli_output_error, return_code = self.execute_command([rgd_cli_exe_path, '-j', rgd_output_file_name_json, '-o', rgd_output_file_name_txt, '-p', self.generated_rgd_crash_dump_file_path])

        # Check if both .txt and .json files are generated and log the results.
        is_text_file_generated = self.is_non_empty_file(rgd_output_file_name_txt)
        is_json_file_generated = self.is_non_empty_file(rgd_output_file_name_json)

        if is_text_file_generated and is_json_file_generated:
            logger.test_msg(f"RGD text and JSON output is generated for test \"{self.test_name}\".")
        elif is_text_file_generated:
            logger.test_msg(f"RGD text output is generated for test \"{self.test_name}\".")
            logger.test_fail(f"RGD JSON output is not generated for test \"{self.test_name}\".")
        elif is_json_file_generated:
            logger.test_msg(f"RGD JSON output is generated for test \"{self.test_name}\".")
            logger.test_fail(f"RGD text output is not generated for test \"{self.test_name}\".")
        else:
            logger.test_fail(f"RGD text and JSON output is not generated for test \"{self.test_name}\".")

        self.post_process_rgd_cli_output()
    
    def post_process_rgd_cli_output(self):
        output_lines = self.rgd_cli_output_console.splitlines()
        logger.test_info("Console output from the RGD CLI:")
        for line in output_lines:
            logger.test_info(line)
        logger.test_info("")

    def log_test_results(self):
        if self.is_capture_test_passed and self.is_backend_tests_passed:
            logger.test_pass(f"Crash test passed for case no [{self.case_no:02}] - \"{self.test_name}\"")
        else:
            logger.test_fail(f"Crash test failed for case no [{self.case_no:02}] - \"{self.test_name}\"")

            # Create extended log for failed test case
            if self.capture_output_console: 
                legacy_logger.log_failure_info(self.case_no, "Capture test console output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.capture_output_console)
                legacy_logger.log_failure_info(self.case_no, "")
            if self.capture_output_error: 
                legacy_logger.log_failure_info(self.case_no, "Capture test error output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.capture_output_error)
                legacy_logger.log_failure_info(self.case_no, "") 
            if self.backend_test_output_console: 
                legacy_logger.log_failure_info(self.case_no, "Backend test console output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.backend_test_output_console)
                legacy_logger.log_failure_info(self.case_no, "")
            if self.backend_test_output_error: 
                legacy_logger.log_failure_info(self.case_no, "Backend test error output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.backend_test_output_error)
                legacy_logger.log_failure_info(self.case_no, "")
            if self.rgd_cli_output_console: 
                legacy_logger.log_failure_info(self.case_no, "RGD CLI console output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.rgd_cli_output_console)
                legacy_logger.log_failure_info(self.case_no, "")
            if self.rgd_cli_output_error: 
                legacy_logger.log_failure_info(self.case_no, "RGD CLI error output:")
                legacy_logger.log_failure_info(self.case_no, "============================")
                legacy_logger.log_failure_info(self.case_no, self.rgd_cli_output_error)
                legacy_logger.log_failure_info(self.case_no, "")
            
        

##
## Test Driver.
##
class TestDriver:
    # Constructor.
    def __init__(self, test_config):
        self.test_config = test_config
        self.count_capture_tests_passed = 0
        self.count_capture_tests_failed = 0
        self.count_backend_tests_passed = 0
        self.count_backend_tests_failed = 0
        self.total_capture_test_duration = 0
        self.total_backend_test_duration = 0
        self.test_start_time = 0
        self.test_end_time = 0
        self.crash_cases = []
    
    def calculate_total_capture_test_duration(self):
        
        self.total_capture_test_duration = timedelta(0)
        for test in self.crash_cases:
            self.total_capture_test_duration += test.capture_test_duration
        
        return self.total_capture_test_duration

    def calculate_total_backend_test_duration(self):
        
        self.total_backend_test_duration = timedelta(0)
        for test in self.crash_cases:
            self.total_backend_test_duration += test.backend_test_duration
        
        return self.total_backend_test_duration

    def run_tests(self):

        is_test_failed = False
        # Process all test descriptor files provided.
        for file in self.test_config.test_descriptors:
            logger.test_msg(f"Processing test descriptor - {file}")
            with open(file, 'r') as file:
                api_tests_set = json.load(file) 
            
            # Run tests for all APIs provided in this test descriptor.
            for api, test_set in api_tests_set.items():
                logger.test_msg(f"Running tests for API - {api}")
                if api not in SUPPORTED_APIS:
                    logger.critical(f"{api} is not supported. Supported APIs - {get_supported_api_str()}")
                else:
                    # For each test described in the test descriptor file, run tests as per the provided test config.
                    for test in test_set:
                        if not test.get('verify_crash_dump', False):
                            test_name = test.get('test_name', "N/A")
                            logger.warning(f"Test \"{test_name}\" is ignored as \"verify_crash_dump\" is not set in the test descriptor.")
                        else:
                            if not test.get('crash_test_case', False):
                                logger.warning(f"Invalid/No case no provided for test \"{test_name}\".")
                            else:
                                test_crash_case = TestCrashCase(api,
                                                                test.get('test_name', "NULL"),
                                                                test['crash_test_case'],
                                                                verify_crash_dump=test.get('verify_crash_dump', False),
                                                                verify_rgd_output=test.get('verify_rgd_output', False))

                                test_crash_case.crashing_app_path = self.test_config.get_gpu_trasher_path()
                                if test.get('page_fault_case', False):
                                    test_crash_case.is_page_fault_case = True

                                self.crash_cases.append(test_crash_case)
                                
                                test_crash_case.launch_crash_generator_exe(self.test_config.crash_generator_exe_path,
                                                                                self.test_config.test_output_files_dir)
                                
                                if test_crash_case.is_capture_test_passed:
                                    if test_crash_case.is_crash_dump_generated:
                                        backend_tests_start_time = datetime.now()
                                        # Run RGD tests on captured .rgd file and update pass fail tests count.
                                        test_crash_case.launch_rgd_test_exe(self.test_config.rgd_test_exe_path)

                                        # Generate rgd output.
                                        test_crash_case.launch_rgd_cli_exe(self.test_config.rgd_cli_exe_path,
                                                                            self.test_config.test_output_files_dir)
                                        
                                        backend_test_end_time = datetime.now()
                                        test_crash_case.backend_test_duration = backend_test_end_time - backend_tests_start_time
                                    else:
                                        # Some cases are expected not to crash on certain system configs.
                                        # For the capture test is marked successful but crash dump file is not generated.
                                        log_msg = f"BackendTest: Skipped. For case #{test_crash_case.case_no}, crash dump is not generated which is expected on the current system config."
                                        legacy_logger.summary_info(log_msg)
                                        legacy_logger.backend_info(log_msg)
                                
                                test_crash_case.log_test_results()

            is_test_failed = self.log_final_test_status()

            # If atleast one capture test passed, close the TDR or Bug Report popup.
            if self.count_capture_tests_passed > 0:
                close_bug_report_popup_window()

            return is_test_failed


    def log_final_test_status(self):

        is_test_failed = False
        log_file_path = Path(logger.log_file)
        log_file_name = log_file_path.parent.name + '\\' + log_file_path.name

        total_tests = len(self.crash_cases)
        for test_crash_case in self.crash_cases:
            
            # Count total capture tests.
            if test_crash_case.is_capture_test_passed:
                self.count_capture_tests_passed += 1
                
                # Count total backend tests.
                if test_crash_case.is_backend_tests_passed:
                    self.count_backend_tests_passed += 1
                else:
                    self.count_backend_tests_failed += 1
            else:
                self.count_capture_tests_failed += 1

        spaces = logger.get_longest_level_name_length()
        results_summary = "Final test summary:\n"
        results_summary += (' ' * spaces) + ': '
        results_summary += "==================\n"
        results_summary += (' ' * spaces) + ': '
        results_summary += "PASSED: " + str(self.count_capture_tests_passed) + "/" + str(self.count_capture_tests_passed + self.count_capture_tests_failed) + "\n"
        results_summary += (' ' * spaces) + ': '
        results_summary += "FAILED: " + str(self.count_capture_tests_failed) + "/" + str(self.count_capture_tests_passed + self.count_capture_tests_failed) + "\n"
        results_summary += (' ' * spaces) + ': '
        results_summary += "==================\n"
        results_summary += (' ' * spaces) + ': \n'
        results_summary += (' ' * spaces) + ': ' + 'RGD Test log file: ' + log_file_name
        logger.test_msg("")
        logger.test_result(results_summary)
        logger.test_msg("")

        # Log standard output.
        legacy_logger.summary_info("=============")
        legacy_logger.summary_info("Test Results:")

        _STR_CAPTURE_TEST_RUNS = "Capture Test runs"
        _STR_BACKEND_TEST_RUNS = "Backend Test runs"
        _STR_OUT_OF            = "out of"
        _STR_TESTS_DURANTION    = "Tests Duration"

        if self.count_capture_tests_failed > 0:
            legacy_logger.summary_info(f"{_STR_CAPTURE_TEST_RUNS}: {self.count_capture_tests_passed} {_STR_OUT_OF} {total_tests}{STR_RGD_TEST_SUFFIX_FAILED}")
        else:
            legacy_logger.summary_info(f"{_STR_CAPTURE_TEST_RUNS}: {self.count_capture_tests_passed} {_STR_OUT_OF} {total_tests}{STR_RGD_TEST_SUFFIX_PASSED}")

        if self.count_backend_tests_failed > 0:
            legacy_logger.summary_info(f"{_STR_BACKEND_TEST_RUNS}: {self.count_backend_tests_passed} {_STR_OUT_OF} {self.count_backend_tests_passed + self.count_backend_tests_failed}{STR_RGD_TEST_SUFFIX_FAILED}")
        else:
            legacy_logger.summary_info(f"{_STR_BACKEND_TEST_RUNS}: {self.count_backend_tests_passed} {_STR_OUT_OF} {self.count_backend_tests_passed + self.count_backend_tests_failed}{STR_RGD_TEST_SUFFIX_PASSED}")
        
        # Log test duration.
        self.test_end_time = datetime.now()
        legacy_logger.summary_info(f"Total {_STR_TESTS_DURANTION}  : {str((self.test_end_time - self.test_start_time).total_seconds())} secs")
        legacy_logger.summary_info(f"Capture {_STR_TESTS_DURANTION}: {str((self.calculate_total_capture_test_duration()).total_seconds())} secs")
        legacy_logger.summary_info(f"Backend {_STR_TESTS_DURANTION}: {str((self.calculate_total_backend_test_duration()).total_seconds())} secs")

        # Log output file path.
        capture_file_name = Path(legacy_logger.capture_test_log_file).parent.name + '\\' + Path(legacy_logger.capture_test_log_file).name
        backend_file_name = Path(legacy_logger.backend_test_log_file).parent.name + '\\' + Path(legacy_logger.backend_test_log_file).name
        summary_file_name = Path(legacy_logger.summary_log_file).parent.name + '\\' + Path(legacy_logger.summary_log_file).name
        legacy_logger.summary_info(f"Capture Test Log    : {capture_file_name}")
        legacy_logger.summary_info(f"RGD Backend Test Log: {backend_file_name}")
        legacy_logger.summary_info(f"Test Run Summary    : {summary_file_name}")

        #Retain output files if any of the tests failed
        if self.count_capture_tests_failed != 0 or self.count_backend_tests_failed != 0:
            is_test_failed = True
            logger.test_msg(f"Test output files are retained! Path: {self.test_config.test_output_files_dir}")
        elif not self.test_config.retain_output_files:
            shutil.rmtree(self.test_config.test_output_files_dir)

        return is_test_failed


def generate_file_name(folder, base_filename, extension):
    timestamp = get_current_timestamp()
    new_filename = f"{base_filename}.{extension}"
    return os.path.join(folder, new_filename)

def move_folder(source_path, destination_path):
    ret = True
    if os.path.exists(source_path) and os.path.exists(destination_path):
        try:
            shutil.move(source_path, destination_path)
        except Exception as e:
            ret = False
            logger.error(f"Unable to move generated test files: {e}")
    else:
        ret = False
    
    return ret

def main(args):

    # Set test configuration
    test_config = TestConfig()
    test_config.set_test_config(args)

    # Initialize Test Driver
    test_driver = TestDriver(test_config)
    test_driver.test_start_time = datetime.now()

    if(args.modern_output):
        # Set log file directory to test output directory
        logger.set_file_handler(test_driver.test_config.out_dir)
        logger.set_console_verbosity(test_config.verbose_console_output)
        legacy_logger.disable_legacy_logger()
    else:
        # Use legacy logger
        # Set log file directory to test output directory
        legacy_logger.set_file_handler(test_driver.test_config.out_dir)
        
        # write version in file if file not there, no version info is printed.
        test_kit_version_info = get_test_kit_version_string()
        if test_kit_version_info:
            legacy_logger.summary_info(f"Test Kit Version: {str(test_kit_version_info)}")

        legacy_logger.summary_info("Open Capture Test Log: " + legacy_logger.capture_test_log_file)
        logger.disable_modern_logger()

    return test_driver.run_tests()

def PythonVersionValid():
    # convert version string to an it and do a single comparison
    version = (((sys.version_info[0] * 1000) + sys.version_info[1]) * 1000) + sys.version_info[2]

    # minimum version is 3.6.05
    if (version >= 3006005):
        return True
    return False

if __name__ == "__main__":
    
    # test for valid python version. Runs on 2.7.11 or above
    if (PythonVersionValid() == False):
        sys.exit("This script requires python 2.7.11 or newer. Exiting.")
    
    parser = argparse.ArgumentParser(description="RGD Automated tests framework launchscript.")
    parser.add_argument(STR_SCRIPT_OPT_TEST_DESC_FILE, action='append',
                        help='Path to a test description file. If not specified, the \'RgdDriverSanity.json\' from the input_description_files folder will be used.')
    parser.add_argument(STR_SCRIPT_OPT_CRASH_GENERATOR_PATH, type=str, default=STR_DEFAULT_CRASH_GENERATOR_PATH, help="Path to the RDP RGD Integration Test CLI.")
    parser.add_argument(STR_SCRIPT_OPT_RGD_TEST_PATH, type=str, default=STR_DEFAULT_RGD_TEST_PATH, help="Path to the RGD Test CLI.")
    parser.add_argument(STR_SCRIPT_OPT_RGD_PATH, type=str, default=STR_DEFAULT_RGD_CLI_PATH, help="Path to the RGD CLI.")
    parser.add_argument(STR_SCRIPT_OPT_RETAIN_OUTPUT, action='store_true', default=True, help="If set to 'False', when all tests pass, artifacts created for each test run are deleted after successful logging of the results. By default, it is set to 'True' and test artifacts created for all the test runs are retained.")
    parser.add_argument(STR_SCRIPT_OPT_TEST_API, action='append', help='Currently only DX12 is supported')
    parser.add_argument(STR_SCRIPT_OPT_VERBOSE, action='store_true', default=False, help='If specified, console output will include more verbose level information for the tests.')
    parser.add_argument(STR_SCRIPT_OPT_MODERN_OUTPUT, action='store_true', default=False, help='If specified, modernized logging format will be used instead of legacy output format for both file and console outputs.')
    
    args = parser.parse_args()
    is_test_failed = main(args)

    if is_test_failed:
        
        # Tell the OS that the test failed by setting the exit code to non-zero. 
        sys.exit(1)

    