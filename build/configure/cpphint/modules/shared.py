"""CppHint exporter module.
"""

import pathlib
import re
from dataclasses import dataclass
from enum import Enum

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

from typing import List


@dataclass
class Condition:
    name: str
    value: bool

    def __eq__(self, other):
        """Match by name."""

        if isinstance(other, Condition):
            return self.name == other.name
        return False


@dataclass
class Source:
    path: pathlib.Path
    comment: str
    include_patterns: List[re.Pattern]  # regular expressions
    exclude_patterns: List[re.Pattern]  # regular expressions

    def __eq__(self, other):
        """Match by path, path expected to always be absolute."""

        if isinstance(other, Source):
            return self.path == other.path
        return False


class MacroType(Enum):
    INCLUDE_GUARD = 0,
    PARAMETERIZED = 1,
    NON_PARAMETERIZED = 2,
    UNKNOWN = 3


@dataclass
class Macro:
    """Macro definition as parsed from source or header files."""
    text: str  # full macro string, parsed from multiple lines
    source: Source  # source from configuration
    type: MacroType  # macro type
