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
    def replace_in_caller(self, call_text: str, caller_params: List[str]) -> str:
        """Replace call of this hint in call_text. 'caller_params' are the
        parameter names of the calling macro."""
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

    def replace_in_caller(self, call_text: str, caller_params: List[str]) -> str:
        """Replace call of this macro with the macro implementation.
        Since there are no parameters, simply replace the name with
        the implementation.
        """

        return StringParser.replace_token(call_text, self.name, self.impl)

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

    def replace_in_caller(self, call_text: str, caller_params: List[str]) -> str:
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

            return self._replace_call(call_text, call_params, temp)

        else:

            # Map original parameter names to called parameters.
            call_params_list = StringParser.listify_params(call_params)
            self_params_list = StringParser.listify_params(self.params)
            assert len(call_params_list) == len(self_params_list)
            params_map = dict(zip(self_params_list, call_params_list))

            # In the called macro implementation, replace the parameter names
            # with the called parameters. Match whole words only (e.g. parameter
            # 'a' must not match inside 'typename') and in a single pass so that
            # replaced text is not substituted again. A stringized parameter
            # ('#param', but not '##param') is matched including its '#'.
            temp = self.impl
            if params_map:
                pattern = (r"(?P<stringize>(?<!#)#(?!#)\s*)?(?<![A-Za-z0-9_])(?P<param>"
                           + "|".join(re.escape(param) for param in params_map)
                           + r")(?![A-Za-z0-9_])")

                def substitute(match: re.Match) -> str:
                    argument = params_map[match.group("param")]
                    if match.group("stringize"):
                        return ParameterizedHint._stringize(argument, caller_params)
                    return argument

                temp = re.sub(pattern, substitute, temp)

            return self._replace_call(call_text, call_params, temp)

    @staticmethod
    def _stringize(argument: str, caller_params: List[str]) -> str:
        """Apply the '#' operator to a call argument.

        Parameters of the calling macro within the argument must stay
        stringized parameters of the caller, the remaining text becomes
        string literals. Adjacent literals are concatenated by the compiler.
        Token pasting ('##') within the argument is applied.

        Examples:
            'condition' (caller parameter) -> '#condition'
            'CCL_TEST_ASSERT' -> '"CCL_TEST_ASSERT"'
            'Class<Type>' (caller parameters) -> '#Class "<" #Type ">"'
            'Class##Register' (caller parameter) -> '#Class "Register"'
        """

        def quote(text: str) -> str:
            escaped = text.replace("\\", "\\\\").replace('"', '\\"')
            return f'"{escaped}"'

        parts: List[str] = list()
        literal = ""
        for token in re.split(r"([A-Za-z_][A-Za-z0-9_]*)", argument):
            if token in caller_params:
                if literal:
                    parts.append(quote(literal))
                    literal = ""
                parts.append(f"#{token}")
            else:
                literal += re.sub(r"\s*##\s*", "", token)

        if literal or not parts:
            parts.append(quote(literal))

        return " ".join(parts)

    def _replace_call(self, call_text: str, call_params: str, expanded: str) -> str:
        """Insert the expanded implementation into the implementation for
        'hint', replacing the nested macro call. The macro may have a
        whitespace between name and param lists.
        """

        replace_str = f"{self.name} {call_params}"
        if not StringParser.contains_token(call_text, replace_str):
            replace_str = f"{self.name}{call_params}"
            assert StringParser.contains_token(call_text, replace_str)

        return StringParser.replace_token(call_text, replace_str, expanded)

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
        hints = self._remove_duplicates(hints)
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
    def _used_as_call(impl: str, macro: str) -> bool:
        """Check if 'impl' invokes 'macro'. Occurrences in string literals
        are not calls, counter example from 'debug.h':

        #define SOFT_ASSERT(e,s) \
            { if(!(e)) CCL::Debugger::printf ("ASSERT FAILED: \"%s\"  "#e"\n", s); }

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

        caller_params: List[str] = list()
        if isinstance(hint, ParameterizedHint):
            caller_params = StringParser.listify_params(hint.params)

        for macro_name in known_hint_names:

            # Do not check self.
            if macro_name == hint.name:
                continue

            if not HintExporter._used_as_call(impl=hint.impl, macro=macro_name):
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
            while StringParser.contains_token(hint.impl, macro_name):
                # Recursion: called macro may in turn require resolving, resolve first.
                if called_hint.name not in processed_hints:
                    HintExporter._replace_call_recursive(called_hint, known_hint_names, known_hints, processed_hints)

                hint.impl = called_hint.replace_in_caller(hint.impl, caller_params)

            logging.debug(f"replaced uses of '{macro_name}' in '{hint.name}'")

        # Guard against redundant processing.
        processed_hints.append(hint.name)

    @staticmethod
    def _resolve_aliases(hints: List[Hint]) -> None:
        """Convert non-parameterized aliases of parameterized macros
        into parameterized hints.

        Example:
            '#define FOO_BOOL FOO_INT' with '#define FOO_INT(a, b) impl...'
            -> '#define FOO_BOOL(a, b) FOO_INT(a, b)'
        """

        hints_dict = {hint.name: hint for hint in hints}
        for index, hint in enumerate(hints):
            if not isinstance(hint, NonParameterizedHint):
                continue

            target = hints_dict.get(hint.impl, None)
            if not isinstance(target, ParameterizedHint):
                continue

            alias = ParameterizedHint(hint.macro)
            alias.name = hint.name
            alias.params = target.params
            alias.impl = f"{target.name}{target.params}"
            hints[index] = alias
            logging.debug(f"resolved alias '{hint.name}' of '{target.name}'")

    @staticmethod
    def _replace_cascaded_calls(hints: List[Hint]) -> None:
        """Resolve cascading macro implementations."""

        HintExporter._resolve_aliases(hints)

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
    def _remove_duplicates(hints: List[Hint]) -> List[Hint]:
        """Keep a single definition per macro name. Duplicates typically stem
        from '#if/#else' branches (e.g. debug vs. release variants) which are
        all scanned unless a condition is configured.

        The first definition that passes the pattern filters is kept, which is
        usually the '#if' branch. Definitions excluded by pattern filters are
        only kept if there is no other definition.
        """

        selected: Dict[str, Hint] = dict()
        for hint in hints:
            existing = selected.get(hint.name, None)
            if existing is None:
                selected[hint.name] = hint
                continue

            kept = existing
            existing_exported = HintExporter._is_exported(existing)
            hint_exported = HintExporter._is_exported(hint)
            if not existing_exported and hint_exported:
                kept = hint
                selected[hint.name] = hint

            # Only definitions exported from different files are a real conflict,
            # others typically are '#if/#else' branches of the same file.
            message = (f"duplicate macro '{hint.name}' in '{hint.macro.source.path}' and "
                       f"'{existing.macro.source.path}', keeping definition from '{kept.macro.source.path}'")
            if existing_exported and hint_exported and hint.macro.source.path != existing.macro.source.path:
                logging.warning(message)
            else:
                logging.debug(message)

        return [hint for hint in hints if selected[hint.name] is hint]

    @staticmethod
    def _is_exported(hint: Hint) -> bool:
        """Check hint against the include and exclude patterns of its source."""

        include_patterns = hint.macro.source.include_patterns
        if include_patterns:
            if not HintExporter._match_any_regex(include_patterns, hint.name):
                logging.debug(f"skipped macro '{hint.name}' due to include pattern filter")
                return False

        exclude_patterns = hint.macro.source.exclude_patterns
        if exclude_patterns:
            if HintExporter._match_any_regex(exclude_patterns, hint.name):
                logging.debug(f"skipped macro '{hint.name}' due to exclude pattern filter")
                return False

        return True

    @staticmethod
    def _filter_hints(hints: List[Hint]) -> List[Hint]:
        """Filter hints by hint individual patterns."""

        return [hint for hint in hints if HintExporter._is_exported(hint)]

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
