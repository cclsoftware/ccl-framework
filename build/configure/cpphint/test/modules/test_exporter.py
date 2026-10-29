"""CppHint exporter module unit tests.
"""

import pathlib
import re
import unittest

from modules.exporter import HintExporter, ParameterizedHint, NonParameterizedHint, Hint
from modules.shared import Macro, MacroType, Source

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


def make_hint(text: str, path: str = "file.h", exclude_patterns=None) -> Hint:
    source = Source(path=pathlib.Path(path), comment="", include_patterns=[],
                    exclude_patterns=[re.compile(p) for p in (exclude_patterns or [])])
    macro_type = MacroType.PARAMETERIZED if "(" in text.split(" ")[1] else MacroType.NON_PARAMETERIZED
    macro = Macro(text=text, source=source, type=macro_type)
    hint = ParameterizedHint(macro) if macro_type == MacroType.PARAMETERIZED else NonParameterizedHint(macro)
    hint.load(text)
    return hint


def expand(*texts: str) -> dict:
    hints = [make_hint(text) for text in texts]
    HintExporter._replace_cascaded_calls(hints)
    return {hint.name: hint.impl for hint in hints}


class StringizeTest(unittest.TestCase):

    def test_stringize_caller_param(self):
        self.assertEqual("#condition", ParameterizedHint._stringize("condition", ["condition"]))

    def test_stringize_text(self):
        self.assertEqual('"FOO"', ParameterizedHint._stringize("FOO", ["condition"]))

    def test_stringize_mixed(self):
        self.assertEqual('#Class "<" #Type ">"', ParameterizedHint._stringize("Class<Type>", ["Class", "Type"]))

    def test_stringize_token_pasting(self):
        self.assertEqual('#Class "Register"', ParameterizedHint._stringize("Class##Register", ["Class"]))


class ExpansionTest(unittest.TestCase):

    def test_stringized_argument(self):
        """'#name' in called macro, argument is not a caller parameter."""
        impls = expand("#define INNER(e, name) check (e, #name)",
                       "#define OUTER(e) INNER (e, OUTER)")
        self.assertEqual('check (e, "OUTER")', impls["OUTER"])

    def test_stringized_caller_param(self):
        """'#name' in called macro, argument is a caller parameter."""
        impls = expand("#define INNER(e) check (#e)",
                       "#define OUTER(x) INNER (x)")
        self.assertEqual("check (#x)", impls["OUTER"])

    def test_token_pasting_untouched(self):
        """'##name' is not stringization."""
        impls = expand("#define INNER(name) int prefix##name;",
                       "#define OUTER(x) INNER (x)")
        self.assertEqual("int prefix##x;", impls["OUTER"])

    def test_call_next_to_literal(self):
        """A literal mention does not prevent expanding a real call."""
        impls = expand("#define INNER(a) a",
                       '#define OUTER(x) "INNER (x)" INNER (x)')
        self.assertEqual('"INNER (x)" x', impls["OUTER"])


class DuplicatesTest(unittest.TestCase):

    def test_keep_first(self):
        hints = [make_hint("#define ASSERT(e) check (e)"), make_hint("#define ASSERT(e) {}")]
        result = HintExporter._remove_duplicates(hints)
        self.assertEqual(1, len(result))
        self.assertEqual("check (e)", result[0].impl)

    def test_prefer_exported(self):
        hints = [make_hint("#define FOO 1", "a.h", exclude_patterns=["^FOO$"]), make_hint("#define FOO 2", "b.h")]
        result = HintExporter._remove_duplicates(hints)
        self.assertEqual(1, len(result))
        self.assertEqual("2", result[0].impl)


if __name__ == '__main__':
    unittest.main()
