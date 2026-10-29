"""CppHint stringutil module.
"""

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"

import re
from typing import List


class ParseConditionException(Exception):
    pass


class StringParser:

    @staticmethod
    def extract_call_params_string(search_string: str, macro_name: str) -> str:
        """Extract called parameters for macro_name.

        Examples:

            "FOO (a, b)" --> "(a, b)"
            "FOO (BAR (a, b), c) -> "(BAR (a, b), c)"
        """

        matches = StringParser._find_tokens(search_string, macro_name)
        assert matches
        part = search_string[matches[0].start():]

        bracket_count = 0
        result = ""
        for c in part:

            # Skip everything outside brackets.
            if c not in ["(", ")"]:
                if bracket_count == 0:
                    continue
                else:
                    result += c
                    continue

            # Increment bracket stack with each opening bracket.
            if c == "(":
                result += c
                bracket_count = bracket_count + 1
                continue

            # Decrement bracket stack or finalize result for closing bracket
            if c == ")":
                assert bracket_count > 0
                result += c
                bracket_count = bracket_count - 1
                if bracket_count == 0:
                    break
                else:
                    continue

        return result

    @staticmethod
    def extract_macro_name(string: str) -> str:
        """Extract macro name excluding parameters.

        Example:
            '#define FOO(a,b,c)' -> 'FOO'
        """

        result = ""
        stripped = string.replace("#define", "").strip()
        for c in stripped:
            if c in ["(", " "]:
                break
            else:
                result += c

        return result

    @staticmethod
    def extract_parameterized_name(string: str) -> str:
        """Extract macro name including parameters. Reminder: split ()
        can not be used with whitespace delimiter due to potential
        whitespaces in the parameter list.

        Example:
        '#define FOO(a, b, c) code' -> 'FOO(a, b, c)'
        """

        result = ""
        stripped = string.replace("#define", "").strip()
        in_bracket = False
        for c in stripped:

            if c == " " and not in_bracket:
                break

            elif c == "(":
                assert not in_bracket
                in_bracket = True
                result += c

            elif c == ")":
                assert in_bracket
                in_bracket = False
                result += c

            else:
                result += c

        return result

    @staticmethod
    def listify_params(params_string: str) -> List[str]:
        """Convert parameter string to list of individual parameters.

        Examples:
            "(param1, param2, param3)" -> ["param1", "param2", "param3"]
            "(param1, param2<t1,t2>)" -> ["param1", "param2<t1,t2>"]
            "(FOO (A, B), param2)" -> ["FOO (A, B), "param2"]
            "(FOO, #str1 ", " #str2)" -> ["FOO", "#str1 \", \" #str2"]
        """

        string_concat_pattern = '", "'
        string_concat_placeholder = "|"

        result = list()
        if not params_string:
            return result

        # Expect string must have enclosing parentheses, remove them.
        valid_string = params_string.startswith("(") and params_string.endswith(")")
        assert valid_string
        if not valid_string:
            return result

        params_string = params_string[1:-1]

        # Preprocess input string. Remove ", " which may appear in string
        # concatenation patterns to keep the parser code below a bit simpler.
        params_string = params_string.replace(string_concat_pattern, string_concat_placeholder)

        template_count = 0
        parentheses_count = 0
        param_token = ""

        for position, c in enumerate(params_string):
            previous_c = params_string[position - 1] if position > 0 else None

            if c == "(":
                parentheses_count = parentheses_count + 1
                param_token += c
                continue

            if c == ")":
                parentheses_count = parentheses_count - 1
                param_token += c
                continue

            # Check if operator or template bracket.
            if c == "<" and not previous_c == " ":
                param_token += c
                template_count = template_count + 1
                continue

            # Check if operator or template bracket.
            if c == ">" and not previous_c == " ":
                assert template_count > 0
                param_token += c
                template_count = template_count - 1
                continue

            if c != ",":
                param_token += c
                continue

            if c == ",":
                if template_count > 0 or parentheses_count > 0:
                    param_token += c
                    continue
                else:
                    result.append(param_token)
                    param_token = ""
                    continue

        # Append remaining token
        if param_token:
            result.append(param_token)

        # Cleanup tokens, revert postprocessing
        for i, token in enumerate(result):
            result[i] = token.strip().replace(string_concat_placeholder, string_concat_pattern)

        return result

    @staticmethod
    def parse_condition_name(def_str: str) -> str:
        """Extract condition variable name.

        Examples:
            "#ifdef FOO" -> "FOO"
            "#elif BAR" -> "BAR"

        This is vastly simplified as there can be more complex expressions
        like "#ifdef FOO==[value]" etc.

        Raises:
            ParseConditionException for more complex condition statements
        """

        tokens = def_str.split(" ")
        if len(tokens) != 2:
            raise ParseConditionException("unsupported condition statement")

        return tokens[1]

    @staticmethod
    def is_literal_in(string: str, search_string: str) -> bool:
        """Check if 'search_string' is contained in 'string'
        and enclosed by quotes, hinting at literal value use.

        Examples:
            'some test string' -> 'test' not enclosed
            'some " fancy test" string' -> 'test' enclosed
            'some "fancy" test "case" string' -> 'test' not enclosed (but has quote pairs left and right)
        """

        # Find all positions of double quotes in the string.
        quote_positions = [pos for pos, char in enumerate(string) if char == '"']

        # Iterate through the positions of quotes in pairs (start and end of quoted segments).
        for i in range(0, len(quote_positions), 2):
            start_quote = quote_positions[i]
            if i + 1 < len(quote_positions):
                end_quote = quote_positions[i + 1]

                # Check if the search string is within the current quoted segment.
                if search_string in string[start_quote:end_quote]:
                    return True

        return False

    @staticmethod
    def _token_pattern(search_string: str) -> str:
        """Regular expression matching 'search_string' not preceded by an
        identifier character, and, if it ends with an identifier character,
        not followed by one either.
        """

        pattern = r"(?<![A-Za-z0-9_])" + re.escape(search_string)
        if re.match(r"[A-Za-z0-9_]", search_string[-1:]):
            pattern += r"(?![A-Za-z0-9_])"
        return pattern

    @staticmethod
    def _mask_literals(string: str) -> str:
        """Blank out the contents of string and character literals,
        keeping all character positions intact.

        Example:
            'FOO ("BAR") BAR' -> 'FOO ("   ") BAR'
        """

        def mask(match: re.Match) -> str:
            literal = match.group(0)
            return literal[0] + " " * (len(literal) - 2) + literal[-1]

        return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', mask, string)

    @staticmethod
    def _find_tokens(string: str, search_string: str) -> List[re.Match]:
        """Find non-overlapping whole-word occurrences of 'search_string' in
        'string'. The leading identifier of 'search_string' must not be part
        of a string literal, while the remainder (e.g. call parameters) may
        contain literals.
        """

        leading_identifier = re.match(r"[A-Za-z0-9_]*", search_string).group(0) or search_string
        masked = StringParser._mask_literals(string)
        full_pattern = re.compile(StringParser._token_pattern(search_string))

        result: List[re.Match] = list()
        end = 0
        for candidate in re.finditer(StringParser._token_pattern(leading_identifier), masked):
            if candidate.start() < end:
                continue

            match = full_pattern.match(string, candidate.start())
            if match:
                result.append(match)
                end = match.end()

        return result

    @staticmethod
    def contains_token(string: str, search_string: str) -> bool:
        """Check if 'string' contains 'search_string' as a whole word,
        ignoring occurrences inside string literals.

        Examples:
            "FOO(a,b)" -> "FOO" is contained
            "FOO_(a,b,c)" -> "FOO" is not contained
            "BAR _FOO (a,b,c)" -> "FOO" is not contained
            "BAR_FOO (a,b,c)" -> "FOO" is not contained
            "_FOO (a) FOO (b)" -> "FOO" is contained
            'printf ("FOO failed")' -> "FOO" is not contained
        """

        return len(StringParser._find_tokens(string, search_string)) > 0

    @staticmethod
    def replace_token(string: str, search_string: str, replacement: str) -> str:
        """Replace whole-word occurrences of 'search_string' in 'string',
        leaving occurrences inside string literals untouched.

        Example:
            ("FOO (a) BAR_FOO (a)", "FOO (a)", "x") -> "x BAR_FOO (a)"
        """

        result = ""
        position = 0
        for match in StringParser._find_tokens(string, search_string):
            result += string[position:match.start()] + replacement
            position = match.end()

        return result + string[position:]

    @staticmethod
    def remove_trailing_comment(string: str) -> str:
        """ Strip trailing comment from string. Expects comment to start
        with '//'. Works for regular '//' and doxygen style '///<' comments.
        """

        index = string.find("//")
        if index > -1:
            return string[:index].strip()

        return string

