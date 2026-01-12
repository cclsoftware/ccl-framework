"""CCL findtool.py module.
Locate binary and Python script tools in repository context with
respect to platform this script is executed on.
"""

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

import pathlib
import platform
from dataclasses import dataclass
from typing import Optional, List, Tuple

from tools.repoinfo import RepositoryInfo


class FindToolException(Exception):
    """Exception raised for errors in the action implementation."""

    def __init__(self, message: str):
        super().__init__(message)


@dataclass
class SearchCandidate:
    """Private helper class. Pair tool absolute path
    with its tools/ origin path.
    """

    tool_path: pathlib.Path  # tool path, absolute
    source_path: pathlib.Path  # tools/ source path, absolute


def find_binary_tool(name: str, category: str, repo: RepositoryInfo) -> Optional[pathlib.Path]:
    """Lookup tool of given category and name with respect
    to current platform heuristically.

    Detection is based on certain keywords in the candidate path, like
    "/bin/win/ccl/tool.exe" for a 'ccl' category tool on Windows platform or
    "/bin/linux/ccl/x86_64/tool for a 'ccl' category tool on Linux x86.

    Parameters:
        name: name of tool to find
        category: category of tool to find, optional
        repo: repo context info, provides "tools/" locations
    """

    # Cleanup tool name: support w/o ".exe" on Windows platform,
    # expect no file extension on other platforms.
    name_path = pathlib.Path(name)
    system = platform.system()
    if system == "Windows":
        if not name_path.suffix == ".exe":
            name_path = name_path.with_suffix('.exe')
    else:
        if name_path.suffix != "":
            name_path = name_path.with_suffix('')

    search_paths = __determine_search_paths(repo)
    candidates: List[SearchCandidate] = __lookup_candidates(str(name_path), search_paths)

    platform_filter = __get_platform_filters(system=system, arch=platform.machine())
    category_filter = ["bin", category] if category else ["bin"]
    combined_filter = platform_filter + category_filter
    filtered = filter_candidates(candidates=candidates, filters=combined_filter)

    if not filtered:
        raise FindToolException(f"failed to find tool name={name}, category={category}, filter={combined_filter}")

    if len(filtered) != 1:
        raise FindToolException(
            f"found non-unique tool name={name}, category={category}, filter={combined_filter}, candidates={filtered}")

    return filtered[0]


def find_python_tool(name: str, category: str, repo: RepositoryInfo) -> Optional[pathlib.Path]:
    """Find Python script tool, may be platform-specific or platform-agnostic.

    Parameters:
        name: name of Python script, may include ".py" file extension
        category: tool category, optional
        repo: repo context info, provides "tools/" locations
    """

    name_path = pathlib.Path(name)
    if not name_path.suffix == ".py":
        name_path = name_path.with_suffix(".py")

    search_paths = __determine_search_paths(repo)
    candidates = __lookup_candidates(str(name_path), search_paths)
    if not candidates:
        return None

    category_filter = [category] if category else []
    platform_filter = __get_platform_filters(system=platform.system(), arch=platform.machine())

    # Try with platform filter, if no results were found fallback to category only filter.
    # Expect in most cases that Python scripts are platform-agnostic.
    active_filter = platform_filter + category_filter
    filtered = filter_candidates(candidates=candidates, filters=active_filter)
    if not filtered:
        active_filter = category_filter
        filtered = filter_candidates(candidates=candidates, filters=active_filter)

    if not filtered:
        raise FindToolException(f"failed to find tool name={name}, category={category}, filter={active_filter}")

    if len(filtered) != 1:
        raise FindToolException(
            f"found non-unique tool name={name}, category={category}, filter={active_filter}, candidates={filtered}")

    return filtered[0]


def __determine_search_paths(repo: Optional[RepositoryInfo]) -> List[pathlib.Path]:
    """Determine tools/ paths with respect to current repository context.
    If the context implies no meta repository, use CCL tools/ as default.

    Parameters:
        repo: repo context info, provides "tools/" locations
    """

    result: List[pathlib.Path] = []

    # Repo info may be None
    if repo:
        result = repo.get_paths(RepositoryInfo.CATEGORY_TOOLS)

    # Repo info may exist but not specify tool paths
    # Fallback to framework default path.
    if not result:
        result.append(RepositoryInfo.get_ccl_tools_path())

    return result


def __lookup_candidates(name: str, paths: List[pathlib.Path]) -> List[SearchCandidate]:
    """Search for files named 'name' recursively in all directories contained
    in 'paths'. May return empty list.

    Parameters:
        name: filename to lookup
        paths: directories to search in
    """

    result: List[SearchCandidate] = list()
    for path in paths:
        result.extend([
            SearchCandidate(tool_path=f, source_path=path)
            for f in path.rglob(name) if f.is_file()
        ])

    return result


def __get_platform_filters(system: str, arch: str) -> List[str]:
    """Get path filter strings for current platform."""

    filters = []
    if system == "Windows":
        filters.append("win")
    elif system == "Darwin":
        filters.append("mac")
    elif system == "Linux":
        filters.extend(["linux", arch])

    return filters


def filter_candidates(candidates: List[SearchCandidate], filters: List[str]) -> List[pathlib.Path]:
    """Filter path candidates by category and platform, return matching paths.
    Result list may be empty.

    Path examples:
        bin/win/ccl/cclmodeller -> Windows platform
        bin/linux/ccl/x86_64/cclmodeller -> Linux x86_64 platform

    Parameters:
        candidates: list of candidates
        filters: keywords that must be contained in path
    """

    def contains_all(expected: List[str], actual: Tuple[str]) -> bool:
        """Check if all elements of 'expected' are contained in 'actual'.

        Avoid implementation using all() function from Python 3.10 here for Python 3.9 compatibility reasons.
        Script may run on GitHub Actions windows runner which supports Python 3.9 as of 2024-05-16.

        Parameters:
            expected: elements to check actual for
            actual: list to check for elements of 'expected'
        """

        for e in expected:
            if e not in actual:
                return False

        return True

    result = []
    for candidate in candidates:
        # For filtering, only consider the part of the tool path inside tools/,
        # not outside of it. Upper directory structures may redundantly contain
        # filter keywords ("win", ...), causing false positives.
        relative_path = candidate.tool_path.relative_to(candidate.source_path)
        if contains_all(filters, relative_path.parts):
            result.append(candidate.tool_path)

    return result
