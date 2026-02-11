"""CMake Doc controller.

Query and prepare model data for use in view/templates.
"""

from dataclasses import dataclass
from typing import List

from modules.model import CommandItem, Model

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class CategorizedCommands:
    name: str
    commands: List[CommandItem]


class Controller:

    @staticmethod
    def get_commands_sorted_by_name(model: Model) -> List[CommandItem]:
        return sorted(model.get_commands(), key=lambda i: i.name, reverse=False)

    @staticmethod
    def get_commands_by_group(model: Model) -> List[CategorizedCommands]:
        """Group commands by their assigned group."""

        groups = list()
        for group in sorted(model.get_groups()):
            items = [item for item in model.data if item.group == group]
            items = sorted(items, key=lambda i: i.name, reverse=False)

            groups.append(CategorizedCommands(
                name=group.capitalize(),
                commands=items
            ))

        return groups

    @staticmethod
    def get_commands_by_kind(model: Model) -> List[CategorizedCommands]:
        """Group commands by their kind."""

        kinds = list()
        for kind in sorted(model.get_kinds()):
            items = [item for item in model.data if item.kind == kind]
            items = sorted(items, key=lambda i: i.name, reverse=False)

            kinds.append(CategorizedCommands(
                name=f"{kind.capitalize()}s",
                commands=items
            ))

        return kinds
