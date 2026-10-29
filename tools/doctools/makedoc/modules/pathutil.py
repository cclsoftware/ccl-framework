#!/usr/bin/python
"""Makedoc pathutil module.
"""

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

import pathlib

def format_path(path: pathlib.Path) -> str:
    """Format path with forward slashes, suitable for configuration files."""

    return str(path).replace("\\", "/")
