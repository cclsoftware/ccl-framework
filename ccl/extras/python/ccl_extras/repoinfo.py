"""CCL repoinfo.py module."""

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

import json
import pathlib
from enum import Enum
from typing import Optional, List, Dict


class RepositoryContext(str, Enum):
    """Repository context type resolved during loading."""

    META = "META"  # Meta repository (can include framework repo)
    FRAMEWORK = "FRAMEWORK"  # Framework-only repository (no meta repo)
    NONE = "NONE"  # No repository context detected, repo.json not found, framework root not found.


class RepositoryInfo:
    """Resolve repository paths for meta-repo and framework-only contexts.

    The class loads optional metadata from a repo.json file at repository root.
    The file maps categories (for example "tools" or "templates") to relative
    paths, which are returned as absolute paths by the get_meta_* helpers.

    If no repo.json is found, the instance still supports framework path lookup
    by scanning for a valid CCL framework root.

    Typical usage:
        repo = RepositoryInfo.load_from(path)
        if repo.get_context() == RepositoryContext.META:
            tools_paths = repo.get_meta_tools_paths()
        else:
            tools_path = repo.get_framework_tools_path()
    """

    CONFIG_FILE = "repo.json"

    def __init__(self):
        self.load_path: pathlib.Path = (
            None  # Path from which the search for repo.json started, used as fallback for framework search.
        )
        self.meta_root_path: Optional[pathlib.Path] = (
            None  # Absolute path containing 'repo.json', determined by load ()
        )
        self.framework_root_path: Optional[pathlib.Path] = (
            None  # Absolute path to CCL framework root, determined by load ()
        )
        self.paths: Dict[str, List[pathlib.Path]] = dict()  # category -> list of paths
        self.context: RepositoryContext = RepositoryContext.NONE

    @classmethod
    def load_from(cls, path: pathlib.Path) -> "RepositoryInfo":
        """Create and load RepositoryInfo in one step. Path should be located inside
        a framework or framework repo containing meta repo. After load_from(), check
        get_context() to determine whether the context is META or FRAMEWORK.

        Parameters:
            path: File or directory path used as starting point when searching
                for repository metadata.

        Returns:
            Loaded RepositoryInfo instance.
        """

        repo = cls()
        repo._load(path)
        return repo

    def get_context(self) -> RepositoryContext:
        """Get the detected repository context.

        Returns:
            RepositoryContext.META when repo.json is found, RepositoryContext.FRAMEWORK
            when only a framework repo was found, RepositoryContext.NONE when no context
            could be detected.
        """

        return self.context

    def get_meta_root_path(self) -> Optional[pathlib.Path]:
        """Get meta repo root as absolute path. This is the path at which
        repo.json was found.

        Returns:
            Absolute meta repository root path, or None when no meta
            repository was detected.
        """

        return self.meta_root_path

    def get_meta_classmodel_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to classmodel folders in a meta repository.

        Returns:
            List of absolute classmodel paths, can be empty.
        """
        return self._get_paths("classmodels")

    def get_meta_documentation_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to documentation folders in a meta repository.

        Returns:
            List of absolute documentation paths, can be empty.
        """
        return self._get_paths("documentation")

    def get_meta_identities_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to identity folders in a meta repository.

        Returns:
            List of absolute identity paths, can be empty.
        """
        return self._get_paths("identities")

    def get_meta_signing_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to signing folders in a meta repository.

        Returns:
            List of absolute signing paths, can be empty.
        """
        return self._get_paths("signing")

    def get_meta_skin_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to skin folders in a meta repository.

        Returns:
            List of absolute skin paths, can be empty.
        """
        return self._get_paths("skins")

    def get_meta_submodule_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to submodule folders in a meta repository.

        Returns:
            List of absolute submodule paths, can be empty.
        """
        return self._get_paths("submodules")

    def get_meta_template_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to template folders in a meta repository.

        Returns:
            List of absolute template paths, can be empty.
        """
        return self._get_paths("templates")

    def get_meta_tools_paths(self) -> List[pathlib.Path]:
        """Get absolute paths to tool folders in a meta repository.

        Returns:
            List of absolute tools paths, can be empty.
        """
        return self._get_paths("tools")

    def get_framework_root_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to the CCL framework repository root.

        Returns:
            Absolute framework root path, or None when it cannot be detected.
        """
        return self._find_ccl_framework()

    def get_framework_identities_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework build identities directory.

        Returns:
            Absolute identities path, or None when framework root cannot be
            detected.
        """
        return self._find_ccl_framework_subpath("build/identities")

    def get_framework_tools_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework tools directory.

        Returns:
            Absolute tools path, or None when framework root cannot be
            detected.
        """
        return self._find_ccl_framework_subpath("tools")

    def get_framework_doctools_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework doctools directory.

        Returns:
            Absolute doctools path, or None when framework root cannot be
            detected.
        """
        return self._find_ccl_framework_subpath("tools/doctools")

    def get_framework_documentation_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework documentation directory.

        Returns:
            Absolute documentation path, or None when framework root cannot be
            detected.
        """
        return self._find_ccl_framework_subpath("documentation")

    def get_framework_generated_path(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework generated Python directory.

        Returns:
            Absolute generated path, or None when framework root cannot be
            detected.
        """
        return self._find_ccl_framework_subpath("ccl/meta/generated/python")

    @staticmethod
    def _is_ccl_framework_path(path: pathlib.Path) -> bool:
        """Check if the given path is a CCL framework path by looking for
        ccl/public/cclversion.h file."""

        return (path / "ccl/public/cclversion.h").is_file()

    @staticmethod
    def _find_file_path(
        start_path: pathlib.Path, filename: str
    ) -> Optional[pathlib.Path]:
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

    def _load(self, path: pathlib.Path) -> None:
        """Attempt to locate and load repo.json file, decide repository
        context to use."""

        self.load_path = path.resolve()

        # Try framework-only context.
        self.framework_root_path = self._find_ccl_framework_standalone()
        if self.framework_root_path is not None:
            self.context = RepositoryContext.FRAMEWORK

        config_file_folder = self._find_file_path(path, RepositoryInfo.CONFIG_FILE)
        if config_file_folder is None:
            return

        # Try meta repo context.
        self.meta_root_path = config_file_folder.resolve()
        try:
            config_file_path = pathlib.Path(
                config_file_folder, RepositoryInfo.CONFIG_FILE
            )
            with config_file_path.open(mode="r") as f:
                json_data = json.load(f)
                for category in json_data:
                    self.paths.setdefault(category, json_data[category])

            self.context = RepositoryContext.META
        except OSError:
            pass

    def _find_ccl_framework(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework root directory in either meta repo
        or framework-only repo context."""

        if self.context == RepositoryContext.META:
            for submodule_path_str in self.get_meta_submodule_paths():
                submodule_path = pathlib.Path(
                    self.get_meta_root_path(), submodule_path_str
                ).resolve()
                if RepositoryInfo._is_ccl_framework_path(submodule_path):
                    return submodule_path

        return self._find_ccl_framework_standalone()

    def _find_ccl_framework_standalone(self) -> Optional[pathlib.Path]:
        """Get absolute path to CCL framework root directory.

        Search strategy:
        1) Start from self.start_path.
        2) For each directory on the upward path to filesystem root,
           search downward for candidates.
        """

        if self.load_path is None:
            return None

        current_path = self.load_path.resolve()
        if current_path.is_file():
            current_path = current_path.parent

        visited_paths = set()
        while True:
            # Symlink-safe upward traversal guard.
            if current_path in visited_paths:
                return None
            visited_paths.add(current_path)

            # Check current path itself first.
            if RepositoryInfo._is_ccl_framework_path(current_path):
                return current_path

            # Search downward from current path for known framework marker.
            try:
                for version_file in current_path.rglob("cclversion.h"):
                    if not version_file.is_file():
                        continue

                    public_path = version_file.parent
                    if public_path.name != "public":
                        continue

                    ccl_path = public_path.parent
                    if ccl_path.name != "ccl":
                        continue

                    framework_path = ccl_path.parent.resolve()
                    if RepositoryInfo._is_ccl_framework_path(framework_path):
                        return framework_path
            except OSError:
                # Ignore inaccessible directories and continue searching upwards.
                pass

            # Stop at filesystem root.
            if current_path == current_path.parent:
                return None

            current_path = current_path.parent.resolve()

    def _find_ccl_framework_subpath(self, subpath: str) -> Optional[pathlib.Path]:
        """Get absolute path to a subfolder within CCL framework root directory."""

        root_path = self._find_ccl_framework()
        if root_path is None:
            return None

        return pathlib.Path(root_path, subpath).resolve()

    def _get_paths(self, category: str) -> List[pathlib.Path]:
        """Get paths for given category, can be empty. All
        paths are absolute."""

        result: List[pathlib.Path] = list()
        paths = self.paths.get(category, list())
        for path in paths:
            abs_path = pathlib.Path(self.meta_root_path, path).resolve()
            result.append(abs_path)

        return result
