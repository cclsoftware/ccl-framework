"""CMake Doc model printer.

Convert macros and functions model data to an rst formatted string.
"""

import json
import pathlib

from jinja2 import Environment, FileSystemLoader

from modules.controller import Controller
from modules.model import Model

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Printer:

    def __init__(self, model: Model, templates_path: pathlib.Path):
        """Initialize printer.

        :param model: data container
        """
        self.model = model
        self.environment = Environment(loader=FileSystemLoader(str(templates_path)), lstrip_blocks=True,
                                       trim_blocks=True)

    def print_reference(self) -> str:
        """Create RST representation of model data."""

        template = self.environment.get_template("reference.rst")
        return template.render(grouplist=Controller.get_commands_by_group(self.model),
                               kindlist=Controller.get_commands_by_kind(self.model))

    def print_refs(self) -> str:
        """Create references file content."""

        refs = dict()
        commands = Controller.get_commands_sorted_by_name(self.model)
        for cmd in commands:
            refs[cmd.name] = cmd.refid

        # Dictionary as JSON string
        return json.dumps(refs, indent=2)
