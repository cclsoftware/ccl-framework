"""TypeDoc util."""

from modules.typedocjson import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class TypeDocUtil:

    @staticmethod
    def cleanup_comment_text(text: str) -> str:
        """
        Replace \r with \n, remove surrounding whitespaces.
        Required for RstPrinter.code_block to return the correct output.
        """
        result = text.replace("*", "")
        result = result.replace("\r", "\n").strip()

        return result

    @staticmethod
    def cleanup_comment_code(text: str) -> str:
        """Strip enclosing ''' from a code snippet."""

        code_string = text.replace("```", "")
        return TypeDocUtil.cleanup_comment_text(code_string)

    @staticmethod
    def get_object_name_safe(obj: JSONObject) -> str:
        """
        Get object name, raise ValueError if name is not available.
        Used in context of hyperlinks. Hyperlinks require an object name
        as part of the ref target UID.

        This could potentially fail when setting a link to a group
        which has a title attribute over name.
        """
        obj_name = obj.get(TDAttribute.NAME, "")
        if not obj_name:
            raise ValueError

        return obj_name


class UID:
    """ Object UID format for use in hyperlinks as unique ref target. """

    @staticmethod
    def create(obj: JSONObject, file_prefix: str) -> str:
        # use name, some elements have a title instead
        return "%s-%s" % (file_prefix, obj.get(TDAttribute.ID))


class DetailsFileReference:
    """
    Anchor for use with the top level 'details' file, i.e. the reference is not
    used in the same file but points at another.
    """
    @staticmethod
    def create(obj: JSONObject, file_prefix: str) -> str:
        return "%s-%s" % ("details", UID.create(obj, file_prefix))
