"""Unit tests for CMake doc parser and printer.
"""

import pathlib
import unittest

from modules.model import Model
from modules.parser import Parser
from modules.printer import Printer

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ParserTest(unittest.TestCase):
    """Test fixture for processing test resource file 'coremacros.cmake'."""

    RESOURCE_PATH = "test/resource/"  # path to test files, must be relative to project root
    TEMPLATES_PATH = "templates"

    def setUp(self) -> None:
        self.model = Model()
        self.parser = Parser(self.model)
        self.test_file = pathlib.Path(ParserTest.RESOURCE_PATH, "coremacros.cmake")
        self.parser.parse(self.test_file)
        self.printer = Printer(model=self.model, templates_path=pathlib.Path(ParserTest.TEMPLATES_PATH))

    def test_print(self):
        """Verify reference documentation file is generated correctly."""

        result = self.printer.print_reference()

        # Create output file.
        out_file = pathlib.Path(ParserTest.RESOURCE_PATH, f"{self.test_file.stem}.rst.out")
        with out_file.open(mode='w+') as f:
            f.write(result)

        # Compare to reference file.
        reference_file = pathlib.Path(ParserTest.RESOURCE_PATH, f"{self.test_file.stem}.rst.ref")
        with reference_file.open(mode='r') as f:
            expected = f.read()
            self.assertEqual(expected, result)

    def test_print_refs(self):
        """Verify reference file is generated correctly."""

        result = self.printer.print_refs()

        # Create output file.
        out_file = pathlib.Path(ParserTest.RESOURCE_PATH, "cmakedb.json.out")
        with out_file.open(mode='w+') as f:
            f.write(result)

        # Compare to reference file.
        reference_file = pathlib.Path(ParserTest.RESOURCE_PATH, "cmakedb.json.ref")
        with reference_file.open(mode='r') as f:
            expected = f.read()
            self.assertEqual(expected, result)


if __name__ == "__main__":
    unittest.main()
