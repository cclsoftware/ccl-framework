#!/usr/bin/python
"""CodeStats tool generator module.
"""

from typing import List, Dict

from modules.language import LanguageManager
from modules.shared import FileStat, ReportData, AggregatedData

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


def generate_report_data(file_stats: List[FileStat], language_handler: LanguageManager) -> ReportData:
    """Assemble ReportData from file statistics. """

    language_stats: Dict[str, AggregatedData] = {}

    # Loop over all file stats and aggregate data per language
    # based on the file extension.
    for file_stat in file_stats:
        if file_stat.excluded:
            continue

        language_name: str = language_handler.get_language_name(file_stat.file_extension)

        # Init for relevant languages only.
        if language_name not in language_stats:
            language_stats[language_name] = AggregatedData()

        language_stats[language_name].add_stats(file_stat)

    subdir_total_lines = 0
    for st in language_stats.values():
        subdir_total_lines += st.linecount

    # Total lines percentage per language.
    for st in language_stats.values():
        st.linecount_pct = round((st.linecount / subdir_total_lines) * 100, 2)

    subdir_total_files = 0
    for st in language_stats.values():
        subdir_total_files += st.filecount

    # Files percentage per language
    for st in language_stats.values():
        st.filecount_pct = round((st.filecount / subdir_total_files) * 100, 2)

    max_file_count = 10
    for stats in language_stats.values():
        stats.files = sorted(stats.files, key=lambda s: s.linecount, reverse=True)[:max_file_count]

    return ReportData(
        language_stats=language_stats,
        total_lines=subdir_total_lines,
        total_files=subdir_total_files
    )
