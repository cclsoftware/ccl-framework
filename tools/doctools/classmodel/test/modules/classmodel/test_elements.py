"""Unit tests for classmodel element classes."""

import unittest

from modules.classmodel.elements import TypeElement

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class TypeElementTest(unittest.TestCase):

    def test_matches(self):
        """ Test matches(). """

        type_element = TypeElement("t", "tn")

        # must match by type only
        self.assertEqual(type_element, TypeElement("t", "tn"))
        self.assertNotEqual(type_element, TypeElement("other", "tn"))
        self.assertEqual(type_element, TypeElement("t", "other"))
        self.assertNotEqual(type_element, TypeElement("other", "other"))
