#!/usr/bin/python
"""Makedoc build environment module.
"""
import logging
import pathlib
import os
import platform
import sys
from typing import Optional

from modules.tools import ToolHelper

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "../../../../../ccl/extras/python"))

from tools.repoinfo import RepositoryInfo

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class BuildEnvironment:
    OUTPUT_DIR = "output"  # makedoc output directory
    BUILD_TEMP_DIR = "~temp"  # makedoc temporary build directory for meta project builds

    def __init__(self, makedoc_folder: pathlib.Path) -> None:
        self.makedoc_folder = makedoc_folder
        self.output_path = pathlib.Path(self.makedoc_folder, BuildEnvironment.OUTPUT_DIR)
        self.build_temp_folder = pathlib.Path(self.makedoc_folder, BuildEnvironment.BUILD_TEMP_DIR)
        self.doxygen_common_path = pathlib.Path(self.makedoc_folder, "doxygen")
        self.sphinx_common_path = pathlib.Path(self.makedoc_folder, "sphinx")

        self.repo_info = self._init_repo_info()
        self.system = platform.system()  # Host system (e.g. "Windows", "Darwin", ...)
        self.machine = platform.machine()  # Host platform architecture (e.g. Linux "x86_64", ...)

        self.tool_helper = ToolHelper(system=self.system, repo=self.repo_info)

    def _init_repo_info(self) -> Optional[RepositoryInfo]:
        """Init repository info from repo.json file. Availability of a repo.json file
        implies this script is run in context of a meta repository.
        """

        repo_info = RepositoryInfo()
        if not repo_info.load(self.makedoc_folder):
            logging.info("repo.json file not found")
            return None

        logging.info(f"found repo.json file in path '{repo_info.get_file_path()}'")
        return repo_info
