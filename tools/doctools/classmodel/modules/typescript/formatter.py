"""Typescript formatter module."""

from typing import List

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Formatter:
    """
    Apply indentation based on scopes.
    """

    SPACING_CHAR = " "  # replace for indentation debugging
    WIDTH = 2  # 2 chars per indentation level

    @staticmethod
    def indent(depth: int) -> str:
        """
        :param depth: indentation level (not width)
        :return: indentation string
        """
        return Formatter.SPACING_CHAR * (depth * Formatter.WIDTH)

    @staticmethod
    def apply_indentation(text: str) -> str:
        """
        Indent scope blocks, indent after "{", stop before "}". Keep "{ }" as
        is, example: 'empty' classes.

        :param text: text to indent
        :returns text with scopes indented in +/- 2 whitespaces
        """
        depth = 0
        result = ""
        for line in text.splitlines():
            if "}" in line and "{" not in line:
                depth -= 1

            result += "%s%s\n" % (Formatter.indent(depth), line)
            if "{" in line and "}" not in line:
                depth += 1
                continue

        return result[:-1]


class PathFormatter:
    """
    Utility functions to process namespaces and canonical class names.

    Reminders:
    - namespaces are provided in "Foo::Bar" notation
    - class types are provided in "Foo.Bar" notation
    - a class "full namespace path" may be a combination of the two
    """

    @staticmethod
    def to_typescript_namespace(path: str):
        """ Convert cpp style namespace to TypeScript namespace path. """
        return path.replace("::", ".")

    @staticmethod
    def to_typescript_guard(path: str):
        """ Convert cpp or TypeScript style namespace to TypeScript guard variable name. """
        return path.replace("::", "_").replace(".", "_")

    @staticmethod
    def get_class_basename(path: str) -> str:
        """ Return class name without namespaces, i.e. A.B.C -> C """
        return path.split(".")[-1]

    @staticmethod
    def tokenize_namespace(path: str) -> List[str]:
        if "." in path:
            return path.split(".")

        return path.split("::")

    @staticmethod
    def build_full_class_namespace(class_name: str, class_namespace: str) -> str:
        """ Create class full namespace from the class name and its type namespace. Note
        that nested classes are named by a canonical name which uses '.' to separate
        tokens, example: ScriptingHost.InterfaceList.

        Example:
            input: class_name = "ScriptingHost.Console", class_namespace = "CCL"
            output: "CCL.ScriptingHost"
        """
        if "." not in class_name:
            return class_namespace

        tokens = []

        # Namespace may be root/empty - don't append.
        if class_namespace:
            tokens.append(class_namespace)

        tokens.extend(class_name.split(".")[:-1])
        return ".".join(tokens)

    @staticmethod
    def build_full_name(class_name: str, class_namespace: str) -> str:
        if not class_namespace:
            return class_name

        return f"{class_namespace}.{class_name}"
