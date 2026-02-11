#!/usr/bin/python
"""CodeStats tool main script.
"""

import argparse
import datetime
import logging
import sys

from modules.parser import LOCParser
from modules.language import LanguageManager
from modules.generator import generate_report_data
from pathlib import Path
from typing import List
import json

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"
__version__ = "1.0.0"

from modules.shared import ReportJobData, DebugLogData
from modules.template import render_report_template, render_debug_log_template


class JSONConfig:
    """ JSON config file supported attributes. """

    ATTR_DESCRIPTION = 'description'  # scan job title
    ATTR_ROOT_PATH = 'root_path'  # Scan root path
    ATTR_SCAN_PATH = 'scan_path'  # Scan path, relative to root path
    ATTR_EXCLUDE_DIRS = 'exclude_dirs'  # List of excluded paths, relative to scan_path (not root_path!)
    ATTR_OUT_FILE = 'out_file'


def main() -> None:
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s [%(levelname)s] %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')

    logging.info(f"CodeStats v{__version__}, {__copyright__}")

    parser = argparse.ArgumentParser()
    parser.add_argument('config', type=str, help='job config json file')

    args = parser.parse_args()

    job_timestamp = datetime.datetime.now().strftime("%Y-%m-%d, %H:%M:%S")

    ###################################################################
    # Load built-in languages config file
    ###################################################################

    this_path = Path(__file__).parent.resolve()
    lang_config_json = Path(this_path, "lang.json")
    language_handler = LanguageManager()
    language_handler.load_from_config(path=lang_config_json)

    logging.info(f"registered languages {language_handler.get_language_names()}")
    logging.info(f"registered file extensions {language_handler.get_file_extensions()}")

    ###################################################################
    # Load job config file
    ###################################################################

    config_path = Path(args.config)
    logging.info(f"Processing config file '{config_path.resolve()}'...")

    if not config_path.exists():
        logging.error(f"config file '{config_path.resolve()}' does not exist")
        sys.exit(1)

    with config_path.open('r') as config_file:
        config = json.load(config_file)

    # Interpret non-absolute paths as relative to config file.
    repo_path: Path = Path(config[JSONConfig.ATTR_ROOT_PATH])
    if not repo_path.is_absolute():
        repo_path = (config_path.parent.resolve() / repo_path).resolve()

    scan_dir = config[JSONConfig.ATTR_SCAN_PATH]

    ###################################################################
    # Run parser, collect stats for all files.
    ###################################################################

    scan_path = repo_path / scan_dir.strip()
    if not scan_path.exists():
        logging.warning(f"Failed to scan sources, scan path '{scan_path.resolve()}' does not exist")
        sys.exit(1)

    # Make all excluded paths absolute to save redundant path
    # resolving later in the processing. Exclude dirs are
    # interpreted as relative to scan path, NOT relative
    # to root path!

    exclude_dirs_str = config.get(JSONConfig.ATTR_EXCLUDE_DIRS, [])
    exclude_dirs: List[Path] = [Path(exclude) for exclude in exclude_dirs_str]
    exclude_dirs_absolute: List[Path] = []
    for path in exclude_dirs:
        if path.is_absolute():
            exclude_dirs_absolute.append(path)
        else:
            abs_path = Path(scan_path.resolve() / path).resolve()
            exclude_dirs_absolute.append(abs_path)

    scan_start_time = datetime.datetime.now()
    logging.info(f"Scanning directory '{scan_path.resolve()}'...")
    file_stats = LOCParser.run(scan_path=scan_path, file_extensions=language_handler.get_file_extensions(),
                               exclude_dirs=exclude_dirs_absolute)
    scan_end_time = datetime.datetime.now()

    # Calculate scan duration.
    scan_duration_secs = (scan_end_time - scan_start_time).total_seconds()
    scan_duration_secs = round(scan_duration_secs, 2)

    # Calculate number of included and excluded files.
    excluded_count = len([stat for stat in file_stats if stat.excluded])
    included_count = len([stat for stat in file_stats if not stat.excluded])
    logging.info(
        f"Finished scanning '{scan_path.resolve()}', took {scan_duration_secs} secs, included {included_count} files, excluded {excluded_count} files")

    ###################################################################
    # Calculate report statistical data.
    ###################################################################

    # Interpret non-absolute paths as relative to config file.
    report_file: Path = Path(config[JSONConfig.ATTR_OUT_FILE])
    if not report_file.is_absolute():
        report_file = (config_path.parent.resolve() / report_file).resolve()

    report_data = generate_report_data(file_stats=file_stats, language_handler=language_handler)

    ###################################################################
    # Calculate job meta data.
    ###################################################################

    excluded_dirs_string = ", ".join(exclude_dirs_str)
    file_extensions_string = ", ".join(language_handler.get_file_extensions())
    report_job_data = ReportJobData(description=config[JSONConfig.ATTR_DESCRIPTION],
                                    timestamp=job_timestamp,
                                    excluded_dirs=excluded_dirs_string, scan_path=scan_dir,
                                    file_extensions=file_extensions_string, duration=scan_duration_secs)

    ###################################################################
    # Assemble report from template file.
    ###################################################################

    report_format = report_file.suffix.replace(".", "")
    templates_path = Path(this_path, "templates").resolve()
    report_string = render_report_template(report_data=report_data, report_job_data=report_job_data,
                                           templates_path=templates_path,
                                           report_format=report_format)

    with report_file.open('w') as f:
        f.write(report_string)
        logging.info(f"Wrote report '{report_file.resolve()}'")

    ###################################################################
    # Write debug log file.
    ###################################################################

    debug_log_data = DebugLogData(file_stats=sorted(file_stats, key=lambda stat: stat.path),
                                  timestamp=job_timestamp, scan_path=scan_dir)
    debug_log_string = render_debug_log_template(log_data=debug_log_data, templates_path=templates_path)

    report_log_file = report_file.with_suffix(".log")
    with report_log_file.open(mode='w', encoding="utf-8") as f:
        f.write(debug_log_string)
        logging.info(f"Wrote report log '{report_log_file.resolve()}'")


if __name__ == "__main__":
    main()
