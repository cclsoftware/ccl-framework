"""Unit tests for CrefDBHandler class.
"""

import pathlib

import unittest

from modules.handler import CrefDBHandler

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class CrefDBHandlerTest(unittest.TestCase):
    """Test fixture for processing test resource file 'crefdb.py'."""

    RESOURCE_PATH = "test/resource/"  # path to test files, must be relative to project root

    def test_run(self):
        """Verify run () creates the correct output."""

        ################################################
        # Step 1: run handler, create output file.
        ################################################

        input_file = pathlib.Path(CrefDBHandlerTest.RESOURCE_PATH, "crefdb.py")
        output_file = pathlib.Path(CrefDBHandlerTest.RESOURCE_PATH, "crefdb.py.out")
        handler = CrefDBHandler(input_file, output_file)
        handler.run()

        ################################################
        # Step 2: compare output to reference output.
        ################################################

        with output_file.open(mode='r') as f:
            output = f.read()

        reference_file = pathlib.Path(CrefDBHandlerTest.RESOURCE_PATH, "crefdb.py.ref")
        with reference_file.open(mode='r') as f:
            expected = f.read()

        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
