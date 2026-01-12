"""CppHint stringutil module unit tests.
"""

import unittest

from modules.stringutil import StringParser

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class StringParserTest(unittest.TestCase):

    def test_extract_call_params_string_single(self):
        """Single argument."""
        params_str = StringParser.extract_call_params_string("FOO (a)", "FOO")
        self.assertEqual("(a)", params_str)

    def test_extract_call_params_string_multiple(self):
        """List of arguments."""
        params_str = StringParser.extract_call_params_string("FOO (a,b,c)", "FOO")
        self.assertEqual("(a,b,c)", params_str)

        params_str = StringParser.extract_call_params_string("FOO(a,b,c)", "FOO")
        self.assertEqual("(a,b,c)", params_str)

    def test_extract_call_params_string_macro(self):
        """Arguments call another macro."""
        params_str = StringParser.extract_call_params_string("FOO (BAR (a, b), c)", "FOO")
        self.assertEqual("(BAR (a, b), c)", params_str)

        params_str = StringParser.extract_call_params_string("FOO (a, BAR (b, c))", "FOO")
        self.assertEqual("(a, BAR (b, c))", params_str)

    def test_listify_params_single(self):
        """Convert single param to list."""
        params_list = StringParser.listify_params("(a)")
        self.assertEqual(["a"], params_list)

    def test_listify_params_multiple(self):
        """Convert multiple params to param list."""
        params_list = StringParser.listify_params("(a,b,c)")
        self.assertEqual(["a", "b", "c"], params_list)

        params_list = StringParser.listify_params("(a, b, c)")
        self.assertEqual(["a", "b", "c"], params_list)

    def test_listify_params_template(self):
        """Convert params with template args to param list."""
        params_list = StringParser.listify_params("(a<T1, T2>,b,c)")
        self.assertEqual(["a<T1, T2>", "b", "c"], params_list)

    def test_listify_params_string_concat(self):
        """Convert params that include strings."""
        params_list = StringParser.listify_params("(a,#b \", \" #c)")
        self.assertEqual(["a", "#b \", \" #c"], params_list)

        params_list = StringParser.listify_params("(#a \", \" #b, c)")
        self.assertEqual(["#a \", \" #b", "c"], params_list)

    def test_listify_params_macro(self):
        """Convert params with macro call to param list."""
        params_list = StringParser.listify_params("(FOO (a), b, c)")
        self.assertEqual(["FOO (a)", "b", "c"], params_list)

        params_list = StringParser.listify_params("(a,FOO (b),c)")
        self.assertEqual(["a", "FOO (b)", "c"], params_list)

        params_list = StringParser.listify_params("(a, b, FOO (c))")
        self.assertEqual(["a", "b", "FOO (c)"], params_list)

    def test_listify_params_operator(self):
        """Convert params with expression call to param list."""

        params_list = StringParser.listify_params("(x > 0)")
        self.assertEqual(["x > 0"], params_list)

        params_list = StringParser.listify_params("(x < 0)")
        self.assertEqual(["x < 0"], params_list)

        params_list = StringParser.listify_params("(x >= 0)")
        self.assertEqual(["x >= 0"], params_list)

        params_list = StringParser.listify_params("(x <= 0)")
        self.assertEqual(["x <= 0"], params_list)

    def test_listify_params_smoke_test(self):

        params_list = StringParser.listify_params(
            "((act > (exp - delta) && act < (exp + delta)), a<T1, T2>, ASSERT_NEAR, #exp \", \" #act \", \" #delta)")
        self.assertEqual([
            "(act > (exp - delta) && act < (exp + delta))",
            "a<T1, T2>",
            "ASSERT_NEAR",
            "#exp \", \" #act \", \" #delta"
        ], params_list)

    def test_is_literal_in(self):
        test_string = 'this is a "test" string'
        self.assertTrue(StringParser.is_literal_in(test_string, "test"))

        test_string = 'this is a "another test" string'
        self.assertTrue(StringParser.is_literal_in(test_string, "test"))

        test_string = 'this is a "another" test string'
        self.assertFalse(StringParser.is_literal_in(test_string, "test"))

        # Special case: must count quotes to detect quote pairs left and right
        # Search string test itself is not enclosed.
        test_string = 'this is a "another" test "failing" string'
        self.assertFalse(StringParser.is_literal_in(test_string, "test"))

        test_string = 'printf("ASSERT FAILED: \"%s\"\"e\"\n\")'
        self.assertTrue(StringParser.is_literal_in(test_string, "ASSERT"))

    def test_contains_token(self):
        test_string = 'FOO (a,b)'
        self.assertTrue(StringParser.contains_token(test_string, "FOO"))

        test_string = 'FOO(a,b)'
        self.assertTrue(StringParser.contains_token(test_string, "FOO"))

        test_string = '_FOO (a,b)'
        self.assertFalse(StringParser.contains_token(test_string, "FOO"))

        test_string = '_FOO(a,b)'
        self.assertFalse(StringParser.contains_token(test_string, "FOO"))

        test_string = 'FOO_ (a,b)'
        self.assertFalse(StringParser.contains_token(test_string, "FOO"))

        test_string = 'FOO_(a,b)'
        self.assertFalse(StringParser.contains_token(test_string, "FOO"))

        test_string = "BAR _FOO (a,b)"
        self.assertFalse(StringParser.contains_token(test_string, "FOO"))

    def test_extract_name(self):
        """Extract macro name from macro text."""

        test_string = "#define FOO BAR"
        self.assertEqual("FOO", StringParser.extract_macro_name(test_string))

        test_string = "#define FOO(a, b, c) abc(def)"
        self.assertEqual("FOO", StringParser.extract_macro_name(test_string))

    def test_extract_parameterized_name(self):
        """Extract macro name with parameters from macro text."""

        # Parameters, no implementation.
        test_string = "#define FOO(a, b, c)"
        self.assertEqual("FOO(a, b, c)", StringParser.extract_parameterized_name(test_string))

        # Parameters, no implementation - no whitespaces between args.
        test_string = "#define FOO(a,b,c)"
        self.assertEqual("FOO(a,b,c)", StringParser.extract_parameterized_name(test_string))

        # Parameters, implementation contains another set of "()"
        test_string = "#define FOO(a, b, c) if(123)"
        self.assertEqual("FOO(a, b, c)", StringParser.extract_parameterized_name(test_string))

        # Parameters, implementation contains another set of "()" - no whitespaces between args
        test_string = "#define FOO(a,b,c) if(123)"
        self.assertEqual("FOO(a,b,c)", StringParser.extract_parameterized_name(test_string))

        # No parameters
        test_string = "#define FOO if(123)"
        self.assertEqual("FOO", StringParser.extract_parameterized_name(test_string))

    def test_remove_trailing_comment(self):
        """Strip trailing comments."""

        test_string = "#define FOO(a,b) // some comment"
        self.assertEqual("#define FOO(a,b)", StringParser.remove_trailing_comment(test_string))

        test_string = "#define FOO(a,b) ///< some doxygen comment"
        self.assertEqual("#define FOO(a,b)", StringParser.remove_trailing_comment(test_string))


if __name__ == '__main__':
    unittest.main()
