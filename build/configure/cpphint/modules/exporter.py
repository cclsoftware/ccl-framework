"""CppHint exporter module.
"""

import logging
import pathlib
import re

from abc import abstractmethod, ABC

from typing import List, Dict, Optional

from modules.shared import Macro, MacroType, Source

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

from modules.stringutil import StringParser

cpphint_path = pathlib.Path(__file__).resolve()
cpphint_folder = cpphint_path.parent


class HintLoadException(Exception):

    def __init__(self):
        super().__init__("macro text parsing error")


class Hint(ABC):
    """Hint definition exported to output file."""

    def __init__(self, macro: Macro) -> None:
        self.name = ""  # macro defined name only
        self.impl = ""  # implementation without name and params
        self.macro = macro  # originating macro, also provides the source file

    @abstractmethod
    def load(self, macro_text: str) -> None:
        """Load hint attributes from full macro text. Use as lazy
        init function to keep work out of the constructor."""
        pass

    @abstractmethod
    def replace_in_caller(self, call_text: str) -> str:
        """Replace call of this hint in call_text."""
        pass

    @abstractmethod
    def to_export_string(self):
        """Create string representation used for hint file export."""
        pass

    def get_file_display_string(self):
        """Print file with direct ancestor containing folder.
        Example:
            "/a/b/c/d/file.cpp" -> "d/file.cpp"
        """

        file = self.macro.source.path

        if len(file.parents) > 1:
            display_path = str(pathlib.Path(file.parent.parent.name, file.parent.name, file.name))
        elif len(file.parents) > 0:
            display_path = str(pathlib.Path(file.parent.name, file.name))
        else:
            display_path = file.name

        return str(display_path).replace("\\", "/")


class NonParameterizedHint(Hint):

    def load(self, macro_text: str) -> None:
        # Use token approach, assume that the first token is "#define"
        # and the second token is the name of the macro.
        tokens = macro_text.split(" ")
        if not tokens:
            raise HintLoadException()

        self.name = tokens[1].strip()

        # Get the implementation part by removing the "#define MACRO_NAME" substring.
        self.impl = macro_text.replace(f"#define {self.name}", "").strip()

    def replace_in_caller(self, call_text: str) -> str:
        """Replace call of this macro with the macro implementation.
        Since there are no parameters, simply replace the name with
        the implementation.
        """

        return call_text.replace(self.name, self.impl)

    def to_export_string(self):
        return f"#define {self.name} {self.impl}".strip()


class ParameterizedHint(Hint):
    VARIADIC_ARGS = "__VA_ARGS__"

    def __init__(self, macro: Macro) -> None:
        super().__init__(macro)
        self.params = ""  # parameters string, including ()

    def load(self, macro_text: str) -> None:
        """Split up macro text into name, params and implementation parts.

        Example:
            '#define SOME_MACRO(param1,param2) impl...'
            -> name = 'SOME_MACRO'
            -> params = '(param1,param2)'
            -> impl = 'impl...'
        """
        try:
            self.name = StringParser.extract_macro_name(macro_text).strip()
            self.params = StringParser.extract_call_params_string(macro_text, self.name).strip()
            self.impl = macro_text.replace(f"#define {self.name}{self.params}", "").strip()
        except ValueError:
            # Index lookup may fail.
            raise HintLoadException()

    def replace_in_caller(self, call_text: str) -> str:
        """ In the 'hint' implementation, the macro placeholder must be
         replaced with the called macro's implementation. Important:
         parameter names may change between call and original macro
         definition so replace them gracefully.

         Find call params string in hint implementation.
         Example: '(...) public: SOME_SUB_MACRO (param1, param2)'
        """

        call_params = StringParser.extract_call_params_string(call_text, self.name)

        if self._has_variadic_params():

            # "#define FOO(...) __VARGS__" -> __VARGS__ must be replaced with parameters
            # "(param1, param2<t>)"  -> "param1, param2<t>"

            # Eliminate enclosing outer brackets, leave potential inner brackets as is.
            assert call_params.startswith("(") and call_params.endswith(")")
            call_params_stripped = call_params[1:-1]

            temp = self.impl
            temp = temp.replace(ParameterizedHint.VARIADIC_ARGS, call_params_stripped)

            replace_str = f"{self.name} {call_params}"
            assert replace_str in call_text

            return call_text.replace(replace_str, temp)

        else:

            # Group parameters into tuples of (renamed, original)
            call_params_list = StringParser.listify_params(call_params)
            self_params_list = StringParser.listify_params(self.params)
            assert len(call_params_list) == len(self_params_list)
            pairs = zip(call_params_list, self_params_list)

            # In the called macro implementation, replace the parameter
            # names with the called parameter names
            temp = self.impl
            for renamed_param, original_param in pairs:
                temp = temp.replace(original_param, renamed_param)

            # Insert the final string into the implementation for
            # 'hint', replacing the nested macro call. The macro
            # may have a whitespace between name and param lists.
            replace_str = f"{self.name} {call_params}"
            if replace_str not in call_text:
                replace_str = f"{self.name}{call_params}"
                assert replace_str in call_text

            return call_text.replace(replace_str, temp)

    def to_export_string(self) -> str:
        return f"#define {self.name}{self.params} {self.impl}"

    def _has_variadic_params(self) -> bool:
        return self.params == "(...)"


class HintExporter:
    """Create hint file from list of parsed macros."""

    def __init__(self, output_file: pathlib.Path) -> None:
        self.output_file = output_file

    def run(self, macros: List[Macro]) -> None:
        """Convert macros into hints, generate output file."""

        hints: List[Hint] = self._create_hints(macros)
        self._replace_cascaded_calls(hints)
        hints_to_export = self._filter_hints(hints)

        logging.info(f"prepared {len(hints_to_export)} hints for export")
        self._write_output_file(hints_to_export)

    @staticmethod
    def _match_any_regex(regex_list: List[re.Pattern], string: str) -> bool:
        """Check if string matches any regexp provided by regex_list.
        Uses search() over match(): search() performs a 'contains-like'
        check.
        """

        for regex in regex_list:
            if re.search(regex, string):
                return True

        return False

    @staticmethod
    def _create_hints(macros: List[Macro]) -> List[Hint]:
        """Loop over macros, create hints for all relevant macros.
        Skips file include guards.
        """

        result: List[Hint] = list()
        for macro in macros:

            if macro.type == MacroType.INCLUDE_GUARD:
                continue

            elif macro.type == MacroType.PARAMETERIZED:
                hint = ParameterizedHint(macro)
                try:
                    hint.load(macro.text)
                    result.append(hint)
                except HintLoadException as e:
                    logging.error(f"could not load hint: {e}")

            elif macro.type == MacroType.NON_PARAMETERIZED:
                hint = NonParameterizedHint(macro)
                try:
                    hint.load(macro.text)
                    result.append(hint)
                except HintLoadException as e:
                    logging.error(f"could not load hint: {e}")

            elif macro.type == MacroType.UNKNOWN:
                logging.warning(f"skipped unknown macro '{macro.text}'")

            else:
                logging.error(f"unexpected macro '{macro.text}'")

        return result

    @staticmethod
    def _used_as_literal(impl: str, macro: str) -> bool:
        """ Attempt to detect whether the occurrence of 'macro'
        is an actual call or not. Counter example from 'debug.h':

        #define ASSERT(e) \
            { if(!(e)) CCL::Debugger::assertFailed (#e, __FILE__, __LINE__); }

        #define SOFT_ASSERT(e,s) \
            { if(!(e)) CCL::Debugger::printf ("ASSERT FAILED: \"%s\"  "#e"\n", s); }

        Here, the macro ASSERT is used by SOFT_ASSERT as a literal, not a call.

        Test strategy: check if enclosed in quotes.
        """

        return StringParser.is_literal_in(string=impl, search_string=macro)

    @staticmethod
    def _used_as_call(impl: str, macro: str) -> bool:
        """Check if 'impl' invokes 'macro'.

        Examples:
            "CCL_TEST (a,b)" uses "CCL_TEST"
            "_CCL_TEST (a,b)" does not use "CCL_TEST"
            "CCL_TEST_ (a,b)" does not use "CCL_TEST"
        """

        return StringParser.contains_token(string=impl, search_string=macro)

    @staticmethod
    def _replace_call_recursive(hint: Hint, known_hint_names: List[str], known_hints: Dict[str, Hint],
                                processed_hints: List[str]) -> None:

        # Already processed, do nothing.
        if hint.name in processed_hints:
            return

        for macro_name in known_hint_names:

            # Do not check self.
            if macro_name == hint.name:
                continue

            if not HintExporter._used_as_call(impl=hint.impl, macro=macro_name):
                continue

            if HintExporter._used_as_literal(impl=hint.impl, macro=macro_name):
                continue

            # Lookup the dependent macro that is called by hint.
            called_hint = known_hints.get(macro_name, None)
            if called_hint is None:
                logging.warning(f"missing called macro '{macro_name}'")
                continue

            if not called_hint.impl:
                path = called_hint.macro.source.path
                logging.warning(f"called macro '{macro_name}' from '{path}' has empty implementation")

            # Same macro can occur multiple times, example: "QUERY_INTERFACE" in "CLASS_INTERFACE2"
            while macro_name in hint.impl:
                # Recursion: called macro may in turn require resolving, resolve first.
                if called_hint.name not in processed_hints:
                    HintExporter._replace_call_recursive(called_hint, known_hint_names, known_hints, processed_hints)

                hint.impl = called_hint.replace_in_caller(hint.impl)

            logging.debug(f"replaced uses of '{macro_name}' in '{hint.name}'")

        # Guard against redundant processing.
        processed_hints.append(hint.name)

    @staticmethod
    def _replace_cascaded_calls(hints: List[Hint]) -> None:
        """Resolve cascading macro implementations."""

        # Create hint-by-name lookup dictionary.
        hints_dict = {hint.name: hint for hint in hints}

        # Check longest names first to not have false positives
        # Example: DEFINE_CLASS could falsely match DEFINE_CLASS_FLAGS
        hint_names_sorted = sorted(hints_dict.keys(), key=len, reverse=True)
        hint_names_sorted = [str(name) for name in hint_names_sorted]

        # Start recursion, setup guard to process each hint only once.
        processed_hints: List[str] = list()
        for hint in hints:
            HintExporter._replace_call_recursive(hint, hint_names_sorted, hints_dict, processed_hints)

    @staticmethod
    def _filter_hints(hints: List[Hint]) -> List[Hint]:
        """Filter hints by hint individual patterns."""

        result: List[Hint] = list()

        for hint in hints:
            hint_name = hint.name

            include_patterns = hint.macro.source.include_patterns
            if include_patterns:
                if not HintExporter._match_any_regex(include_patterns, hint_name):
                    logging.debug(f"skipped macro '{hint_name}' due to include pattern filter")
                    continue

            exclude_patterns = hint.macro.source.exclude_patterns
            if exclude_patterns:
                if HintExporter._match_any_regex(exclude_patterns, hint_name):
                    logging.debug(f"skipped macro '{hint_name}' due to exclude pattern filter")
                    continue

            result.append(hint)

        return result

    def _write_output_file(self, hints: List[Hint]) -> None:

        output_path = self.output_file.parent
        if not output_path.exists():
            pathlib.Path(output_path).mkdir(parents=True, exist_ok=True)

        with self.output_file.open(mode="w+", encoding="utf-8") as f:
            f.write(f"// !!! GENERATED WITH cpphint.py, DO NOT EDIT !!!\n")
            current_filepath = None
            for hint in hints:
                # Expects hints to be grouped by file (i.e. in the original parse order)

                path = hint.macro.source.path
                if path != current_filepath:
                    f.write("\n//////////////////////////////////////////////////////////////////////////\n")
                    f.write(f"// {hint.get_file_display_string()}\n")
                    f.write("//////////////////////////////////////////////////////////////////////////\n\n")
                    current_filepath = path

                f.write(f"{hint.to_export_string()}\n")
                logging.debug(f"wrote hint for '{hint.name}' from source '{path}'")

        logging.info(f"wrote hint file '{self.output_file.resolve()}'")
