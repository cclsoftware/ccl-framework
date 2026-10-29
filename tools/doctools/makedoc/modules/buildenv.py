#!/usr/bin/python
"""Makedoc build environment module.
"""

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

import logging
import pathlib
import platform
from typing import Optional

from modules.tools import ToolHelper
from ccl_extras.repoinfo import RepositoryInfo, RepositoryContext


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

        repo_info = RepositoryInfo.load_from(self.makedoc_folder)
        if repo_info.get_context () == RepositoryContext.META:
            logging.info(f"using meta repo '{repo_info.get_meta_root_path()}' context")
        else:
            logging.info(f"using framework repo '{repo_info.get_framework_root_path()}' context")

        return repo_info
