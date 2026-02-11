#!/usr/bin/python
"""CodeStats tool template module.
"""

from pathlib import Path
from jinja2 import Environment, FileSystemLoader, Template
from modules.shared import ReportData, ReportJobData, DebugLogData

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


def format_int(number: int) -> str:
    """ Formats number with separators. """
    return f"{number:,}".replace(",", ".")


def format_float(number: float) -> str:
    """ Format float number to have ',' as decimal separator. """
    return f"{number:,}".replace(".", ",")


# Define the custom filter
def path_to_rst_string(path: Path) -> str:
    """Convert pathlib path to sphinx escaped format."""

    canonical_path = str(path)
    canonical_path = canonical_path.replace("/", "\\")
    canonical_path = canonical_path.replace("\\", "\\/")

    max_length = 70
    if len(canonical_path) > max_length:
        canonical_path = f"...{canonical_path[-max_length:]}"

    return canonical_path


def str_to_rst_anchor(str: str) -> str:
    """Convert string to a sphinx rst anchor suitable format. """
    return str.lower().replace(" ", "_")


def rjust_int(number: int, width: int) -> str:
    """Right justify input string. """
    return str(number).rjust(width)


def path_to_rst_anchor(path: Path) -> str:
    """Convert path to a sphinx rst anchor suitable format. """

    modified_path = str_to_rst_anchor(path)
    modified_path = modified_path.replace("/", "_")
    return modified_path


def render_report_template(report_data: ReportData, report_job_data: ReportJobData, templates_path: Path,
                           report_format: str) -> str:
    """ Render report template to string. """

    env = Environment(loader=FileSystemLoader(templates_path))
    env.filters['format_int'] = format_int
    env.filters['format_float'] = format_float
    env.filters['path_to_rst_string'] = path_to_rst_string
    env.filters['path_to_rst_anchor'] = path_to_rst_anchor
    env.filters['str_to_rst_anchor'] = str_to_rst_anchor

    template: Template = env.get_template(f"report.{report_format}.in")
    return template.render(report_data=report_data, report_job_data=report_job_data)


def render_debug_log_template(log_data: DebugLogData, templates_path: Path) -> str:
    """ Render debug log template to string. """

    env = Environment(loader=FileSystemLoader(templates_path))
    env.filters['rjust_int'] = rjust_int
    template: Template = env.get_template("debug.log.in")
    return template.render(log_data=log_data)
