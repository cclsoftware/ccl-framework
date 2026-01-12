"""Repository info module.
"""

import json
import os
import pathlib
from typing import Optional, List, Dict

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class RepositoryInfo:
    """Repository info file. Provides list of paths inside a meta repo. Paths
    are organized by category. Example content:

    {
       "skins": [ "path1", "path2", "path3 ],
       "identities": [ "path1", "path2" ],
       "signing": [ "path1" ],
       "templates": [ "path1", "path2" ]
    }

    This file is expected to be located at repository root level.
    Use get_file_path () after succesful load () to retrieve meta
    repo root directory.

    """

    CONFIG_FILE = "repo.json"

    CATEGORY_CLASSMODELS = "classmodels"
    CATEGORY_DOCUMENTATION = "documentation"
    CATEGORY_IDENTITIES = "identities"
    CATEGORY_SIGNING = "signing"
    CATEGORY_SKINS = "skins"
    CATEGORY_SUBMODULES = "submodules"
    CATEGORY_TEMPLATES = "templates"
    CATEGORY_TOOLS = "tools"

    def __init__(self):
        self.file_path: Optional[pathlib.Path] = None  # Absolute path containing 'repo.json', determined by load ()
        self.paths: Dict[str, List[pathlib.Path]] = dict()  # category -> list of paths

    @staticmethod
    def get_ccl_root_path() -> pathlib.Path:
        """Get absolute path to CCL framework root directory."""
        this_path = pathlib.Path(__file__).parent.resolve()
        return pathlib.Path(this_path, "../../../../").resolve()

    @staticmethod
    def get_ccl_build_identities_path() -> pathlib.Path:
        """Get absolute path to CCL framework build/identities directory."""
        return pathlib.Path(RepositoryInfo.get_ccl_root_path(), "build/identities").resolve()

    @staticmethod
    def get_ccl_tools_path() -> pathlib.Path:
        """Determine CCL framework tools/ path."""
        return pathlib.Path(RepositoryInfo.get_ccl_root_path(), "tools")

    @staticmethod
    def get_ccl_doctools_path() -> pathlib.Path:
        """Determine CCL framework 'doctools' path."""
        return pathlib.Path(RepositoryInfo.get_ccl_root_path(), "tools/doctools")

    @staticmethod
    def get_ccl_documentation_path() -> pathlib.Path:
        """Determine CCL framework 'documentation' path."""
        return pathlib.Path(RepositoryInfo.get_ccl_root_path(), "documentation")

    def get_file_path(self) -> pathlib.Path:
        """Get absolute path at which the repo.json file is located.
        Requires successful prior run of load ().
        """

        return self.file_path.resolve()

    def get_paths(self, category: str) -> List[pathlib.Path]:
        """Get paths for given category, can be empty. All
        paths are absolute.
        """

        result: List[pathlib.Path] = list()
        paths = self.paths.get(category, list())
        for path in paths:
            abs_path = pathlib.Path(self.file_path, path).resolve()
            result.append(abs_path)

        return result

    def load(self, start_path: pathlib.Path) -> bool:
        """Attempt to locate and load repo.json file."""

        config_file_folder = self._find_file_path(start_path, RepositoryInfo.CONFIG_FILE)
        if config_file_folder is None:
            return False

        self.file_path = config_file_folder

        # Load .json file data.
        try:
            config_file_path = pathlib.Path(config_file_folder, RepositoryInfo.CONFIG_FILE)
            with config_file_path.open(mode="r") as f:
                json_data = json.load(f)
                for category in json_data:
                    self.paths.setdefault(category, json_data[category])
        except OSError:
            return False

        return True

    @staticmethod
    def _find_file_path(start_path: pathlib.Path, filename: str) -> Optional[pathlib.Path]:
        """Find the first occurrence of the specified file in a directory structure, traversing
        the directory tree upwards. Gracefully track visited paths to guard against potential
        infinite loops caused by symlinks.
        """

        visited_paths = set()
        current_path = start_path.resolve()

        while True:
            # Check if the specified file exists in the current path.
            config_file_path = current_path / filename
            if config_file_path.is_file():
                return current_path

            # Add current path to visited set to detect symlink loops.
            if current_path in visited_paths:
                return None
            visited_paths.add(current_path)

            # If the current path is the root directory, stop searching.
            if current_path == current_path.parent:
                return None

            # Move up to the parent directory.
            current_path = current_path.parent.resolve()
