"""ClassModel class module."""

import copy
import logging
import operator
import os
from itertools import chain
from typing import Dict, List, Optional

from modules.classmodel.elements import ClassElement, EnumerationElement, ObjectElement, DocGroupElement, \
    LinkedClassElement, ScopedLinkedClassElement, LinkedElement, BaseElement, MemberElement, SchemaGroupElement
from modules.classmodel.environment import Environment
from modules.util import ElementUtil

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Model:
    """ Store all data read from XML, enriched by helper classes. """

    def __init__(self):
        self._root_name = ""
        self._class_hierarchy: Dict[str, str] = dict()
        self._classes = dict()
        self._enumerations = dict()
        self._objects = dict()
        self._objects_hierarchy = dict()
        self._doc_groups: Dict[str, DocGroupElement] = dict()  # class groups
        self._schema_groups: Dict[str, SchemaGroupElement] = dict()  # class schema groups

    @property
    def classes(self) -> Dict[str, ClassElement]:
        return self._classes

    @classes.setter
    def classes(self, classes: Dict[str, ClassElement]) -> None:
        self._classes = classes

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
    def doc_groups(self) -> Dict[str, DocGroupElement]:
        return self._doc_groups

    @property
    def schema_groups(self) -> Dict[str, SchemaGroupElement]:
        return self._schema_groups

    @property
    def displayed_schema_groups(self) -> Dict[str, SchemaGroupElement]:
        return self._get_displayed_schema_group_buckets()

    @property
    def objects_hierarchy(self) -> Dict[str, ObjectElement]:
        return self._objects_hierarchy

    @property
    def class_hierarchy(self) -> Dict[str, str]:
        return self._class_hierarchy

    @class_hierarchy.setter
    def class_hierarchy(self, hierarchy: Dict[str, str]):
        self._class_hierarchy = hierarchy

    def post_process(self, env: Environment):
        """
        Lookup additional model data by cross-references.
        """

        # Resolve parents and children. Important: must be first step as
        # the other enrich steps involve the class hierarchy.
        for class_name, class_element in self._classes.items():
            class_element.parents = self._determine_class_parents(class_element)

            class_element.schema_groups = self._determine_class_schema_groups(class_element)
            class_element.child_group = self._determine_class_child_group(class_element)

            class_element.children = self._determine_class_children_names(class_name)
            class_element.link_elements = self._build_class_link_elements(class_name, env)
            class_element.external_file = self._determine_external_file(class_element, env)

        for object_name, object_element in self._objects.items():
            object_element.external_file = self._determine_external_file(object_element, env)

        # resolve objects hierarchy, note that an alternative approach is
        # to parse the XML for Model.Object nodes recursively
        self._resolve_objects(self._objects_hierarchy, self._objects)
        self._determine_enum_member_definition(self._classes, self._enumerations)
        self._determine_enum_parents(self._classes, self._enumerations)
        self._build_members_by_class(self._classes, self._enumerations)

        self._doc_groups = self._create_class_doc_groups(self._classes, env)
        self._schema_groups = self._build_schema_group_buckets(env)
        self._set_classes_child_group_link_kinds()

    def _determine_class_parents(self, class_element: ClassElement) -> List[ClassElement]:
        """
        Recursively determine all parent classes of 'parent_name'.
        :param: parent_name  name of first parent to lookup
        :param: classes_dict  tuples of {class name:parent name}
        :return: list of parents
        """

        result: List[ClassElement] = []

        parent_name = class_element.parent_name
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
        parent_name = class_element.parent_name
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
            if parent_name == parent_element.parent_name:
                logging.error(f"parent class matches class for '{parent_name}'")
                return result_schema_groups

            parent_name = parent_element.parent_name

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
        parent_name = class_element.parent_name
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
            if parent_name == parent_element.parent_name:
                logging.error(f"parent class matches class for '{parent_name}'")
                return None

            # Advance recursion
            parent_name = parent_element.parent_name

        return None

    @staticmethod
    def _determine_external_file(element: BaseElement, env: Environment) -> str:
        """
        Generate include directive if there is an external file to include for use
        in the brief section of an element. Checks for file exists first to not have
        dangling includes as they are reported by sphinx during build.

        Uses element type and name as expected file name, e.g. "class-command.rst".

        :returns: RST include directive or empty string if file does not exist
        """

        if element is None:
            return ""

        filename = "%s-%s.rst" % (element.kind, element.name.lower())
        file_path = os.path.join(env.get_external_files_path(), filename)
        if not os.path.isfile(file_path):
            return ""

        logging.info(f"including external file '{filename}' for element '{element.name}'")
        return filename

    def _determine_class_children_names(self, class_name: str) -> List[str]:
        """
        Determine children of 'class_name'.
        :return: list of children
        """

        result = []
        if not class_name:
            return result

        for (key, value) in self._class_hierarchy.items():
            if value == class_name:
                result.append(key)

        return result

    def _build_class_link_elements(self, class_name: str, env: Environment) -> List[LinkedElement]:
        """Generate links to class links if the linked element is known to the model."""

        class_element = self._classes.get(class_name, None)
        if class_element is None:
            logging.warning(f"could not prepare class links, class {class_name} not found")
            return []

        # No links, nothing to do.
        if not class_element.links:
            return []

        anchor_prefix = env.get_anchor_prefix()
        result = []

        for link in class_element.links:
            if link in self._class_hierarchy:
                result.append(LinkedClassElement(link, env))
            # TODO skin-elements -> link to visual styles
            # TODO: workaround until import of multiple files at once is supported
            elif anchor_prefix == "skin-elements-classmodel":
                if link.endswith("Style") and link not in self._class_hierarchy:
                    result.append(ScopedLinkedClassElement(link, "visual-styles-classmodel", env))
            # TODO visual styles -> link to skin elements
            # TODO: workaround until import of multiple files at once is supported
            elif anchor_prefix == "visual-styles-classmodel":
                # see comment above, same workaround that should be improved
                if link not in self._class_hierarchy:
                    result.append(ScopedLinkedClassElement(link, "skin-elements-classmodel", env))
            else:
                # for a consistent documentation, this should never happen
                logging.warning(f"no target for link {link} in class {class_name}")

        return result

    @staticmethod
    def _create_class_doc_groups(classes: Dict[str, ClassElement], env: Environment) -> Dict[str, DocGroupElement]:
        """
        Build GroupElement map group name -> GroupElement with each group containing a list of
        Model classes that are associated with this group.
        """
        result = dict()
        for class_name, class_element in classes.items():
            doc_group_of_class = class_element.doc_group
            # cover those elements that are explicitly assigned to some doc group only
            if not doc_group_of_class:
                continue
            element = DocGroupElement(doc_group_of_class, env)
            result.setdefault(doc_group_of_class, element).add_linked_class(class_name)

        return result

    def _build_schema_group_buckets(self, env: Environment) -> Dict[str, SchemaGroupElement]:
        """Create and fill 'schema group' buckets with their respective classes."""

        result: Dict[str, SchemaGroupElement] = dict()
        for class_name, class_element in self.classes.items():
            schema_groups = class_element.schema_groups
            assert (schema_groups is not None)
            if schema_groups is None:
                continue

            for schema_group in schema_groups:
                group = result.setdefault(schema_group, SchemaGroupElement(schema_group, env))
                group.add_linked_class(class_name)

        return result

    def _get_displayed_schema_group_buckets(self) -> Dict[str, SchemaGroupElement]:
        """Sort out the schema groups that should not be exported to the documentation:
        - groups that are not used as child groups
        - groups that are "single class" groups, i.e. are derived from a class
        and contain that class only
        """

        result: Dict[str, SchemaGroupElement] = dict()
        for group_name, group_element in self._schema_groups.items():
            if not self._is_used_as_child_schema_group(group_element):
                continue
            if self._is_single_class_schema_group(group_element):
                continue
            result.setdefault(group_name, group_element)

        return result

    def _is_single_class_schema_group(self, group: SchemaGroupElement) -> bool:
        """Identify whether a schema group has these special characteristics:
        - it is named after a class (i.e. not a meta group)
        - it only contains the class itself and no other classes
        """

        return group.name in self.classes and group.count() == 1 and group.contains(group.name)

    def _is_used_as_child_schema_group(self, group: SchemaGroupElement) -> bool:
        """Check whether a schema group is used as child group by some class."""

        for class_element in self.classes.values():
            if class_element.child_group is not None:
                if group.name == class_element.child_group:
                    return True

        return False

    def _set_classes_child_group_link_kinds(self) -> None:
        """Determine whether a class schema group link should reference
        the schema group (in the schema groups overview) or a class.
        """

        for class_name, class_element in self._classes.items():
            child_schema_group = class_element.child_group
            if not child_schema_group:
                continue

            schema_group: SchemaGroupElement = self._schema_groups.get(child_schema_group, None)
            if schema_group is None:
                continue

            if self._is_single_class_schema_group(schema_group):
                class_element.child_group_link_kind = ClassElement.KIND
            else:
                class_element.child_group_link_kind = SchemaGroupElement.KIND

        # Perform integrity check: for each class that has a child schema
        # group the child schema group link type should be set as well.
        for class_name, class_element in self._classes.items():
            if class_element.child_group:
                assert (class_element.child_group_link_kind != "")

    @staticmethod
    def _build_members_by_class(classes: Dict[str, ClassElement],
                                enumerations: Dict[str, EnumerationElement]) -> None:
        """Unify and group members by introducing class, apply doc and type information
        in derived class to parent class order.
        """

        for class_element in classes.values():

            # Create list in order derived class -> parent class.
            all_classes = list(chain([class_element], class_element.parents))
            unified_members: Dict[str, MemberElement] = {}  # pointers!

            # Create lists by class, unify members. Iterate in reverse order (parent class -> derived class)
            for cls in reversed(all_classes):
                result_members = []
                for m in cls.members:
                    if m.name in unified_members.keys():
                        continue

                    # Must copy to avoid modifications to other classes.
                    cloned_m = copy.deepcopy(m)
                    result_members.append(cloned_m)
                    unified_members[cloned_m.name] = cloned_m

                # Prepend to have result in derived class -> parent class order.
                if result_members:
                    sorted_result_members = sorted(result_members, key=operator.attrgetter("name"))
                    member_list = ClassElement.MemberList(class_name=cls.name, members=sorted_result_members)
                    class_element.members_by_class.insert(0, member_list)

            # Combine members and parent members into single list to simplify
            # loops below. List order is derived class -> parent class.
            all_members = list(chain(class_element.members, *[p.members for p in class_element.parents]))

            # Set member brief to first match in derived class -> parent class order.
            for member in unified_members.values():
                for m in all_members:
                    if m.name == member.name and m.brief:
                        member.brief = m.brief
                        break

            # Set member type to first match once in derived class -> parent class order.
            for member in unified_members.values():
                for m in all_members:
                    if m.name == member.name:
                        member.type_element.type_name = m.type_element.type_name
                        break

            # Set enumeration source class once in derived class -> parent class order.
            for member in unified_members.values():
                if not member.is_enumeration():
                    continue

                for cls in all_classes:
                    enum_lookup_name = ElementUtil.enum_name(cls.name, member.name)
                    if enum_lookup_name in enumerations:
                        member.enumeration_source_class = cls.name
                        break

            # Save statistics.
            class_element.members_count = len(unified_members.values())

    @staticmethod
    def _determine_enum_member_definition(classes: Dict[str, ClassElement],
                                          enumerations: Dict[str, EnumerationElement]) -> None:
        """
        Determine the base class introducing a specific enumeration member for all classes in 'classes'.
        Lookup the enumeration in 'enumerations', test for all Class.Enumeration in inheritance order.

        This is not part determine_member_base_class() as in this scenario, the current class can also
        be the introducing class. Additionally, the enums are defined as Class.Name over just Name.
        """
        for class_element in classes.values():
            for m in class_element.members:
                if not m.is_enumeration():
                    continue

                classes = list(chain([class_element.name], *[cls.name for cls in class_element.parents]))

                # check all class names in inheritance order for Class.Enum being in 'enumerations'
                for class_name in classes:
                    enum_lookup_name = ElementUtil.enum_name(class_name, m.name)
                    if enum_lookup_name in enumerations:
                        m.enumeration_source_class = class_name
                        break

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
                    parent_enum_name = ElementUtil.enum_name(parent_name, enum_name)
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

    @staticmethod
    def _resolve_objects(out_map: Dict[str, ObjectElement], objects_dict: Dict[str, ObjectElement]) -> None:
        """
        Create a dict of all parsed objects traversing the children of each object.
        The approach ensures that each Object element is listed only once in the output.
        """
        for key, obj in objects_dict.items():
            out_map[obj.name] = obj
            Model._resolve_objects(out_map, obj.children)
