"""CppHint parser module.
"""

import logging
import pathlib

from typing import List, Optional

from modules.shared import MacroType, Macro, Condition, Source

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

from modules.stringutil import StringParser, ParseConditionException


class MacroClassifier:
    """Class to heuristically determine a macro type using classify ()"""

    @staticmethod
    def classify(text: str) -> MacroType:
        """Scan macro text for certain patterns and decide
        on a 'type' of macro.

        Expand this list as needed to detect more types.
        Warning: order of checks may matter.
        """

        if MacroClassifier._is_include_guard_macro(text):
            return MacroType.INCLUDE_GUARD

        if MacroClassifier._is_parameterized(text):
            return MacroType.PARAMETERIZED

        if MacroClassifier._is_non_parameterized(text):
            return MacroType.NON_PARAMETERIZED

        else:
            return MacroType.UNKNOWN

    @staticmethod
    def _is_parameterized(text: str) -> bool:
        """Check if 'text' denotes a function type macro.

        Example match:
            '#define SOME_MACRO(param1,param2) impl...'

        Example mismatches:
            '#define FOO BAR ("value")'
            '#define FOO if(1)'
        """

        function_name = StringParser.extract_parameterized_name(text)
        contains_brackets = all(s in function_name for s in ["(", ")"])
        return contains_brackets

    @staticmethod
    def _is_non_parameterized(text: str) -> bool:
        """Check if 'text' denotes a non-function type macro.

        Example match:
            '#define SOME_MACRO impl...'

        Example mismatches:
            '#define FOO(a) BAR ("value")'
            '#define FOO(a) if(1)'
        """

        return not MacroClassifier._is_parameterized(text)

    @staticmethod
    def _is_regular_macro(text: str) -> bool:
        tokens = text.split(" ")
        name = tokens[1]
        contains_brackets = any(s in name for s in ["(", ")"])
        return not contains_brackets

    @staticmethod
    def _is_include_guard_macro(text: str) -> bool:
        """Example: #define _some_file_h"""
        stripped_text = text.strip()
        return stripped_text.startswith("#define _") and stripped_text.endswith("_h")

    @staticmethod
    def _is_value_def_macro(text: str) -> bool:
        """Check if macro could be a value assignment.
        These can have different forms:

        '#define SOME_MACRO "value"
        '#define SOME_MACRO "value" // comment'
        '#define SOME_MACRO CCLSTR ("value")'
        '#define SOME_MACRO CCLSTR ("value") // comment'
        '#define SOME_MACRO 123'
        '#define SOME_MACRO 123 // comment'

        """
        tokens = text.split(" ")
        if not tokens or len(tokens) < 3:
            return False

        # Check value at multiple positions.
        candidates = [tokens[2]]
        if len(tokens) > 3:
            candidates.append(tokens[3])

        for candidate in candidates:
            if '"' in candidate:
                return True

            if candidate.isdigit():
                return True

        return False


class ConditionalStatementHandler:

    def __init__(self, filepath: pathlib.Path, conditions: List[Condition]) -> None:
        self.conditions: List[Condition] = conditions
        self.filepath = filepath
        self.active_condition: Optional[Condition] = None

    def process_line(self, line: str, line_number: int) -> bool:
        """Track ifdef or #if fblock. Returns true if line was
        consumed by this function, false otherwise.
        """

        try:
            if line.startswith("#ifdef") or line.startswith("#if "):
                cond_name = StringParser.parse_condition_name(line)
                self._log_debug(token="#ifdef", line_number=line_number)
                if self._is_registered_condition(cond_name):
                    self.active_condition = Condition(name=cond_name, value=True)
                return True

            if line.startswith("#ifndef"):
                cond_name = StringParser.parse_condition_name(line)
                self._log_debug(token="#ifndef", line_number=line_number)
                if self._is_registered_condition(cond_name):
                    self.active_condition = Condition(name=cond_name, value=False)
                return True

            if line.startswith("#elif"):
                cond_name = StringParser.parse_condition_name(line)
                self._log_debug(token="#elif", line_number=line_number)
                if self.active_condition is not None:
                    self.active_condition = Condition(name=cond_name, value=True)
                return True

        except ParseConditionException as e:
            line_shorted = f"{line[: 12]}..."
            logging.debug(
                f"could not parse unsupported '{line_shorted}' in '{self.filepath.name}' line {line_number}: {e}")
            # Consume line
            return True

        # Invert existing condition
        if line.startswith("#else"):
            # assert self.active_condition is not None
            self._log_debug(token="#else", line_number=line_number)
            if self.active_condition:
                self.active_condition = Condition(name=self.active_condition.name, value=False)

            return True

        # End condition.
        if line.startswith("#endif"):
            self._log_debug(token="#endif", line_number=line_number)
            if self.active_condition is not None:
                self.active_condition = None
            return True

        return False

    def is_satisfied(self) -> bool:
        """Check if active condition is met."""

        if self.active_condition is not None:
            if not self._active_condition_satisfied():
                return False

        return True

    def _log_debug(self, token: str, line_number: int):
        """Print log message for debugging purpose."""
        logging.debug(f"{token}, file '{self.filepath}', line {line_number}")

    def _active_condition_satisfied(self) -> bool:
        """Check if active_condition is met with respect to registered
        conditions. Unknown conditions are always considered satisfied.
        The conditions filter is to be used as a restriction.
        """

        for d in self.conditions:
            if d.name == self.active_condition.name:
                return d.value == self.active_condition.value

        return True

    def _is_registered_condition(self, condition_name: str) -> bool:
        """Track conditions from the config file only to not handle
        false positives such as include guards.
        """

        for d in self.conditions:
            if d.name == condition_name:
                return True

        return False


class MacroParser:

    def __init__(self, scan_path: pathlib.Path, sources: List[Source], conditions: List[Condition]) -> None:
        self.scan_path = scan_path
        self.sources = sources
        self.conditions = conditions

    def run(self) -> List[Macro]:
        """Parse scan_path recursively, process included files."""

        result: List[Macro] = list()
        for source in self.sources:
            try:
                self._parse_file(source, result)
            except OSError as e:
                logging.error(f"could not parse file '{source.path}': {e}")

        return result

    @staticmethod
    def _append_macro(macros: List[Macro], macro_text_raw: str, source: Source) -> None:
        """Add new macro to 'macros' output container, do some cleanup and classification."""

        macro_text_clean = MacroParser._cleanup_sourcecode(macro_text_raw)
        macro_type = MacroClassifier.classify(macro_text_clean)
        macros.append(Macro(text=macro_text_clean, source=source, type=macro_type))

    def _parse_file(self, source: Source, macros: List[Macro]) -> None:
        """Parse file for #define macros, add result to 'macros' container."""

        filepath = source.path
        if filepath is None:
            logging.error(f"source {source} has no path")

        condition_handler = ConditionalStatementHandler(filepath, self.conditions)

        with filepath.open(mode="r") as file:
            logging.info(f"parsing file '{filepath}'")

            lines = file.readlines()
            macro = ""
            line_number = 0
            for line in lines:
                line_number = line_number + 1

                # Cleanup, remove potential trailing comments.
                # Typically occur on single line macros.
                line = line.strip()
                line = StringParser.remove_trailing_comment(line)

                if condition_handler.process_line(line, line_number):
                    continue

                if not condition_handler.is_satisfied():
                    continue

                # Skip comment lines
                if line and (line.startswith("//") or line.startswith("/*")):
                    continue

                # Start of a new macro.
                if line.startswith("#define"):
                    assert not macro
                    # One-liner
                    if not line.endswith("\\"):
                        MacroParser._append_macro(macros, line, source)
                    # Multi-line
                    elif line.endswith("\\"):
                        macro += line
                    else:
                        logging.warning(f"unexpected line {line}")
                    continue

                # Multiline macro, line ends on '\'.
                if macro and line.endswith("\\"):
                    macro += line
                    continue

                # Last line of a macro, does not end on '\' character.
                if macro and line:
                    macro += line
                    MacroParser._append_macro(macros, macro, source)
                    macro = ""
                    continue

                # Stop contributing to macro on empty line.
                if not line and macro:
                    MacroParser._append_macro(macros, macro, source)
                    macro = ""
                    continue

    @staticmethod
    def _cleanup_sourcecode(parsed_text: str) -> str:
        result = parsed_text

        # (1) Remove tabs, (2) replace newlines, (3) replace macro linebreak '\'
        result = result.replace("\t", " ")
        result = result.replace("\n", " ")
        result = result.replace("\\", " ")

        # Consolidate superfluous whitespaces to single whitespaces.
        while "  " in result:
            result = result.replace("  ", " ")

        return result
