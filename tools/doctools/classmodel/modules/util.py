"""Utility classes module."""

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class EncodingHelper(object):
    """ Encoding helper class. """

    WINDOWS_LINE_ENDING = "\r\n"
    UNIX_LINE_ENDING = "\n"

    @staticmethod
    def replace_crlf(text):
        """
         Replaces CRLF (ASCII #13#10) with LF (ASCII #10) in 'text' to fix an issue with
         superfluous new lines when parsing classmodel files that originate from a windows host
        """
        return text.replace(EncodingHelper.WINDOWS_LINE_ENDING, EncodingHelper.UNIX_LINE_ENDING)


class ElementUID(object):
    """
    Helper class to generate an element UID for use in RST :ref: directives.
    """

    @staticmethod
    def create(prefix, element_type, name):
        """
        Generate UID string from file name, element type and name.
        """
        return "%s-%s-%s" % (prefix, element_type, name)


class ElementUtil:

    @staticmethod
    def enum_name(class_name, enum_name):
        """ Create 'Class.Enum' string. """
        return "%s.%s" % (class_name, enum_name)
