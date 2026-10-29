"""RST view classes module.

Renders the classmodel domain model (modules.classmodel.elements/model) to RST. The domain
classes hold parsed data only; every RST-specific concern (hyperlinks, anchors, formatting)
lives here instead, built by RstBuilder (see builder.py) from a parsed Model.
"""

from dataclasses import dataclass
from typing import Dict, List

from modules.classmodel.elements import TypeElement, MethodArgumentElement, MethodElement, PropertyElement, \
    MemberElement, EnumeratorElement, EnumerationElement, ClassElement, ObjectElement
from modules.restructuredtext.environment import Environment
from modules.restructuredtext.rstprinter import RstPrinter

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

TITLE_DETAILS = "Description"
TITLE_METHODS = "Methods"


def subsection_title(title: str) -> str:
    """ Unify title format for subsections such as 'Attributes', 'Properties', ... """
    return RstPrinter.paragraph(RstPrinter.bold(title))


class LinkedElement:
    """ Common purpose linked element, 'name' is printed with :ref:. """

    def __init__(self, name: str, kind: str, env: Environment):
        self.name: str = name
        self.kind: str = kind
        self.env: Environment = env

    def to_rst(self) -> str:
        """ Print name of this class as a 'kind' type anchor. Wrap with RstPrinter.bracketed() for a list. """
        anchor_prefix = self.env.get_anchor_prefix()
        target = RstPrinter.anchor_id(anchor_prefix, self.kind, self.name)
        return RstPrinter.hyperlink(self.name, target)


class LinkedClassElement(LinkedElement):
    """Link to a class."""

    def __init__(self, name: str, env: Environment):
        super().__init__(name, "class", env)


class LinkedObjectElement(LinkedElement):
    """Link to object."""

    def __init__(self, name: str, env: Environment):
        super().__init__(name, "object", env)


class LinkedEnumerationElement(LinkedElement):
    """Link to enumeration."""

    def __init__(self, name: str, env: Environment):
        super().__init__(name, "enumeration", env)


class LinkedDocGroupElement(LinkedElement):
    """Link to group."""

    def __init__(self, name: str, env: Environment):
        super().__init__(name=name, kind=DocGroupElement.KIND, env=env)


class ScopedLinkedClassElement(LinkedElement):
    """
    LinkedClassElement for which a custom scope (i.e. different rst file)
    can be specified that is used for UID reference. This is a workaround
    class to link between skin elements and visual styles only.
    """

    def __init__(self, name: str, scope: str, env: Environment):
        super().__init__(name, "class", env)
        self.scope: str = scope

    def to_rst(self) -> str:
        """ Print name of this class as a class anchor. """

        # bypass the actual scope provided by the environment
        anchor_prefix = self.scope
        target = RstPrinter.anchor_id(anchor_prefix, self.kind, self.name)
        return RstPrinter.hyperlink(self.name, target)


class LinkedEnumerationMemberElement(LinkedElement):
    """
    Represents a linked enumeration member which name is printed with :ref: but points to the
    baseclass.anchor which introduced the enumeration.

    For this class, different names for display and link references are used. This is required because
    enumerations are defined with their introducing class (e.g. "View.attach") in the XML but are referenced
    by their name only in derived Model.Class member elements (e.g. "attach").

    """

    def __init__(self, name: str, link_target_name: str, env: Environment):
        """
        :param name: name as displayed in the output
        :param link_target_name:  name that must match the hyperlink target
        """
        super().__init__(name, "enumeration", env)
        # allow different target name [BaseClass].[EnumerationName]
        self.link_target_name: str = link_target_name

    def to_rst(self) -> str:
        """ Print name of this enumeration as an enumeration anchor. """
        anchor_prefix = self.env.get_anchor_prefix()
        target = RstPrinter.anchor_id(anchor_prefix, self.kind, self.link_target_name)
        return "%s " % RstPrinter.hyperlink(self.name, target)


def render_type(type_element: TypeElement) -> str:
    if type_element.type_name:
        return type_element.type_name
    if type_element.type:
        return type_element.type

    raise ValueError


def render_method_argument(argument: MethodArgumentElement) -> str:
    """ Print w/o default argument """
    value_string = argument.name if not argument.default_value else "%s=%s" % (argument.name, argument.default_value)
    return "%s: %s" % (render_type(argument.type_element), RstPrinter.italic(value_string))


def render_method(method: MethodElement) -> str:
    rst_args = [render_method_argument(arg) for arg in method.arguments]
    args_string = ", ".join(rst_args)
    return "%s %s (%s)\n\n" % (render_type(method.return_type), RstPrinter.bold(method.name), args_string)


def render_property(prop: PropertyElement) -> str:
    return "%s (%s)\n\n" % (RstPrinter.bold(prop.name), render_type(prop.return_type))


def render_enumerator(enumerator: EnumeratorElement) -> str:
    """
    Create an RST string for an enumerator, containing 'name', 'value' and 'brief' description.
    Does not print the value of an enumeration as it is not helpful from a readers perspective.
    """

    result = RstPrinter.bold(enumerator.name)
    doc_string = enumerator.build_doc_string()
    if doc_string:
        result += f": {doc_string}"

    result += "\n"
    return result


class RstMember:
    """ RST view of a member: adds RST link targets resolved by RstBuilder (the class introducing
    an enum-typed member's enumeration, and the Model.Class an object-typed member's typeName
    resolves to, if any). """

    def __init__(self, member: MemberElement, env: Environment, enumeration_source_class: str = "",
                type_class_name: str = ""):
        self.member = member
        self.env = env
        self.enumeration_source_class = enumeration_source_class
        self.type_class_name = type_class_name

    def to_rst(self) -> str:
        member = self.member

        # link to enum definitions for reference, enum members only, requires the enum source class to be set
        member_name_or_link = RstPrinter.bold(member.name)
        if member.is_enumeration() and self.enumeration_source_class:
            # the enum definitions and ref targets are in format "Class.Enum"
            enum_link_target = "%s.%s" % (self.enumeration_source_class, member.name)
            member_name_or_link = LinkedEnumerationMemberElement(member.name, enum_link_target, self.env).to_rst()

        # link to class definitions for reference, object type members only
        if member.is_object_type() and self.type_class_name:
            type_str = LinkedClassElement(self.type_class_name, self.env).to_rst()
        else:
            type_str = RstPrinter.italic(render_type(member.type_element))

        result = f"{member_name_or_link} ({type_str})"

        doc_string = member.build_doc_string()
        if doc_string:
            result += f": {doc_string}"

        result += "\n"
        return result


@dataclass
class RstMemberList:
    class_name: str  # member introducing class
    members: List[RstMember]  # members introduced by class_name


class RstEnumeration:
    """ RST view of an enumeration, including inherited parent enumerators. """

    def __init__(self, enumeration: EnumerationElement, env: Environment):
        self.enumeration = enumeration
        self.env = env

    @property
    def name(self) -> str:
        return self.enumeration.name

    @property
    def kind(self) -> str:
        return self.enumeration.kind

    def to_rst(self) -> str:
        enumeration = self.enumeration

        out = RstPrinter.h3(enumeration.name)
        out += RstPrinter.paragraph(enumeration.brief)

        if enumeration.detailed:
            out += subsection_title(TITLE_DETAILS) + RstPrinter.paragraph(
                enumeration.detailed)

        out += subsection_title("Enumerators")

        # May be empty if enumerators are derived from parent only.
        if enumeration.enumerators:
            enumerators_str = ""
            for e in enumeration.enumerators:
                enumerators_str += RstPrinter.list_element(render_enumerator(e))
            out += RstPrinter.paragraph(enumerators_str)

        # if available, also print all enumerators that origin from parent classes
        if enumeration.parents:
            for parent_enum in enumeration.parents:
                parent_enum_link = LinkedEnumerationElement(parent_enum.name, self.env)
                out += RstPrinter.paragraph(
                    "Inherited from %s" % RstPrinter.bracketed(parent_enum_link.to_rst()))
                for p in parent_enum.enumerators:
                    out += RstPrinter.list_element(render_enumerator(p))

                # sphinx: list must end with a new line
                out += "\n"

        return out


class RstClass:
    """ RST view of a class: adds "\\see" links, resolved grouped-by-introducing-class members
    (with RST link targets), and the include file for an external file, all built by RstBuilder. """

    def __init__(self, class_element: ClassElement, env: Environment, link_elements: List[LinkedElement],
                members_by_class: List[RstMemberList], members_count: int, child_group_link_kind: str,
                external_file: str):
        self.class_element = class_element
        self.env = env
        self.link_elements = link_elements
        self.members_by_class = members_by_class
        self.members_count = members_count
        self.child_group_link_kind = child_group_link_kind
        self.external_file = external_file

    @property
    def name(self) -> str:
        return self.class_element.name

    @property
    def kind(self) -> str:
        return self.class_element.kind

    def to_rst(self) -> str:
        """ Convert the class model object to an RST string """

        class_element = self.class_element

        class_str = RstPrinter.h3(class_element.name) + RstPrinter.paragraph(class_element.brief, True)
        out = ""
        if self.external_file:
            out += RstPrinter.include(self.external_file)

        if class_element.detailed:
            out += subsection_title(TITLE_DETAILS) + RstPrinter.paragraph(
                class_element.detailed)

        if self.link_elements:
            rst_links = [RstPrinter.bracketed(e.to_rst()) for e in self.link_elements]
            links_text = " ".join(rst_links)
            out += RstPrinter.paragraph(f"See also: {links_text}")

        # iterate over list of {brief, code} dictionaries
        if class_element.code_samples:
            for caption_index, code_sample in enumerate(class_element.code_samples):
                # enumerate for multiple code samples only to not have "Example 1"
                # as the single only caption in the documentation for the current class
                caption = f"Example {caption_index + 1}" if len(class_element.code_samples) > 1 else "Example"

                # "Example: brief" or just "Example" if there is no brief
                if code_sample.brief:
                    code_caption = f"{caption}: {code_sample.brief}"
                else:
                    code_caption = caption

                out += RstPrinter.code_block(code_caption, code_sample.code, class_element.code_lang)

        # print members, grouped by inheriting class
        out_members = ""
        for member_list in self.members_by_class:
            members_str = ""

            # Print 'inherited from' for parent classes only.
            if member_list.class_name != class_element.name:
                parent_link = LinkedClassElement(member_list.class_name, self.env)
                members_str = RstPrinter.paragraph(f"Inherited from {RstPrinter.bracketed(parent_link.to_rst())}")

            for member in member_list.members:
                members_str += RstPrinter.list_element(member.to_rst())
            out_members += RstPrinter.paragraph(members_str)

        # any member data was written
        if out_members:
            out += subsection_title(f"Attributes ({self.members_count})") + out_members

        # print methods section
        if class_element.methods:
            methods_str = ""
            for method in class_element.methods:
                methods_str += RstPrinter.list_element(render_method(method))
            out += subsection_title("Methods") + RstPrinter.paragraph(methods_str)

        # avoid empty class body before meta section to prevent sphinx build warnings
        if not out.strip():
            out += RstPrinter.paragraph("No info available.")

        ################################################################
        # List of meta info attributes
        ################################################################

        meta_items: List[str] = list()

        if class_element.abstract:
            meta_items.append("Abstract")

        if class_element.scriptable:
            meta_items.append("Scriptable")

        if class_element.mutable:
            meta_items.append("Mutable")

        # class parents
        if class_element.parents:
            elements_str = [RstPrinter.bracketed(LinkedClassElement(parent.name, self.env).to_rst())
                             for parent in class_element.parents]
            parents_str = " <- ".join(elements_str)
            meta_items.append(f"Parents: {parents_str}")

        # child classes
        if class_element.children:
            elements_str = [RstPrinter.bracketed(LinkedClassElement(child_name, self.env).to_rst())
                             for child_name in class_element.children]
            child_classes_str = " ".join(elements_str)
            meta_items.append(f"Children: {child_classes_str}")

        # doc group
        if class_element.doc_group:
            group_link = LinkedDocGroupElement(class_element.doc_group, self.env)
            meta_items.append(f"Group: {RstPrinter.bracketed(group_link.to_rst())}")

        # schema child group
        if class_element.child_group and self.child_group_link_kind:
            link = LinkedElement(class_element.child_group, self.child_group_link_kind, self.env)
            meta_items.append(f"Schema children: {RstPrinter.bracketed(link.to_rst())}")

        if meta_items:
            out += subsection_title("Meta")
            out += RstPrinter.list(meta_items)

        return f"{class_str}{out}"


class RstObject:
    """ RST view of an object, including the include file for an external file, built by RstBuilder. """

    def __init__(self, obj: ObjectElement, env: Environment, external_file: str):
        self.object = obj
        self.env = env
        self.external_file = external_file

    @property
    def name(self) -> str:
        return self.object.name

    @property
    def kind(self) -> str:
        return self.object.kind

    def to_rst(self) -> str:
        obj = self.object
        out = RstPrinter.h3(obj.name)

        # external file content
        if self.external_file:
            out += RstPrinter.include(self.external_file)

        # print methods section
        if obj.methods:
            out += subsection_title(TITLE_METHODS)
            methods_str = ""
            for method in obj.methods:
                out += RstPrinter.list_element(render_method(method))
            out += RstPrinter.paragraph(methods_str)

        # print properties section
        if obj.properties:
            out += subsection_title("Properties")
            properties_str = ""
            for p in obj.properties:
                out += RstPrinter.list_element(render_property(p))
            out += RstPrinter.paragraph(properties_str)

        # print child objects
        if obj.children:
            elements_str = [RstPrinter.bracketed(LinkedObjectElement(child_name, self.env).to_rst())
                             for child_name in obj.children]
            children_str = " ".join(elements_str)
            out += subsection_title("Meta") + RstPrinter.list_element(f"Children: {children_str}")

        return out


class RstGroup:
    """ RST view of a doc group or schema group: a named bucket of linked classes/objects. """

    def __init__(self, name: str, kind: str, env: Environment):
        self.name = name
        self.kind = kind
        self.env = env
        self.items: Dict[str, LinkedElement] = {}  # all contained classes, objects as linked element

    def add_linked_class(self, name: str) -> None:
        self.items[name] = LinkedClassElement(name, self.env)

    def add_linked_object(self, name: str) -> None:
        self.items[name] = LinkedObjectElement(name, self.env)

    def to_rst(self) -> str:
        """ Print all items, separated by whitespace. """
        out = RstPrinter.h3("%s (%d)" % (self.name, len(self.items.values())))
        out += " ".join([RstPrinter.bracketed(item.to_rst()) for item in self.items.values()])
        return out

    def count(self) -> int:
        return len(self.items)

    def contains(self, name: str) -> bool:
        return name in self.items


class DocGroupElement(RstGroup):
    KIND = "docgroup"

    def __init__(self, name: str, env: Environment):
        super().__init__(name, DocGroupElement.KIND, env)


class SchemaGroupElement(RstGroup):
    KIND = "schemagroup"

    def __init__(self, name: str, env: Environment):
        super().__init__(name, SchemaGroupElement.KIND, env)
