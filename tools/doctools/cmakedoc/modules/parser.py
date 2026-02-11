"""CMake Doc model parser.

Parse macros and functions from a cmake file.
"""

import logging
import pathlib
from typing import List

from modules.model import ParamItem, Model

__copyright__ = "Copyright (c) 2025 CCL Software Licensing GmbH"


class ParserError(Exception):
    """Custom exception for handling file parser errors."""

    def __init__(self, message):
        super().__init__(message)


class Parser:

    def __init__(self, model: Model):
        """Initialize parser.

        :param model: data container to fill
        """
        self.model: Model = model

    def _parse_item(self, line: str, kind: str, comment_lines: List[str], source: str) -> None:
        """Extract a relevant item from line.
        @param line:  stripped line from cmake file
        @param kind:  target item type (macro, function)
        """

        # Tokenize: "macro (foo bar zed)" -> return list ["foo", "bar", "zed"]
        tokens = line[line.find("(") + 1:line.find(")")].split(" ")
        item_name = tokens[0]

        # Extract parameters from function or macro signature.
        signature_params = tokens[1:]
        param_items = {}
        # Reminder: dict insertion order is kept intact, i.e. parameters can not be re-arranged.
        for p in signature_params:
            param_items[p] = ParamItem(name=p, type="", comment="")

        # Extract group information, if any
        group = "Shared"
        for c in comment_lines:
            if "@group" in c:
                group = c.replace("@group", "").replace("#", "").strip()
                break

        # Extract parameter details. Assign to parameter info
        # read from function or macro signature.
        for c in comment_lines:
            if "@param" not in c:
                continue
            stripped = c.replace("@param", "").replace("#", "").strip()
            param_tokens = stripped.split(" ")
            if not param_tokens:
                continue

            # Type is optional. If type is specified, assume all tokens after it to be related
            # to the brief comment. Important: allow parameters from comments to be added even
            # when they do not appear in the command signature. This is needed for optional
            # cmake command arguments.
            if param_tokens[0].startswith("{"):
                param_name = param_tokens[1]
                item = param_items.get(param_name, ParamItem(name=param_name, type="", comment=""))
                item.type = param_tokens[0][1:-1]
                item.comment = " ".join(param_tokens[2:])
                param_items[param_name] = item

                # Sanity check only.
                if not item.type.upper() in Model.VAR_TYPES:
                    logging.warning(f"unsupported var type '{item.type}' in file '{source}'")
            else:
                param_name = param_tokens[0]
                item = param_items.get(param_name, ParamItem(name=param_tokens[0], type="", comment=""))
                item.comment = " ".join(param_tokens[1:])
                param_items[param_name] = item

        # Extract "brief" comment for macro or function.
        # Stop after first occurrence of a keyword such as '@param' or '@group'.
        command_comment = []
        for c in comment_lines:
            if "@param" in c or "@group" in c:
                break
            command_comment.append(c)

        self.model.add(name=item_name, kind=kind, group=group, source=source, comment=command_comment,
                       params=list(param_items.values()))
        logging.debug(
            f"parsed item={item_name}, kind={kind}, group=group, source=source, comment={command_comment}, params={signature_params}")

    def parse(self, file: pathlib.Path) -> None:
        """Read commands from file, add results to model.

        :param file: path to input file
        """
        comment_lines = []

        try:
            file_lines = list(file.open())
        except (FileNotFoundError, PermissionError, IOError):
            raise ParserError(f"failed to parse file '{file}'")

        for raw_line in file_lines:
            line: str = raw_line.rstrip()
            # Comment line may start with "#" or "# ". Important: read anything
            # after that as raw rst text which is indentation sensitive.
            if line.startswith("# "):
                comment_lines.append(line[2:])
            elif line.startswith("#"):
                comment_lines.append(line[1:])
            elif not line:
                comment_lines.clear()
            elif line.startswith("macro ("):
                self._parse_item(line, Model.TYPE_MACRO, comment_lines, file.name)
            elif line.startswith("function ("):
                self._parse_item(line, Model.TYPE_FUNCTION, comment_lines, file.name)
            else:
                continue

        logging.debug("found %d elements" % self.model.count())
