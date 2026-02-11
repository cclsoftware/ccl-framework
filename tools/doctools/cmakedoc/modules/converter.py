#!/usr/bin/python
"""CMake Doc model parser.

Create reference documentation for macros and functions contained in a cmake file.
"""

import logging
import pathlib
from typing import List

from modules.model import Model
from modules.parser import Parser
from modules.printer import Printer

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class CMakeDocConverter:
    TEMPLATES_PATH = "templates/"  # templates folder, must be relative to project root

    @staticmethod
    def run(files_in: List[str], file_out: str, root_path: pathlib.Path) -> None:
        """
        Parse file, build model, convert model to string and write result file.

        :param files_in: input file to parse
        :param file_out: output file to write
        :param root_path: caller script root path (for templates)
        """

        model = Model()
        parser = Parser(model)

        # Make all files contribute to the same model
        for f in files_in:
            file_in_path = pathlib.Path(f)
            parser.parse(file_in_path)

        # Note that data from multiple files is combined into a single model,
        # hence use a global anchor prefix for all items. Expect no name clashes
        # between files.

        templates_path = pathlib.Path(root_path, CMakeDocConverter.TEMPLATES_PATH)
        printer = Printer(model=model, templates_path=templates_path)

        #####################################
        # Create reference documentation file
        #####################################

        result = printer.print_reference()
        if not result:
            return

        # Ensure output directory is created if needed.
        file_out_path = pathlib.Path(file_out)
        file_out_path.parent.mkdir(parents=True, exist_ok=True)
        with file_out_path.open('w+') as f:
            f.write(result)
            logging.info(f"wrote file {file_out_path}")

        #####################################
        # Create references shortcut file
        #####################################

        result = printer.print_refs()
        if not result:
            return

        # Filename must be in sync with custom cmake role sphinx extension.
        file_out_path = pathlib.Path(file_out_path.parent, "cmakedb.json")
        file_out_path.parent.mkdir(parents=True, exist_ok=True)
        with file_out_path.open('w+') as f:
            f.write(result)
            logging.info(f"wrote file {file_out_path}")
