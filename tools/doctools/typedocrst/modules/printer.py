"""TypeDoc model printer."""

from modules.elements import *
from modules.model import Model

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ShortcutListPrinter:

    @staticmethod
    def _parse_groups_recursively(model: Model, obj, kind_id: TDReflectionKind, kind_string: str,
                                  full_name: str) -> str:
        """
        Create shortcuts for all sub elements recursively. Stops at
        an element that has no more groups (methods, attributes, ...).
        """

        # Check for recursion end, i.e. no more groups contained. Assume
        # a property (method, ...), export as PropertyShortcut, ID based.
        group_objects = obj.get(TDAttribute.GROUPS)
        if group_objects is None:
            if model.is_exported_to_separate_file(kind_id):
                return Shortcut(obj, model.get_namespace(), kind_string, full_name).to_rst() + "\n"
            else:
                return PropertyShortcut(obj, model.get_namespace(), kind_string, full_name).to_rst() + "\n"

        # Object still has subgroups (namespace, class, enumeration). Export as
        # reference shortcut, i.e. assume the object has its own file.
        result = RstPrinter.comment(obj.get(TDAttribute.NAME))
        result += Shortcut(obj, model.get_namespace(), kind_string, full_name).to_rst() + "\n"

        # The groups only contain ID lists, resolve the objects before iterating them.
        for group_object in group_objects:
            children_objects = model.get_object_children(group_object)
            for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
                child_full_name = full_name + "." + child_object.get(TDAttribute.NAME)
                child_kind_string = child_object.get(TDAttribute.KIND_STRING)
                child_kind = child_object.get(TDAttribute.REFLECTION_KIND)
                result += ShortcutListPrinter._parse_groups_recursively(model, child_object, child_kind,
                                                                        child_kind_string,
                                                                        child_full_name)

        return result

    @staticmethod
    def print(model: Model) -> str:

        result = ""

        group_objects = model.root_object.get(TDAttribute.GROUPS)
        for group_object in sorted(group_objects, key=lambda k: k['title']):
            # group children are all top level objects
            children_objects = model.get_object_children(group_object)
            for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
                obj_name = child_object.get(TDAttribute.NAME)
                obj_kind_string = child_object.get(TDAttribute.KIND_STRING)
                obj_kind_id = child_object.get(TDAttribute.REFLECTION_KIND)
                result += ShortcutListPrinter._parse_groups_recursively(model, child_object, obj_kind_id,
                                                                        obj_kind_string, obj_name)

        return result
