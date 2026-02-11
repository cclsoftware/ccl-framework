"""ElementListFile module."""

import logging
import os
from typing import List

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ElementListFile:
    """
    List of model elements constructed from a text file. Entries are of form "type.name"
    per element, one entry per row. The file format supported comments and empty rows.
    The file entries are imported in lower case.

    Example:

    # Some Comment
    class.Foo
    object.Bar
    enum.FooBar

    """

    elements = []  # model elements list, lower case! (class.foo, enum.bar, ...)
    title = ""

    def __init__(self, source_file: str = "") -> None:
        self.elements = ElementListFile._read_file(source_file)

    @staticmethod
    def _read_file(source_file: str) -> List[str]:
        """ Build 'elements' list from source_file. """

        result = []

        # default construction
        if not source_file:
            return result

        # no file found
        if not os.path.isfile(source_file):
            return result

        # file found, build list
        logging.info("using export list file %s" % source_file)
        with open(source_file) as fp:
            for line in fp:
                # support comments
                if line.startswith("#"):
                    continue

                # use lower case, stripped
                result.append(line.strip().lower())

        # potentially not intended: export list file is empty, causing all elements to be discarded
        if not result:
            logging.warning("found empty export list file %s" % source_file)

        return result

    def contains(self, element_type: str, element_name: str) -> bool:
        """ Check if an element is contained. Checks for "type.name" as lower-case. """
        if len(self.elements) == 0:
            return False

        lookup_name = str(element_type + "." + element_name).lower()
        if lookup_name in self.elements:
            return True

        return False


class ElementFilter:
    """Manage element inclusion/exclusion."""

    def __init__(self, includes: ElementListFile = None, excludes: ElementListFile = None) -> None:
        """ Construct for given, optional include/exclude list. """
        self.includes = includes
        self.excludes = excludes

    def skip(self, element_type: str, element_name: str) -> bool:
        """ Check if type/name combination should be discarded."""
        if self.includes is not None:
            if not self.includes.contains(element_type, element_name):
                return True

        if self.excludes is not None:
            if self.excludes.contains(element_type, element_name):
                return True

        return False
