#!/usr/bin/python
"""Makedoc pathutil module.
"""

import pathlib

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


def format_path(path: pathlib.Path) -> str:
    """Format path with forward slashes, suitable for configuration files."""

    return str(path).replace("\\", "/")
