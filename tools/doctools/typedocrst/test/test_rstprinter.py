"""Unit tests for RstPrinter class."""

import unittest

from modules.rstprinter import RstPrinter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class RstPrinterTest(unittest.TestCase):
    """ Test RstPrinter class. """

    def test_rst_tag_h1(self):
        """ Test <h1> tag. """

        expected = "###\nAPI\n###\n\n"
        self.assertEqual(expected, RstPrinter.h1("API", False))
        expected = "###\nApi\n###\n\n"
        self.assertEqual(expected, RstPrinter.h1("API"))


if __name__ == "__main__":
    unittest.main()
