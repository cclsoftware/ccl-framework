"""Unit tests for RstGenerator: config loading/orchestration, plus the HTML ref links
and index file outputs, neither covered elsewhere.
"""

import json
import os
import tempfile
import unittest

from modules.restructuredtext.generator import RstGenerator

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

CLASSMODEL_RESOURCE_PATH = "test/resource/classmodel/"
RESOURCE_PATH = "test/resource/restructuredtext/"


class RstGeneratorTest(unittest.TestCase):
    """ Runs the converter with output redirected to a temp dir, so no generated artifact but
    the '.out' debug copy lands in the resource tree.
    """

    def setUp(self) -> None:
        self._temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp_dir.cleanup)

    def _run_converter(self, generate_shortcut_file: bool, generate_index_file: bool) -> str:
        """ Run the converter against the shared 'Classes.classModel' fixture, return the
        configured output basename.
        """

        output_base = os.path.join(self._temp_dir.name, "classes-classmodel")
        config = {
            "files": [
                {
                    "input": f"{CLASSMODEL_RESOURCE_PATH}Classes.classModel",
                    "output": output_base,
                    "ref-prefix": "classes-classmodel",
                    "external-files-path": ".",
                    "generate-shortcut-file": generate_shortcut_file,
                    "generate-index-file": generate_index_file,
                }
            ]
        }

        config_path = os.path.join(self._temp_dir.name, "config.json")
        with open(config_path, mode="w") as f:
            json.dump(config, f)

        RstGenerator.run_config(config_path)
        return output_base

    @staticmethod
    def _assert_matches_ref(generated_path: str, ref_path: str, out_name: str) -> None:
        """ Compare a generated file against 'ref_path', writing an '.out' debug copy alongside. """

        with open(generated_path, mode="r") as f:
            actual = f.read()

        out_path = os.path.join(RESOURCE_PATH, f"{out_name}.out")
        with open(out_path, mode="w") as f:
            f.write(actual)

        with open(ref_path, mode="r") as f:
            expected = f.read()

        if expected != actual:
            raise AssertionError(f"'{out_name}' does not match '{ref_path}', see '{out_path}'")

    def test_write_rst(self) -> None:
        """ Must match ClassModelPrinter's own golden file for the same input/prefix. """

        output_base = self._run_converter(generate_shortcut_file=False, generate_index_file=False)
        self._assert_matches_ref(f"{output_base}.rst", f"{CLASSMODEL_RESOURCE_PATH}classes-classmodel.ref",
                                 "classes-classmodel.rst")

    def test_write_shortcuts(self) -> None:
        """ RST replace-directives (matches ClassModelRefPrinter's golden file) and HTML links
        (no other coverage).
        """

        output_base = self._run_converter(generate_shortcut_file=True, generate_index_file=False)
        self._assert_matches_ref(f"{output_base}.ref.rst",
                                 f"{CLASSMODEL_RESOURCE_PATH}classes-classmodel.links.ref",
                                 "classes-classmodel.ref.rst")
        self._assert_matches_ref(f"{output_base}.refs.html", f"{RESOURCE_PATH}classes-classmodel.refs.html.ref",
                                 "classes-classmodel.refs.html")

    def test_write_index(self) -> None:
        """ Flat element list for seeding an exclusion list. No other coverage. """

        output_base = self._run_converter(generate_shortcut_file=False, generate_index_file=True)
        self._assert_matches_ref(f"{output_base}.index.txt", f"{RESOURCE_PATH}classes-classmodel.index.txt.ref",
                                 "classes-classmodel.index.txt")

    def test_parse_error_is_handled(self) -> None:
        """ A malformed input must not raise out of run_config() - the job is skipped and logged. """

        malformed_path = os.path.join(self._temp_dir.name, "malformed.classModel")
        with open(malformed_path, mode="w") as f:
            f.write("<not valid xml")

        output_base = os.path.join(self._temp_dir.name, "malformed-out")
        config = {"files": [{"input": malformed_path, "output": output_base}]}
        config_path = os.path.join(self._temp_dir.name, "config.json")
        with open(config_path, mode="w") as f:
            json.dump(config, f)

        RstGenerator.run_config(config_path)  # must not raise
        self.assertFalse(os.path.isfile(f"{output_base}.rst"))


if __name__ == "__main__":
    unittest.main()
