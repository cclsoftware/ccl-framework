#!/usr/bin/python
"""CodeStats tool shared module.
"""

from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


@dataclass
class FileStat:
    path: Path  # file full path, including filename
    file_extension: str  # file extension
    linecount: int  # number of lines of this file
    excluded: bool  # File is not to be considered for data aggregation


@dataclass
class AggregatedData:
    filecount: int  # number of files with this extension
    filecount_pct: float  # filecount of total filecount, in percent
    linecount: int  # number of lines
    linecount_pct: float  # linecount of total linecount, in percent
    files: List[FileStat]  # file stats

    def __init__(self):
        self.filecount = 0
        self.filecount_pct = 0.0
        self.linecount = 0
        self.linecount_pct = 0
        self.files = []

    def add_stats(self, file_stat: FileStat) -> None:
        self.files.append(file_stat)
        self.linecount += file_stat.linecount
        self.filecount += 1


@dataclass
class ReportData:
    language_stats: Dict[str, AggregatedData]
    total_lines: int
    total_files: int


@dataclass
class ReportJobData:
    description: str  # job title
    timestamp: str  # job execution timestamp
    excluded_dirs: str  # concatenated
    scan_path: str  # job scan path
    file_extensions: str  # job used file extensions as concatenated string
    duration: float  # job scan duration


@dataclass
class DebugLogData:
    timestamp: str  # report timestamp
    file_stats: List[FileStat]  # all files
    scan_path: str  # scanned path
