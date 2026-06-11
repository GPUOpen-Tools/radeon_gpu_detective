# Copyright © Advanced Micro Devices, Inc. All rights reserved.
#
#!/usr/bin/python

"""
RGD JSON Test Module

This module provides functionality for testing RGD's JSON parsing and validation capabilities.
It includes JSON comparison utilities, test execution, and logging specifically for parse-to-json tests.

Main Components:
- ParseToJsonLogger: Dedicated logging for JSON tests
- JsonComparator: JSON comparison and validation utilities
- RGDJsonTestSuite: Test suite for RGD JSON functionality
"""

import os
import json
import copy
import subprocess
import logging
from pathlib import Path
from datetime import datetime


# Logging levels (matching TestRunner.py)
DEBUG = 10
TEST_INFO = 15
INFO = 20
TEST_MSG = 22
TEST_PASS = 32
WARNING = 30
TEST_FAIL = 37
ERROR = 40
TEST_RESULT = 45
CRITICAL = 100

# Add custom level names
logging.addLevelName(TEST_INFO, "")
logging.addLevelName(TEST_MSG, "")
logging.addLevelName(TEST_PASS, "PASS")
logging.addLevelName(TEST_FAIL, "*FAIL*")
logging.addLevelName(TEST_RESULT, "RESULT")

def get_current_timestamp():
    """Generate timestamp for unique file names"""
    return datetime.now().strftime('%Y%m%d_%H%M%S')


class JsonComparator:
    """Utility class for comparing JSON files and data structures"""
    
    def __init__(self, logger):
        self.logger = logger
    
    def compare_json_with_golden(self, generated_json, golden_json, allowed_differences=None):
        """
        Compare generated JSON with golden reference JSON.
        Allows specified fields in allowed_differences to be different.
        Returns True if JSONs match (except for allowed differences), False otherwise.
        """
        # Default allowed differences if none provided
        if allowed_differences is None:
            allowed_differences = ["crash_analysis_file.input_crash_dump_file_name"]
        
        # Create deep copies to avoid modifying original data
        gen_copy = copy.deepcopy(generated_json)
        gold_copy = copy.deepcopy(golden_json)
        
        # Normalize all allowed different fields in both copies
        for field_path in allowed_differences:
            self._normalize_field(gen_copy, field_path)
            self._normalize_field(gold_copy, field_path)
        
        # Find all differences
        differences = self._find_differences(gen_copy, gold_copy)
        
        if differences:
            self.logger.test_fail("Generated JSON differs from golden reference:")
            self.logger.test_msg(f"Total differences found: {len(differences)}")
            self.logger.test_msg("First 10 differences:")
            for i, diff in enumerate(differences[:10], 1):  # Show first 10 differences
                self.logger.test_msg(f"  {i}. {diff}")
            if len(differences) > 10:
                self.logger.test_msg(f"  ... and {len(differences) - 10} more differences not shown")
            return False
        else:
            allowed_fields_str = ", ".join(allowed_differences)
            self.logger.test_pass(f"Generated JSON matches golden reference (ignoring: {allowed_fields_str})")
            return True
    
    def _normalize_field(self, obj, field_path, normalized_value="NORMALIZED_VALUE"):
        """Set a field at the given path to a normalized value"""
        keys = field_path.split('.')
        current = obj
        
        # Navigate to the parent object
        for key in keys[:-1]:
            if isinstance(current, dict) and key in current:
                current = current[key]
            else:
                return  # Path doesn't exist, nothing to normalize
        
        # Set the final key to normalized value if it exists
        final_key = keys[-1]
        if isinstance(current, dict) and final_key in current:
            current[final_key] = normalized_value
    
    def _find_differences(self, obj1, obj2, path=""):
        """Recursively find differences between two JSON objects"""
        differences = []
        
        if type(obj1) != type(obj2):
            differences.append(f"Type mismatch at {path}: {type(obj1).__name__} vs {type(obj2).__name__}")
            return differences
            
        if isinstance(obj1, dict):
            all_keys = set(obj1.keys()) | set(obj2.keys())
            for key in all_keys:
                current_path = f"{path}.{key}" if path else key
                if key not in obj1:
                    differences.append(f"Missing key in generated: {current_path}")
                elif key not in obj2:
                    differences.append(f"Extra key in generated: {current_path}")
                else:
                    differences.extend(self._find_differences(obj1[key], obj2[key], current_path))
        elif isinstance(obj1, list):
            if len(obj1) != len(obj2):
                differences.append(f"Array length mismatch at {path}: {len(obj1)} vs {len(obj2)}")
            else:
                for i, (item1, item2) in enumerate(zip(obj1, obj2)):
                    differences.extend(self._find_differences(item1, item2, f"{path}[{i}]"))
        else:
            if obj1 != obj2:
                differences.append(f"Value mismatch at {path}: '{obj1}' vs '{obj2}'")
        
        return differences


class TestDescriptorLoader:
    """Utility class for loading test descriptor files"""
    
    def __init__(self, script_folder):
        self.script_folder = script_folder
    
    def load_parse_test_descriptor(self):
        """Load and return the parse-to-json test descriptor file"""
        input_desc_path = os.path.join(self.script_folder, 'input_description_files', 'RgdParseToJsonTest.json')
        with open(input_desc_path, 'r') as f:
            return json.load(f)
    
    def load_compare_test_descriptor(self):
        """Load and return the compare JSON test descriptor file"""
        input_desc_path = os.path.join(self.script_folder, 'input_description_files', 'RgdCompareJsonTest.json')
        with open(input_desc_path, 'r') as f:
            return json.load(f)


class RGDJsonTestSuite:
    """
    RGD Parse-to-JSON Test Suite
    Encapsulates all parse-to-json testing functionality following SOLID principles
    """
    
    def __init__(self, rgd_cli_path, output_dir, script_folder, logger=None, legacy_logger=None):
        # Validate RGD CLI path
        if not os.path.exists(rgd_cli_path):
            raise FileNotFoundError(f"RGD CLI not found at: {rgd_cli_path}")
        
        self.script_folder = script_folder
        self.descriptor_loader = TestDescriptorLoader(script_folder)
        
        # Use provided loggers or create simple fallback loggers
        self.logger = logger
        self.legacy_logger = legacy_logger
        self.json_comparator = JsonComparator(self)
        
        # Load descriptors once and cache test names
        parse_desc = self.descriptor_loader.load_parse_test_descriptor()
        compare_desc = self.descriptor_loader.load_compare_test_descriptor()
        
        self.parse_test_name = parse_desc.get('test_name', 'Parse RGD to JSON')
        self.compare_test_name = compare_desc.get('test_name', 'Compare JSON with Golden Reference')
        
        output_folder_name = parse_desc.get('output_folder', 'ParseTestFiles')
        self.parse_output_folder = os.path.join(output_dir, output_folder_name)
        self.test_results = []
        self.rgd_cli_path = rgd_cli_path
        self.output_dir = output_dir
        
        # Create parse output folder if it doesn't exist
        if not os.path.exists(self.parse_output_folder):
            os.makedirs(self.parse_output_folder)
    
    def set_console_verbosity(self, is_verbose: bool):
        """Set console verbosity for the logger"""
        if self.logger and hasattr(self.logger, 'set_console_verbosity'):
            self.logger.set_console_verbosity(is_verbose)
    
    # Logger methods for JSON tests
    def debug(self, message):
        if self.logger:
            self.logger.debug(message)
        else:
            print(f"DEBUG: {message}")
    
    def test_info(self, message):
        if self.logger:
            self.logger.test_info(message)
        else:
            print(message)
    
    def info(self, message):
        if self.logger:
            self.logger.info(message)
        else:
            print(f"INFO: {message}")
    
    def test_msg(self, message):
        if self.logger:
            self.logger.test_msg(message)
        elif self.legacy_logger:
            self.legacy_logger.json_test_info(message)
        else:
            print(message)
    
    def test_pass(self, message):
        if self.logger:
            self.logger.test_pass(message)
        elif self.legacy_logger:
            self.legacy_logger.json_test_info(f"PASS: {message}")
        else:
            print(f"PASS: {message}")
    
    def test_fail(self, message):
        if self.logger:
            self.logger.test_fail(message)
        elif self.legacy_logger:
            self.legacy_logger.json_test_info(f"*FAIL*: {message}")
        else:
            print(f"*FAIL*: {message}")
    
    def warning(self, message):
        if self.logger:
            self.logger.warning(message)
        else:
            print(f"WARNING: {message}")
    
    def error(self, message):
        if self.logger:
            self.logger.error(message)
        else:
            print(f"ERROR: {message}")
    
    def critical(self, message):
        if self.logger:
            self.logger.critical(message)
        else:
            print(f"CRITICAL: {message}")
    
    def _execute_parse_to_json(self):
        """
        Execute RGD CLI to parse crash dump to JSON format.
        Returns True if successful, False otherwise.
        """
        desc = self.descriptor_loader.load_parse_test_descriptor()
        
        # Convert relative paths to absolute paths
        input_file = desc['input_file']
        if not os.path.isabs(input_file):
            input_file = os.path.join(self.script_folder, "samples", input_file)
        input_file = os.path.normpath(input_file)
        
        # Prepare output path
        output_file = desc['output_file']
        output_path = os.path.join(self.parse_output_folder, output_file)
        
        # Build RGD CLI command
        cli_options = desc.get('cli_options', [])
        cmd = [self.rgd_cli_path, '--parse', input_file, '--json', output_path] + cli_options
        
        self.test_msg(f"Running: {' '.join(cmd)}")
        self.test_msg(f"Input file: {input_file}")
        self.test_msg(f"Output file: {output_path}")
        
        try:
            # Set working directory to RGD CLI executable's directory
            rgd_workdir = os.path.dirname(os.path.abspath(self.rgd_cli_path))
            self.test_msg(f"Working directory: {rgd_workdir}")
            
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=rgd_workdir)
            
            if result.returncode != 0:
                self.error(f"RGD CLI failed with return code {result.returncode}")
                self.error(f"STDERR: {result.stderr}")
                self.error(f"STDOUT: {result.stdout}")
                return False
            
            self.test_msg(f"RGD CLI output: {result.stdout}")
            
            # Verify output file was created
            if not os.path.exists(output_path):
                self.error(f"Output file was not created: {output_path}")
                return False
            
            # Validate JSON format
            with open(output_path, 'r') as jf:
                json.load(jf)
            
            self.test_pass("JSON output format is valid.")
            return True
            
        except json.JSONDecodeError as e:
            self.test_fail(f"JSON format error: {e}")
            return False
        except Exception as e:
            self.error(f"Exception running parse-to-json test: {e}")
            return False
    
    def _execute_compare_json(self):
        """
        Compare generated JSON file with golden reference JSON.
        Returns True if JSONs match (within allowed differences), False otherwise.
        """
        desc = self.descriptor_loader.load_compare_test_descriptor()
        
        # Get the expected generated JSON file name from the descriptor
        generated_json_filename = desc.get('generated_json_file', 'sample_crash_dump.json')
        generated_json_file = os.path.join(self.parse_output_folder, generated_json_filename)
        generated_json_file = os.path.normpath(generated_json_file)
        
        # Get golden reference file path
        golden_ref_file = desc['golden_reference_file']
        if not os.path.isabs(golden_ref_file):
            golden_ref_file = os.path.join(self.script_folder, "samples", golden_ref_file)
        golden_ref_file = os.path.normpath(golden_ref_file)
        
        self.test_msg(f"Comparing generated JSON: {generated_json_file}")
        self.test_msg(f"With golden reference: {golden_ref_file}")
        
        # Verify both files exist
        if not os.path.exists(generated_json_file):
            self.error(f"Generated JSON file not found: {generated_json_file}")
            return False
        
        if not os.path.exists(golden_ref_file):
            self.error(f"Golden reference file not found: {golden_ref_file}")
            return False
        
        try:
            # Load both JSON files
            with open(generated_json_file, 'r') as gf:
                generated_json = json.load(gf)
            
            with open(golden_ref_file, 'r') as rf:
                golden_json = json.load(rf)
            
            # Get allowed differences from config
            allowed_differences = desc.get('allowed_differences', ["crash_analysis_file.input_crash_dump_file_name"])
            
            # Compare JSONs using the JsonComparator
            return self.json_comparator.compare_json_with_golden(generated_json, golden_json, allowed_differences)
            
        except Exception as e:
            self.error(f"Error comparing JSON files: {e}")
            return False
    
    def run_parse_test(self):
        """Run the RGD parse-to-JSON test"""
        test_number = 1
        self.test_msg("")
        self.test_msg(f"Test #{test_number}: {self.parse_test_name}")
        self.test_msg("-" * 60)
        success = self._execute_parse_to_json()
        
        if success:
            self.test_pass(f"Test #{test_number} '{self.parse_test_name}' PASSED.")
            self.test_results.append((self.parse_test_name, True))
        else:
            self.test_fail(f"Test #{test_number} '{self.parse_test_name}' FAILED.")
            self.test_results.append((self.parse_test_name, False))
        
        return success
    
    def run_compare_test(self):
        """Run the JSON comparison test"""
        test_number = 2
        self.test_msg("")
        self.test_msg(f"Test #{test_number}: {self.compare_test_name}")
        self.test_msg("-" * 60)
        success = self._execute_compare_json()
        
        if success:
            self.test_pass(f"Test #{test_number} '{self.compare_test_name}' PASSED.")
            self.test_results.append((self.compare_test_name, True))
        else:
            self.test_fail(f"Test #{test_number} '{self.compare_test_name}' FAILED.")
            self.test_results.append((self.compare_test_name, False))
        
        return success
    
    def run_test_suite(self):
        """
        Run the complete RGD parse-to-json test suite
        Returns True if all tests pass, False otherwise
        """
        total_tests = 2
        self.test_msg("=" * 60)
        self.test_msg("Starting RGD Parse-to-JSON Test Suite")
        self.test_msg(f"Total Tests: {total_tests}")
        self.test_msg("=" * 60)
        
        try:
            # Test 1: Parse RGD to JSON
            parse_success = self.run_parse_test()
            
            if not parse_success:
                self.test_msg("")
                self.test_fail("Test #2 'Compare JSON with Golden Reference' SKIPPED (parsing failed).")
                self.test_results.append(("Compare JSON with Golden Reference", False, "Skipped due to parse failure"))
                self.log_final_results(parse_success)
                return False
            
            # Test 2: Compare with golden reference
            compare_success = self.run_compare_test()
            
            # Log final results
            overall_success = parse_success and compare_success
            self.log_final_results(overall_success)
            return overall_success
            
        except Exception as e:
            self.error(f"Unexpected error in RGD test suite: {e}")
            self.test_results.append(("Exception", False, str(e)))
            self.log_final_results(False)
            return False
    
    def log_final_results(self, overall_success):
        """Log the final test results summary"""
        self.test_msg("")
        self.test_msg("=" * 70)
        self.test_msg("RGD Parse-to-JSON Test Suite - Final Results")
        self.test_msg("=" * 70)
        
        passed_count = 0
        failed_count = 0
        
        # Header for table
        self.test_msg(f"{'#':<5} {'Test Name':<40} {'Result':<10}")
        self.test_msg("-" * 70)
        
        for i, result in enumerate(self.test_results, 1):
            test_name = result[0]
            passed = result[1]
            status = "PASSED" if passed else "FAILED"
            
            if passed:
                passed_count += 1
            else:
                failed_count += 1
            
            if len(result) > 2:
                # Skipped tests
                self.test_msg(f"{i:<5} {test_name:<40} SKIPPED")
                self.test_msg(f"      Reason: {result[2]}")
            else:
                self.test_msg(f"{i:<5} {test_name:<40} {status:<10}")
        
        total_tests = passed_count + failed_count
        self.test_msg("=" * 70)
        self.test_msg(f"Total: {total_tests} tests | Passed: {passed_count} | Failed: {failed_count}")
        self.test_msg("=" * 70)
        
        if overall_success:
            self.test_pass("[SUCCESS] All RGD parse-to-json tests completed successfully!")
        else:
            self.test_fail("[FAILED] RGD parse-to-json test suite failed!")
        
        # Log the location of the parse-to-json log file
        log_file = None
        if self.logger and hasattr(self.logger, 'json_test_log_file') and self.logger.json_test_log_file:
            log_file = self.logger.json_test_log_file
        elif self.legacy_logger and hasattr(self.legacy_logger, 'json_test_log_file') and self.legacy_logger.json_test_log_file:
            log_file = self.legacy_logger.json_test_log_file
        
        if log_file:
            log_file_path = Path(log_file)
            log_file_name = log_file_path.parent.name + '\\' + log_file_path.name
            self.test_msg(f"Detailed log: {log_file_name}")
        
        # Log the location of the parse output folder
        parse_output_path = Path(self.parse_output_folder)
        parse_output_name = parse_output_path.parent.name + '\\' + parse_output_path.name
        self.test_msg(f"Parse output folder: {parse_output_name}")
        self.test_msg("")