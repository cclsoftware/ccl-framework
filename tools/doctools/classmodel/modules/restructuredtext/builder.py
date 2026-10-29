"""Build an RST view of a parsed classmodel Model, resolving all RST-specific link targets."""

import copy
import logging
import operator
import os
from itertools import chain
from typing import Dict, List, Tuple

from modules.classmodel.elements import ClassElement, EnumerationElement, ObjectElement, MemberElement
from modules.classmodel.model import Model
from modules.restructuredtext.elements import RstClass, RstEnumeration, RstObject, RstMember, RstMemberList, DocGroupElement, \
    SchemaGroupElement, RstGroup, LinkedElement, LinkedClassElement, ScopedLinkedClassElement
from modules.restructuredtext.environment import Environment

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


class RstModel:
    """ RST view of a Model: mirrors Model's collections, with every element wrapped for RST output. """

    def __init__(self):
        self.root_name: str = ""
        self.classes: Dict[str, RstClass] = {}
        self.enumerations: Dict[str, RstEnumeration] = {}
        self.objects_hierarchy: Dict[str, RstObject] = {}
        self.doc_groups: Dict[str, DocGroupElement] = {}
        self.displayed_schema_groups: Dict[str, SchemaGroupElement] = {}


class RstBuilder:
    """ Enrich a parsed Model with RST-specific data (link targets, grouped members, doc/schema
    group listings, external files) and wrap it for RST output. """

    def __init__(self, model: Model, env: Environment):
        self.model = model
        self.env = env

    def build(self) -> RstModel:
        rst_model = RstModel()
        rst_model.root_name = self.model.root_name

        # Schema groups and the class -> child-group-link-kind resolution must be computed before
        # building the RstClass views, which need the resolved link kind.
        schema_groups = self._build_schema_group_buckets()
        rst_model.displayed_schema_groups = self._get_displayed_schema_group_buckets(schema_groups)
        child_group_link_kinds = self._determine_child_group_link_kinds(schema_groups)

        for class_name, class_element in self.model.classes.items():
            rst_model.classes[class_name] = self._build_class(
                class_element, child_group_link_kinds.get(class_name, ""))

        for enum_name, enumeration in self.model.enumerations.items():
            rst_model.enumerations[enum_name] = RstEnumeration(enumeration, self.env)

        objects_hierarchy: Dict[str, ObjectElement] = {}
        self._resolve_objects(objects_hierarchy, self.model.objects)
        for object_name, obj in objects_hierarchy.items():
            external_file = self._determine_external_file(obj.kind, obj.name)
            rst_model.objects_hierarchy[object_name] = RstObject(obj, self.env, external_file)

        rst_model.doc_groups = self._build_doc_groups()

        return rst_model

    def _build_class(self, class_element: ClassElement, child_group_link_kind: str) -> RstClass:
        link_elements = self._build_class_link_elements(class_element)
        members_by_class, members_count = self._build_members_by_class(class_element)
        external_file = self._determine_external_file(class_element.kind, class_element.name)

        return RstClass(class_element, self.env, link_elements, members_by_class, members_count,
                        child_group_link_kind, external_file)

    def _determine_external_file(self, kind: str, name: str) -> str:
        """
        Generate include directive if there is an external file to include for use
        in the brief section of an element. Checks for file exists first to not have
        dangling includes as they are reported by sphinx during build.

        Uses element type and name as expected file name, e.g. "class-command.rst".

        :returns: RST include directive or empty string if file does not exist
        """

        filename = "%s-%s.rst" % (kind, name.lower())
        file_path = os.path.join(self.env.get_external_files_path(), filename)
        if not os.path.isfile(file_path):
            return ""

        logging.info(f"including external file '{filename}' for element '{name}'")
        return filename

    def _build_class_link_elements(self, class_element: ClassElement) -> List[LinkedElement]:
        """Generate links to class links if the linked element is known to the model."""

        if not class_element.links:
            return []

        anchor_prefix = self.env.get_anchor_prefix()
        result = []

        for link in class_element.links:
            # "\see" links are bare, hand-authored references with no namespace information,
            # so resolve them against all registered classes rather than the (now
            # namespace-qualified) class_hierarchy keys directly.
            is_known_class = self.model.lookup_class(link) is not None
            if is_known_class:
                result.append(LinkedClassElement(link, self.env))
            # TODO skin-elements -> link to visual styles
            # TODO: workaround until import of multiple files at once is supported
            elif anchor_prefix == "skin-elements-classmodel":
                if link.endswith("Style") and not is_known_class:
                    result.append(ScopedLinkedClassElement(link, "visual-styles-classmodel", self.env))
            # TODO visual styles -> link to skin elements
            # TODO: workaround until import of multiple files at once is supported
            elif anchor_prefix == "visual-styles-classmodel":
                # see comment above, same workaround that should be improved
                if not is_known_class:
                    result.append(ScopedLinkedClassElement(link, "skin-elements-classmodel", self.env))
            else:
                # for a consistent documentation, this should never happen
                logging.warning(f"no target for link {link} in class {class_element.name}")

        return result

    def _build_members_by_class(self, class_element: ClassElement) -> Tuple[List[RstMemberList], int]:
        """Unify and group members by introducing class, apply doc and type information in
        derived class to parent class order, and resolve RST link targets (enum source class,
        object type link) for each unified member.
        """

        all_classes = list(chain([class_element], class_element.parents))
        unified_members: Dict[str, MemberElement] = {}  # pointers!
        grouped: List[Tuple[str, List[MemberElement]]] = []

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
                grouped.insert(0, (cls.name, sorted_result_members))

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

        # Resolve enum source class once in derived class -> parent class order.
        enumeration_source_class: Dict[str, str] = {}
        for member in unified_members.values():
            if not member.is_enumeration():
                continue

            for cls in all_classes:
                enum_lookup_name = EnumerationElement.qualified_name(cls.name, member.name)
                if enum_lookup_name in self.model.enumerations:
                    enumeration_source_class[member.name] = cls.name
                    break

        # Resolve object type members (typeName="RenderAtom" etc.) to a known Model.Class, if any.
        type_class_name: Dict[str, str] = {}
        for member in unified_members.values():
            if not member.is_object_type():
                continue

            referenced_class = self.model.lookup_class(member.type_element.type_name)
            if referenced_class is not None:
                type_class_name[member.name] = referenced_class.name

        members_by_class = [
            RstMemberList(class_name=cls_name,
                          members=[RstMember(m, self.env, enumeration_source_class.get(m.name, ""),
                                             type_class_name.get(m.name, "")) for m in members])
            for cls_name, members in grouped
        ]

        return members_by_class, len(unified_members)

    def _build_doc_groups(self) -> Dict[str, DocGroupElement]:
        """
        Build GroupElement map group name -> GroupElement with each group containing a list of
        Model classes that are associated with this group.
        """
        result: Dict[str, DocGroupElement] = {}
        for class_element in self.model.classes.values():
            doc_group_of_class = class_element.doc_group
            # cover those elements that are explicitly assigned to some doc group only
            if not doc_group_of_class:
                continue
            group = DocGroupElement(doc_group_of_class, self.env)
            result.setdefault(doc_group_of_class, group).add_linked_class(class_element.name)

        return result

    def _build_schema_group_buckets(self) -> Dict[str, SchemaGroupElement]:
        """Create and fill 'schema group' buckets with their respective classes."""

        result: Dict[str, SchemaGroupElement] = dict()
        for class_element in self.model.classes.values():
            schema_groups = class_element.schema_groups
            assert (schema_groups is not None)
            if schema_groups is None:
                continue

            for schema_group in schema_groups:
                group = result.setdefault(schema_group, SchemaGroupElement(schema_group, self.env))
                group.add_linked_class(class_element.name)

        return result

    def _get_displayed_schema_group_buckets(self, schema_groups: Dict[str, SchemaGroupElement]
                                            ) -> Dict[str, SchemaGroupElement]:
        """Sort out the schema groups that should not be exported to the documentation:
        - groups that are not used as child groups
        - groups that are "single class" groups, i.e. are derived from a class
        and contain that class only
        """

        result: Dict[str, SchemaGroupElement] = dict()
        for group_name, group in schema_groups.items():
            if not self._is_used_as_child_schema_group(group):
                continue
            if self._is_single_class_schema_group(group):
                continue
            result.setdefault(group_name, group)

        return result

    def _is_single_class_schema_group(self, group: RstGroup) -> bool:
        """Identify whether a schema group has these special characteristics:
        - it is named after a class (i.e. not a meta group)
        - it only contains the class itself and no other classes

        Note: schema groups default to being named after their introducing class's bare
        name (see ClassElement.schema_groups), so this must check bare names, not the
        namespace-qualified registry keys in model.classes.
        """

        return self.model.lookup_class(group.name) is not None and group.count() == 1 and group.contains(group.name)

    def _is_used_as_child_schema_group(self, group: RstGroup) -> bool:
        """Check whether a schema group is used as child group by some class."""

        for class_element in self.model.classes.values():
            if class_element.child_group is not None:
                if group.name == class_element.child_group:
                    return True

        return False

    def _determine_child_group_link_kinds(self, schema_groups: Dict[str, SchemaGroupElement]) -> Dict[str, str]:
        """Determine, per class, whether its child schema group link should reference the schema
        group (in the schema groups overview) or a class, keyed by qualified class name.
        """

        result: Dict[str, str] = {}
        for class_name, class_element in self.model.classes.items():
            child_schema_group = class_element.child_group
            if not child_schema_group:
                continue

            schema_group = schema_groups.get(child_schema_group, None)
            if schema_group is None:
                continue

            if self._is_single_class_schema_group(schema_group):
                result[class_name] = ClassElement.KIND
            else:
                result[class_name] = SchemaGroupElement.KIND

        # Perform integrity check: for each class that has a child schema
        # group the child schema group link type should be set as well.
        for class_name, class_element in self.model.classes.items():
            if class_element.child_group:
                assert (result.get(class_name, "") != "")

        return result

    @staticmethod
    def _resolve_objects(out_map: Dict[str, ObjectElement], objects_dict: Dict[str, ObjectElement]) -> None:
        """
        Create a dict of all parsed objects traversing the children of each object.
        The approach ensures that each Object element is listed only once in the output.
        """
        for key, obj in objects_dict.items():
            out_map[obj.name] = obj
            RstBuilder._resolve_objects(out_map, obj.children)
