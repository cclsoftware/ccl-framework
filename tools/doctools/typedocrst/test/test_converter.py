#!/usr/bin/python
"""Typedoc converter class unit test."""

import os
import pathlib
import shutil
import unittest

from modules.converter import TypeDocConverter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ConverterTest(unittest.TestCase):
    """TypeDoc converter class test fixture."""

    def _run_test(self, testname: str) -> None:
        """Run 'testname', expects a resource test case folder
        with same name containing a testname.json input file.
        """

        resource_path = pathlib.Path(os.getcwd(), "test", "resource")
        ref_path = pathlib.Path(resource_path, "ref")
        out_path = pathlib.Path(resource_path, "out")

        # Output files may change, ensure no leftovers of no
        # longer generated files from previous test runs.
        shutil.rmtree(out_path, ignore_errors=True)

        # CUT: call converter class
        input_file = pathlib.Path(resource_path, f"{testname}.json")
        TypeDocConverter.run(file_in=input_file, out_path=out_path, anchor_name=testname, write_reference_file=True)

        # Compare files in ref/ to out/.
        ref_files = os.listdir(ref_path)
        self.assertTrue(len(ref_files) > 0)
        out_files = os.listdir(out_path)
        self.assertEqual(len(ref_files), len(out_files))

        # Compare file by file.
        for ref_file in ref_files:
            ref_file_path = os.path.join(ref_path, ref_file)
            out_file_path = os.path.join(out_path, ref_file)
            with open(ref_file_path, "r") as f:
                expected = f.read()
            with open(out_file_path, "r") as f:
                actual = f.read()
            self.assertEqual(expected, actual)

    def test_convert_sdk(self) -> None:
        """Run test for sdk.json."""
        self._run_test("cclnative")


if __name__ == "__main__":
    unittest.main()
