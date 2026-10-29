"""ClassModel class module."""

import logging
from typing import Dict, List, Optional

from modules.classmodel.elements import ClassElement, EnumerationElement, ObjectElement

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


class Model:
    """ Store all data read from XML, enriched with cross-referenced domain facts
    (resolved class/enum inheritance) that hold regardless of output format. """

    def __init__(self):
        self._root_name = ""
        self._class_hierarchy: Dict[str, str] = dict()
        self._classes = dict()
        self._enumerations = dict()
        self._objects = dict()

    @property
    def classes(self) -> Dict[str, ClassElement]:
        return self._classes

    @classes.setter
    def classes(self, classes: Dict[str, ClassElement]) -> None:
        self._classes = classes

    def lookup_class(self, bare_name: str, scope: str = "") -> Optional[ClassElement]:
        """
        Resolve a bare (non-namespace-qualified) class name to a single registered class.

        Classes are registered by namespace-qualified identity (see
        ClassElement.qualified_name), because classmodel files may legitimately reuse the
        same bare name across different namespaces. Some classmodel data only ever provides
        a bare name with no namespace information at all (member/argument/return type names,
        "\\see" documentation links), so this indirection resolves such a reference: a class
        registered in 'scope' is preferred, and if none matches and more than one class
        shares the bare name, the choice is inherently ambiguous from the source data, so the
        first (by registration order) is used and a warning is logged.
        """

        if not bare_name:
            return None

        candidates = [c for c in self._classes.values() if c.name == bare_name]
        if not candidates:
            return None

        if scope:
            for c in candidates:
                if c.namespace == scope:
                    return c

        if len(candidates) > 1:
            logging.warning(
                "ambiguous class reference '%s': %d candidates (%s), using '%s'"
                % (bare_name, len(candidates), ", ".join(c.qualified_name for c in candidates),
                   candidates[0].qualified_name))

        return candidates[0]

    @property
    def objects(self) -> Dict[str, ObjectElement]:
        return self._objects

    @objects.setter
    def objects(self, objects: Dict[str, ObjectElement]) -> None:
        self._objects = objects

    @property
    def enumerations(self) -> Dict[str, EnumerationElement]:
        return self._enumerations

    @enumerations.setter
    def enumerations(self, enumerations: Dict[str, EnumerationElement]) -> None:
        self._enumerations = enumerations

    @property
    def root_name(self) -> str:
        return self._root_name

    @root_name.setter
    def root_name(self, root_name) -> None:
        self._root_name = root_name

    @property
    def class_hierarchy(self) -> Dict[str, str]:
        return self._class_hierarchy

    @class_hierarchy.setter
    def class_hierarchy(self, hierarchy: Dict[str, str]):
        self._class_hierarchy = hierarchy

    def post_process(self):
        """
        Resolve cross-referenced domain data: class inheritance (parents/children), schema/child
        group inheritance, and enumeration inheritance. Output-format-specific enrichment (RST
        link targets, grouped/formatted views, ...) is built separately - see modules.restructuredtext.builder.
        """

        # Resolve parents and children. Important: must be first step as
        # the other enrich steps involve the class hierarchy.
        for class_name, class_element in self._classes.items():
            class_element.parents = self._determine_class_parents(class_element)
            class_element.schema_groups = self._determine_class_schema_groups(class_element)
            class_element.child_group = self._determine_class_child_group(class_element)
            class_element.children = self._determine_class_children_names(class_name)

        self._determine_enum_parents(self._classes, self._enumerations)

    def _determine_class_parents(self, class_element: ClassElement) -> List[ClassElement]:
        """
        Recursively determine all parent classes of 'parent_name'.
        :param: parent_name  name of first parent to lookup
        :param: classes_dict  tuples of {class name:parent name}
        :return: list of parents
        """

        result: List[ClassElement] = []

        parent_name = class_element.qualified_parent_name
        parent_names_guard = []
        while parent_name is not None and parent_name in self._class_hierarchy:
            parent_element = self._classes.get(parent_name, None)
            if parent_element is None:
                break

            # Prevent infinite loop
            if parent_name in parent_names_guard:
                break
            parent_names_guard.append(parent_name)

            result.append(parent_element)

            # Advance to next parent
            parent_name = self._class_hierarchy.get(parent_name, None)

        return result

    def _determine_class_schema_groups(self, class_element: ClassElement) -> List[str]:
        """ Lookup 'schema groups' for class_element.

        'None' schema groups imply 'inherit from parent'. An empty list implies this class is not
        contained in any schema group and this information is not inherited from parent class.
        """

        # Class already has schema groups, use right away - no inheritance required.
        result_schema_groups = class_element.schema_groups
        if class_element.has_schema_groups_attr:
            return result_schema_groups

        # Find parent 'schema groups' recursively.
        parent_names_guard = []
        parent_name = class_element.qualified_parent_name
        while parent_name is not None and parent_name in self._class_hierarchy:
            parent_element: ClassElement = self._classes.get(parent_name, None)
            if parent_element is None:
                return result_schema_groups

            # Prevent infinite loop
            if parent_name in parent_names_guard:
                return result_schema_groups
            parent_names_guard.append(parent_name)

            parent_schema_groups = parent_element.schema_groups
            if parent_schema_groups:
                result_schema_groups.extend(parent_schema_groups)

            # Stop recursion if parent defines explicit groups
            if parent_element.has_schema_groups_attr:
                return result_schema_groups

            # Guard against infinite loop
            # TODO: this should not happen, check why export contains these
            if parent_name == parent_element.qualified_parent_name:
                logging.error(f"parent class matches class for '{parent_name}'")
                return result_schema_groups

            parent_name = parent_element.qualified_parent_name

        return result_schema_groups

    def _determine_class_child_group(self, class_element: ClassElement) -> Optional[str]:
        """ Lookup 'child group' for class_element.

        'None' groups imply 'inherit from parent'. Empty groups imply 'do not inherit'.
        For documentation purposes, child groups with an empty name can be abandoned here as
        they should not show up in the output, hence the mapping from empty name to None.
        """

        # Class already has  child group, use right away.
        child_group = class_element.child_group
        if child_group is not None:
            return child_group

        # Find parent 'child group' recursively.
        parent_name = class_element.qualified_parent_name
        parent_names_guard = []
        while parent_name is not None and parent_name in self._class_hierarchy:
            parent_element: ClassElement = self._classes.get(parent_name, None)
            if parent_element is None:
                return None

            # Prevent infinite loop
            if parent_name in parent_names_guard:
                return None
            parent_names_guard.append(parent_name)

            parent_child_group = parent_element.child_group
            if parent_child_group is not None:
                return parent_child_group

            # Guard against infinite loop
            # TODO: this should not happen, check why export contains these
            if parent_name == parent_element.qualified_parent_name:
                logging.error(f"parent class matches class for '{parent_name}'")
                return None

            # Advance recursion
            parent_name = parent_element.qualified_parent_name

        return None

    def _determine_class_children_names(self, class_name: str) -> List[str]:
        """
        Determine children of 'class_name' (a namespace-qualified key, see
        ClassElement.qualified_name).
        :return: list of children, by their plain (bare) name for display
        """

        result = []
        if not class_name:
            return result

        for (key, value) in self._class_hierarchy.items():
            if value == class_name:
                child_element = self._classes.get(key, None)
                result.append(child_element.name if child_element is not None else key)

        return result

    @staticmethod
    def _determine_enum_parents(classes: Dict[str, ClassElement], enumerations: Dict[str, EnumerationElement]) -> None:
        """
        For an enum (e.g. 'Button.options'), find all parent enumerations via the members of
        all parent classes of 'Button.

        Motivation: In the output document, print enumerations not only with their direct enumerators
        but also with enumerators introduced by their parents so the user does not have to browse the
        class hierarchy.

        :param classes: all classes as parsed from file
        :param enumerations: all enumerations as parsed from file, key is 'Class.EnumerationName'
        :return: nothing, the information is added to the elements in 'enumerations'
        """

        # Step 1: resolve enum hierarchy by class hierarchy
        for enum_element in enumerations.values():

            # Support enums that imply a class name only. Expected name format is
            # [ClassName].[EnumName] where the enum may contain periods as well.
            # Examples: 'Button.options' or 'Flexbox.flex.align'.
            period_index = enum_element.name.find('.')
            if period_index == -1:
                continue

            # Lookup enum introducing class, traverse class inheritance tree.
            class_name = enum_element.name[:period_index]
            enum_name = enum_element.name[period_index + 1:]

            class_element = classes.get(class_name, None)
            if class_element is not None:
                for parent_name in [p.name for p in class_element.parents]:
                    parent_enum_name = EnumerationElement.qualified_name(parent_name, enum_name)
                    parent_enum_element = enumerations.get(parent_enum_name, None)
                    if parent_enum_element is not None:
                        enum_element.parents.append(parent_enum_element)

        # Step 2: determine parents for enumerations that have the 'parent' attribute set.
        for enum_name, enum_element in enumerations.items():
            parent_enum_name = enum_element.parent_name
            if not parent_enum_name:
                continue

            parent_enum_element = enumerations.get(parent_enum_name)
            if parent_enum_element is None:
                logging.warning("referenced parent enumeration %s not found" % parent_enum_name)
                continue

            enum_element.parents.append(parent_enum_element)

            # Register 'parent enumeration' parents.
            parent_parents = parent_enum_element.parents
            for p_element in parent_parents:
                enum_element.parents.append(p_element)
