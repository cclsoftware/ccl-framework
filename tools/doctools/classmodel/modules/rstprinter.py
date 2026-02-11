"""RstPrinter module, util functions to generate RST conform tags."""

from typing import List

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class RstPrinter(object):
    """
    Common purpose RST tag generator.
    """

    CODE_BLOCK_LANG_JAVASCRIPT = "javascript"

    class Decorators(object):
        """
        Inner class to provide decorators.
        """

        @classmethod
        def paragraphed(cls, rst_function, *args):
            """
            Annotation: return result from rst_function as paragraph.
            Do nothing if rst_function returns empty string.
            """
            def p(*args):
                text = rst_function(*args)
                if text:
                    return "%s\n\n" % text
                else:
                    return ""

            return p

    @staticmethod
    @Decorators.paragraphed
    def _underline(tag_char, text):
        """
        Add an underline to 'text' of same length as text.
        Used to create RST section headings.

        :param tag_char: character to use for the underline
        :param text: text to underline
        :return: text with an exact underline consisting of tag_char, nothing if 'text' is empty
        """
        if not text:
            return ""

        rst_tag = tag_char * len(text)

        # use title() to uppercase initial letters
        return "%s\n%s" % (text, rst_tag)

    @staticmethod
    @Decorators.paragraphed
    def _double_line(tag_char, text):
        """
        Wrap 'text' in an upper- and underline of same length as text.
        Used for creating RST headings.

        :param tag_char: character to use for upper- and underline
        :param text: text to wrap
        :return: 'text' wrapped in an upper- and underline consisting of tag_char, nothing if 'text' is empty
        """
        if not text or text is None:
            return ""

        rst_tag = tag_char * len(text)
        return "%s\n%s\n%s" % (rst_tag, text, rst_tag)

    @staticmethod
    def h1(text, capitalize=True):
        """ Format 'text' as H1 """
        # Force initial upper cases with title()
        title_str = text if not capitalize else text.title()
        return RstPrinter._double_line("#", title_str)

    @staticmethod
    def h2(text):
        """ Format 'text' as H2 """
        return RstPrinter._double_line("*", text)

    @staticmethod
    def h3(text):
        """ Format 'text' as H3 """
        return RstPrinter._underline("=", text)

    @staticmethod
    def h4(text):
        """ Format 'text' as H4 """
        return RstPrinter._underline("-", text)

    @staticmethod
    @Decorators.paragraphed
    def anchor(text):
        """ Generate an RST anchor for 'text'. """
        return ".. _%s:" % text

    @staticmethod
    def list_element(text):
        """ Print text as a list element (ul). """
        return "* %s" % text

    @staticmethod
    def list(items: List[str]) -> str:
        """ Print multiple items as list elements (ul). """
        out = ""
        for item in items:
            out += f"* {item}\n"

        return out

    @staticmethod
    def bold(text):
        """ Return text as bold. """
        return "**%s**" % text.strip()

    @staticmethod
    def italic(text):
        """ Return text as italic. """
        return "*%s*" % text.strip()

    @staticmethod
    @Decorators.paragraphed
    def code_block(brief, code, language):
        """
        Return brief and code as a code block.

        Note that an "\t%s" % (text) does not indent multi line strings correctly.
        Therefore, the passed in text must be indented with \t line by line.

        :param brief: code brief description, can be empty depending on code snippet source format
        :param code: code block, can contain inline brief depending on code snippet source format
        :param language: a language describing string supported by sphinx (cpp, xml, ...).
        """
        code_text = ""
        for line in code.split('\n'):
            code_text += "\t%s\n" % line

        # old format, just code containing a description as source code comment
        if not brief:
            return ".. code-block:: %s\n\n%s\n" % (language, code_text)
        else:
            # new format, split brief and code
            # avoid line breaks and tabs in brief to not cause malformed rst
            brief_stripped = brief.strip().replace('\n', ' ').replace('\r', '')
            return ".. code-block:: %s\n\t:caption: %s\n\n%s\n" % (language, brief_stripped, code_text)

    @staticmethod
    def hyperlink(text, target):
        """ Return text, target as :ref: link """
        return ":ref:`%s <%s>`" % (text, target)

    @staticmethod
    @Decorators.paragraphed
    def quote(text):
        """ Return text as indented (quote like). """
        return "\t%s" % text

    @staticmethod
    @Decorators.paragraphed
    def paragraph(text, end_with_period=False):
        """
        Create an RST string for a brief section text, as stored in 'text'.
        Provided 'text' can be empty, return nothing in that case.

        TODO: not a plain rst tag due to period appendage, refactor.

        :param text text to print as paragraph
        :param end_with_period for true, adds '.' if missing
        """
        if not text:
            return ""

        adjusted_text = text
        if end_with_period and not text.endswith('.'):
            adjusted_text += '.'

        return "%s" % adjusted_text

    @staticmethod
    @Decorators.paragraphed
    def hr():
        """ Print a horizontal line ('transition marker') """
        return "\n\n-----"

    @staticmethod
    def code_sample(text):
        """ Print text as code sample. """
        return "`%s`" % text

    @staticmethod
    def image_alias(name, file):
        """
        Add an image alias.

        :param name  reference via |name|
        :param file filename of the icon file
        """
        return ".. |%s| image:: %s" % (name, file)

    @staticmethod
    def image_alias_ref(alias_name):
        """
        Create a ref to an image alias
        """
        return "|%s|" % alias_name

    @staticmethod
    def toctree(files, maxdepth, hidden):
        """
        Create a toctree referencing 'files'. Attribute as hidden optionally.

        :param files  files to reference, expected without suffix
        :param hidden  declare toctree as hidden
        """

        result = ".. toctree::\n"
        result += "\t:maxdepth: %d\n" % maxdepth

        if hidden:
            result += "\t:hidden:\n\n"
        else:
            result += "\n"

        for f in files:
            result += "\t%s\n" % f

        # end paragraph
        result += "\n"
        return result

    @staticmethod
    @Decorators.paragraphed
    def include(file):
        """
        Print include statement paragraphed.
        """
        return ".. include:: %s" % file

    @staticmethod
    def replace(link, replacement):
        """
        Create a replace statement:
        .. |link| replace:: replacement
        """

        return ".. |%s| replace:: %s\n" % (link, replacement)

    @staticmethod
    @Decorators.paragraphed
    def comment(text):
        """
        Print comment text.
        """

        return ".. %s" % text

    @staticmethod
    @Decorators.paragraphed
    def latex_clear_page():
        return ".. raw:: latex\n\n\t\\clearpage"
