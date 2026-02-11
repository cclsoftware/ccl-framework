#!/usr/bin/python
"""Makedoc tools module.
"""

import datetime
import logging
import os
import pathlib
import subprocess
import sys
from typing import Optional, List, Dict

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "../../../../../ccl/extras/python"))
from tools.findtool import find_binary_tool, find_python_tool, FindToolException
from tools.repoinfo import RepositoryInfo


class SphinxTool:
    """Wrap sphinx-build tool."""

    BINARY: str = "sphinx-build"

    @staticmethod
    def run(format: str, input_path: pathlib.Path, output_path: pathlib.Path) -> None:
        try:
            now = datetime.datetime.now()
            subprocess.run([SphinxTool.BINARY, "-M", format, input_path, output_path])
            duration = (datetime.datetime.now() - now).total_seconds()
            logging.info(f"sphinx {format} build written to {output_path}, took {duration} secs")
        except FileNotFoundError:
            logging.error(f"{SphinxTool.BINARY} executable not found")


class PdfTool:
    """Wrap PDF tool."""

    BINARY = "pdflatex"  # tool to convert latex files to pdf, must be installed

    @staticmethod
    def run(latex_file: pathlib.Path, latex_out_path: pathlib.Path) -> None:
        try:
            logging.info(f"pdf build started for {latex_file}")
            now = datetime.datetime.now()
            subprocess.run([PdfTool.BINARY, latex_file], cwd=latex_out_path, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
            subprocess.run([PdfTool.BINARY, latex_file], cwd=latex_out_path, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
            duration = (datetime.datetime.now() - now).total_seconds()
            logging.info(f"pdf build finished in {duration} secs")
        except FileNotFoundError:
            logging.error(f"{PdfTool.BINARY} executable not found")


class Tools:
    """Define repository tool names and categories."""

    CATEGORY_CCL = "ccl"
    CATEGORY_DOCTOOLS = "doctools"

    CCLMODELLER = "cclmodeller"  # ccl, binary
    DOXYREST = "doxyrest"  # doctools, binary
    DOXYREST_SPHINX = "doxyrest.py"  # doctools, script
    CLASSMODELRST = "classmodelrst"  # doctools, script
    CLASSMODELDTS = "classmodeldts"  # doctools, script
    CMAKEDOC = "cmakedoc"  # doctools, script
    CREFDBFIX = "crefdbfix"  # doctools, script
    TYPEDOCRST = "typedocrst"  # doctools, script
    CODESTATS = "codestats"  # doctools, script


class ToolHelper:
    """Tool access utility class."""

    def __init__(self, system, repo: RepositoryInfo) -> None:
        self.tools: Dict[str, pathlib.Path] = dict()
        self.system = system
        self.repo = repo

    def find_binary_tool(self, name: str, category: str) -> Optional[pathlib.Path]:

        try:
            return find_binary_tool(name=name, category=category, repo=self.repo)
        except FindToolException as e:
            logging.warning(e)

        return None

    def find_python_tool(self, name: str, category: str) -> Optional[pathlib.Path]:

        try:
            return find_python_tool(name=name, category=category, repo=self.repo)
        except FindToolException as e:
            logging.warning(e)

        return None

    def find_doxyrest_sphinx_path(self) -> Optional[pathlib.Path]:
        """Attempt to lookup location of doxyrest sphinx extension which is platform dependent.
        Lookup strategy: find the extension script file 'doxygen.py', then return the parent path
        that is expected to contain the 'sphinx' sub folder.
        """

        try:
            result = find_python_tool(name=Tools.DOXYREST_SPHINX, category=Tools.CATEGORY_DOCTOOLS,
                                      repo=self.repo)
            if result is None:
                return None

            sphinx_parent_folder = result.parents[1]
            assert sphinx_parent_folder.joinpath("sphinx").is_dir()
            return sphinx_parent_folder

        except FindToolException as e:
            logging.warning(e)

    def get_npm_command(self, name: str) -> str:
        """Convert NPM script name to platform executable name.
        On Windows, '.cmd' suffix must be added to make the script
        callable via subprocess.call().

        Parameters:
            name: name of typedoc command
        """

        # TODO: subprocess.call() npm command on Linux/Mac untested
        if self.system == "Windows":
            return f"{name}.cmd"

        return name
