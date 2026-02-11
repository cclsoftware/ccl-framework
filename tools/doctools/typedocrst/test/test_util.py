"""Unit tests for typedocrst.py converter script."""

import unittest

from modules.util import TypeDocUtil

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class TypeDocUtilTest(unittest.TestCase):
    """
    Test ParserUtil class from typedocrst.py.
    """

    def test_cleanup_comment_code(self):
        """ Parse code example with incomplete tags returns None. """

        example = "```\rlet variable=123;\rlet variable2=234;\r```*"
        result = TypeDocUtil.cleanup_comment_code(example)
        self.assertEqual(result, "let variable=123;\nlet variable2=234;")


if __name__ == "__main__":
    unittest.main()
