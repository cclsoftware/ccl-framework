#!/usr/bin/python
"""CodeStats tool parser module.
"""

import logging
import os
import concurrent.futures
import pathlib
from pathlib import Path
from typing import List, Tuple, Optional

from modules.shared import FileStat

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class LOCParser:

    @staticmethod
    def _count_lines(file_path: Path) -> int:
        """ Count the number of lines in a file. """

        with file_path.open('r', encoding='utf-8', errors='ignore') as f:
            return sum(1 for _ in f)

    @staticmethod
    def _process_file(root: Path, file: str, file_extensions: List[str], exclude_dirs: List[Path]) -> Tuple[
        Optional[str], int, pathlib.Path, bool]:
        """ Retrieve file specific stats. Reminder: returns None
        file extension if the file was not of interest.
        """

        file_path = root / Path(file)
        file_ext = file_path.suffix
        if file_ext in file_extensions:
            excluded = LOCParser.is_path_excluded(path=root, excluded_paths=exclude_dirs)
            return file_ext, LOCParser._count_lines(file_path), file_path.resolve(), excluded

        return None, 0, file_path.resolve(), True

    @staticmethod
    def is_path_excluded(path: Path, excluded_paths: List[Path]) -> bool:
        """ Check if path is not contained in any excluded paths. """

        path_abs = path.resolve()
        for excluded_absolute_path in excluded_paths:
            if path_abs.is_relative_to(excluded_absolute_path):
                logging.debug(f"excluded file '{path_abs}', contained in excluded path '{excluded_absolute_path}'")
                return True

        return False

    @staticmethod
    def run(scan_path: Path, file_extensions: List[str], exclude_dirs: List[Path]) -> List[FileStat]:
        """Query file stats from scan_path with respect to directory filter and file extensions. """

        result: List[FileStat] = []
        with concurrent.futures.ThreadPoolExecutor() as executor:
            future_to_file = {
                executor.submit(LOCParser._process_file, Path(root), file, file_extensions, exclude_dirs): (root, file)
                for root, _, files in os.walk(scan_path)
                for file in files}

            for future in concurrent.futures.as_completed(future_to_file):
                file_ext, lines, file_path, excluded = future.result()
                if file_ext:
                    relative_file_path = file_path.relative_to(scan_path)
                    file_stat = FileStat(path=relative_file_path, file_extension=file_ext, linecount=lines,
                                         excluded=excluded)
                    result.append(file_stat)

        return result
