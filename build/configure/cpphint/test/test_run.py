#!/usr/bin/python3
"""CppHint tool functional tests run script.
Script execution based on pytest. Run 'pytest' inside this directory.
Run 'pytest --verbose' for a list of all test cases.
"""

import os
import pathlib
import shutil
import sys
import unittest
import subprocess
from typing import List

import pytest

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

# Magic files
RUN_SCRIPT = "run.sh"  # test individual run.sh script


def init_testcases() -> List[str]:
    """
    Lookup test cases in current working directory. Assume that
    each test case has its own sub folder.

    Identify test case directories by a contained 'run.sh' file
    so the directory names do not need to be prefixed with 'test_'.
    """

    result = list()
    cwd = os.getcwd()
    for item in os.listdir(cwd):
        path = os.path.join(cwd, item)
        if not os.path.isdir(path):
            continue

        run_file = pathlib.Path(path, RUN_SCRIPT)
        if run_file.exists():
            result.append(item)

    return result


@pytest.mark.parametrize('testcase', init_testcases())
def test_run(testcase) -> None:
    """Clear previous output, run testcase run.sh shell script.
    Perform comparison of output directory and individual files.
    :param testcase:  testcase, also the test folder name
    """

    out_path = pathlib.Path(testcase, "out")
    ref_path = pathlib.Path(testcase, "ref")
    shutil.rmtree(out_path, ignore_errors=True)

    # Use cloned environment that provides path to ccl doctools location.
    subprocess.call(["sh", RUN_SCRIPT], cwd=pathlib.Path(".", testcase))

    # Compare file list ref/ vs out/
    ref_files = os.listdir(ref_path)
    assert len(ref_files) > 0
    out_files = os.listdir(out_path)
    assert len(ref_files), len(out_files)

    # Compare files individually
    for ref_file in ref_files:
        ref_file_path = os.path.join(ref_path, ref_file)
        out_file_path = os.path.join(out_path, ref_file)
        with open(ref_file_path, "r") as f:
            expected = f.read()
        with open(out_file_path, "r") as f:
            actual = f.read()
        assert expected == actual


if __name__ == "__main__":
    unittest.main()
