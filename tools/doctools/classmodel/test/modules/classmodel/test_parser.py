"""Unit tests for classmodel_to_rest converter script."""

import os
import unittest

from modules.classmodel.model import Model
from modules.classmodel.parser import Parser
from modules.classmodel.printer import ClassModelPrinter, ClassModelRefPrinter
from modules.classmodel.elementfilter import ElementFilter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

CLASSMODEL_PATH = "test/resource/classmodel/"

TEST_PARAMS = [
    ("Classes.classModel", "classes-classmodel"),
    ("Objects.classModel", "objects-classmodel"),
    ("Skin Elements.classModel", "skin-elements-classmodel"),
    ("Visual Styles.classModel", "visual-styles-classmodel")
]


class ParserTest(unittest.TestCase):
    """Parser class test fixture."""

    @staticmethod
    def _write_result_to_file(result_str: str, filename: str) -> None:
        """Write test result file."""

        file_path = os.path.join(CLASSMODEL_PATH, filename)
        with open(file_path, mode='w') as out_file:
            out_file.write(result_str)

    def _run_test(self, classmodel: str, prefix: str) -> None:
        """
        Generate model, convert to rst string for both full output
        and reference links document. Compare both generated strings
        to strings from reference files.
        """

        ##################################################
        # Fill model with test data.
        ##################################################

        model = Model()
        input_file = os.path.join(CLASSMODEL_PATH, classmodel)
        parser = Parser(ref_prefix=prefix, external_files_path="", model=model, element_filter=ElementFilter())
        parser.parse_file(input_file)

        ##################################################
        # Generate and verify full documentation output.
        ##################################################

        full_doc_string = ClassModelPrinter.model_data_to_string(model, prefix)
        ParserTest._write_result_to_file(full_doc_string, f"{prefix}.out")
        ref = f"{prefix}.ref"
        with open(os.path.join(CLASSMODEL_PATH, ref), mode='r') as f:
            expected = f.read()
            self.assertEqual(expected, full_doc_string)

        ##################################################
        # Generate and verify references list output.
        ##################################################

        links_doc_string = ClassModelRefPrinter.model_data_to_string(model, prefix)
        ParserTest._write_result_to_file(links_doc_string, f"{prefix}.links.out")
        ref = f"{prefix}.links.ref"
        with open(os.path.join(CLASSMODEL_PATH, ref), mode='r') as f:
            expected = f.read()
            self.assertEqual(expected, links_doc_string)

    def test_parser(self) -> None:
        """Run parameterized test."""

        for classmodel, prefix in TEST_PARAMS:
            with self.subTest(classmodel=classmodel, prefix=prefix):
                self._run_test(classmodel, prefix)


if __name__ == "__main__":
    unittest.main()
