"""CMake Doc model.

Store macros and functions documentation data parsed from a cmake file.
"""

from dataclasses import dataclass
from typing import List

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class ParamItem:
    """ Group parameter description. """
    name: str
    type: str
    comment: str


@dataclass
class CommandItem:
    """Store a macro or function with optional arguments."""
    name: str  # macro or function name
    kind: str  # item type, "macro" or "function"
    group: str  # command group
    source: str  # file this command originates from
    comment: List[str]  # comment block as lines of raw rst markup
    params: List[ParamItem]  # optional: arguments
    refid: str  # Stable identifier for anchors, derived from other fields
    name_with_args: str  # Command name including parameters, derived from other fields


class Model(object):
    """Store parsed commands data."""

    TYPE_FUNCTION = "function"
    TYPE_MACRO = "macro"
    VAR_TYPES = ["BOOL", "FILEPATH", "PATH", "STRING"]  # from cmake 'set' command doc

    def __init__(self):
        self.data: List[CommandItem] = []

    def add(self, name: str, kind: str, group: str, source: str, comment: List[str], params: List[ParamItem]) -> None:

        # Create identifier.
        refid = f"{source}-{name}-{kind}".lower()

        # Create name including arguments
        param_names = []
        for p in params:
            param_names.append(p.name)
        args_string = ", ".join(param_names)
        name_with_args = name + f"({args_string})"

        self.data.append(
            CommandItem(name, kind, group, source, comment, params, refid, name_with_args)
        )

    def count(self) -> int:
        return len(self.data)

    def get_groups(self) -> List[str]:
        """Get list of unique groups."""
        groups = set()
        for c in self.data:
            groups.add(c.group)

        return list(groups)

    def get_kinds(self) -> List[str]:
        """Get list of command types."""
        kinds = set()
        for c in self.data:
            kinds.add(c.kind)

        return list(kinds)

    def get_commands(self) -> List[CommandItem]:
        return self.data
