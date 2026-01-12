"""Gitmodules module. Support for loading .gitmodule files.
"""

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

import pathlib
from typing import List

class GitModuleInfo:
    """Store git submodule info."""

    name: str  # Name of the submodule
    path: pathlib.Path  # Path of the submodule inside current repo
    url: str  # URL of referenced submodule repository
    url_short: str  # Short URL of referenced submodule repository, can be used as stable identifier
    branch: str  # Submodule branch to checkout


class GitModulesReader:
    """Load .gitmodules file."""

    @staticmethod
    def load(path: pathlib.Path) -> List[GitModuleInfo]:
        """ Attempt to load submodule configuration from .gitmodules file.
        Returns an empty list if no file or data could be read.

        A .gitmodule file is expected to consists of multiple blocks of:

            [submodule "module_name"]
            path = path/to/module
            url = git@github.com:organization/module-repository.git
            branch = main
        """

        result = list()

        modules_file = pathlib.Path(path, ".gitmodules")
        with modules_file.open(mode="r") as f:
            entry = None
            for line in f:
                line = line.strip()

                # Create new entry or save and replace existing one.
                if line.startswith("[submodule"):
                    if entry is not None:
                        result.append(entry)

                    entry = GitModuleInfo()
                    entry.name = GitModulesReader._get_name(line)

                # Parse path from line, example line: "path = submodules/cclsoftware/framework"
                # Path is stored absolute, using modulesfile path as working directory.
                elif line.startswith("path"):
                    path_value = GitModulesReader._get_value(line)
                    entry.path = pathlib.Path(path, path_value).resolve()

                # Parse url from line, example line: "path = git@github.com:cclsoftware/ccl-framework.git"
                elif line.startswith("url"):
                    entry.url = GitModulesReader._get_value(line)
                    entry.url_short = GitModulesReader._extract_short_url(entry.url)

                # Get submodule branch
                elif line.startswith("branch"):
                    entry.branch = GitModulesReader._get_value(line)

            # End of file: append last pending entry
            if entry is not None:
                result.append(entry)

        return result

    @staticmethod
    def _get_name(config_line: str) -> str:

        """Retrieve submodule name from config line.

        Example:
            '[submodule "my_submodule"]' -> "my_submodule"
        """

        return config_line[config_line.index('"') + 1:config_line.rindex('"')]

    @staticmethod
    def _get_value(config_line: str) -> str:
        """Retrieve trimmed value from a config line.
        Example:
            "path = a/b/c" -> "a/b/c"

        Returns unmodified config_line on failure.
        """

        tokens = config_line.split("=")
        assert len(tokens) == 2
        if len(tokens) == 2:
            return tokens[1].strip()

        return config_line

    @staticmethod
    def _extract_short_url(github_url: str) -> str:
        """Extract short url from GitHub repository URL.
        Example:
            "git@github.com:organization/repository-name.git" -> "organization/repository-name"

        Returns unmodified github_url on failure.
        """

        url_tokens = github_url.split(":")
        assert len(url_tokens) == 2
        if len(url_tokens) == 2:
            return url_tokens[1].replace(".git", "")

        return github_url
