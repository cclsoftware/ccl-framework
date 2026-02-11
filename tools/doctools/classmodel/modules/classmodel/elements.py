"""Element classes module."""

from abc import ABC
from dataclasses import dataclass
from typing import Dict, List, Optional

from modules.classmodel.environment import Environment
from modules.classmodel.xmldefs import XML

from modules.rstprinter import RstPrinter

from modules.util import EncodingHelper, ElementUtil, ElementUID

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class PrintableElement(ABC):
    """Element class that can be converted to rst string."""

    def to_rst(self) -> str:
        raise NotImplementedError


class BaseElement(PrintableElement):
    """ Base class for class, enumeration or object Model elements. """

    TITLE_DETAILS: str = "Description"
    TITLE_METHODS: str = "Methods"

    def __init__(self, name: str, kind: str, env: Environment) -> None:
        self.name: str = name  # element name (e.g.: class or method name)
        self.env: Environment = env  # document environment
        self.attributes: dict = {}  # all XML element attributes
        self.brief: str = ""  # class brief description
        self.detailed: str = ""  # class detailed description
        self.kind = kind
        self.external_file: str = ""  # name of external file to include

    def to_rst(self) -> str:
        raise NotImplementedError

    def set_detailed(self, details: str) -> None:
        """ Replace CRLFs as they cause unwanted additional paragraphs. """
        self.detailed = EncodingHelper.replace_crlf(details)

    @staticmethod
    def subsection_title(title: str) -> str:
        """ Unify title format for subsections such as 'Attributes', 'Properties', ... """
        return RstPrinter.paragraph(RstPrinter.bold(title))

    def _build_doc_string(self) -> str:
        """Combine documentation brief and detailed into a single, appendable doc string.
        Return nothing if there is no brief description.
        """

        if not self.brief:
            return ""

        result = self.brief
        if not self.detailed:
            return result

        # Details may introduce a second sentence. Ensure brief
        # "sentence" ends on a period character. Consistency: ensure
        # detailed sentence also ends on a period character.
        detailed = self.detailed
        if not result.endswith("."):
            result += "."

        # Detailed sentence should always start with upper case.
        detailed = detailed.replace(detailed[0], detailed[0].upper(), 1)
        result += f" {detailed}"
        if not result.endswith("."):
            result += "."

        return result


class TypeElement(PrintableElement):
    """
    Argument or return type and type name.
    The type name is optionally provided by the classmodel file
    and is to be prioritized.

    Examples, not correlated to each other:

        <Model.MethodArg name="element" type="object" typeName="StripElement"/>
        <Model.MethodArg name="invoker" type="object" typeName="Object" defaultValue="null"/>
        <Model.ReturnValue x:id="retval" type="bool"/>
        <Model.ReturnValue x:id="retval" type="object" typeName="Element"/>
    """

    def __init__(self, type_value: str, type_name_value: str) -> None:
        self.type: str = type_value
        self.type_name: str = type_name_value

    def __eq__(self, other) -> bool:
        """Compare by type only, type_name not consistently available. The type_name
        may also vary for the same class member along the class inheritance tree.
        """
        return self.type == other.type

    def to_rst(self) -> str:
        if self.type_name:
            return self.type_name
        if self.type:
            return self.type

        raise ValueError

    def is_enum(self) -> bool:
        """ Check typeName over type! """
        return self.type_name == "enum"


class MethodArgumentElement(BaseElement):
    """ Represents a method argument """

    def __init__(self, name: str, type_element: TypeElement, default_value: str, env: Environment):
        super().__init__(name, "method-arg", env)
        self.type_element: TypeElement = type_element
        self.default_value: str = default_value

    def to_rst(self) -> str:
        """ Print w/o default argument """
        value_string = self.name if not self.default_value else "%s=%s" % (self.name, self.default_value)
        return "%s: %s" % (self.type_element.to_rst(), RstPrinter.italic(value_string))


class MethodElement(BaseElement):
    """ Represent a class or object method"""

    def __init__(self, name: str, return_type: TypeElement, arguments: List[MethodArgumentElement], env: Environment):
        super().__init__(name, "method", env)
        self.return_type: TypeElement = return_type
        self.arguments: List[MethodArgumentElement] = arguments

    def to_rst(self) -> str:
        rst_args = [arg.to_rst() for arg in self.arguments]
        args_string = ", ".join(rst_args)
        return "%s %s (%s)\n\n" % (self.return_type.to_rst(), RstPrinter.bold(self.name), args_string)


class PropertyElement(BaseElement):
    """ Represent an object property"""

    def __init__(self, name: str, return_type: TypeElement, env: Environment):
        super().__init__(name, "property", env)
        self.return_type: TypeElement = return_type

    def to_rst(self) -> str:
        return "%s (%s)\n\n" % (RstPrinter.bold(self.name), self.return_type.to_rst())


class MemberElement(BaseElement):
    """ Represent an object member. """

    def __init__(self, name: str, type_element: TypeElement, readonly: bool, brief: str, detailed: str,
                 env: Environment):
        super().__init__(name, "member", env)
        self.type_element: TypeElement = type_element
        self.brief: str = brief
        self.detailed: str = detailed
        self.readonly: bool = readonly
        self.inherited: bool = False
        self.enumeration_source_class: str = ""  # enumerations only: introducing class (name)

    def is_enumeration(self) -> bool:
        return self.type_element.is_enum()

    def to_rst(self) -> str:

        # link to enum definitions for reference, enum members only, requires the enum source class to be set
        member_name_or_link = RstPrinter.bold(self.name)
        if self.is_enumeration() and self.enumeration_source_class:
            # the enum definitions and ref targets are in format "Class.Enum"
            enum_link_target = ElementUtil.enum_name(self.enumeration_source_class, self.name)
            member_name_or_link = LinkedEnumerationMemberElement(self.name, enum_link_target, self.env).to_rst()

        type_str = self.type_element.to_rst()
        result = f"{member_name_or_link} ({RstPrinter.italic(type_str)})"

        doc_string = self._build_doc_string()
        if doc_string:
            result += f": {doc_string}"

        result += "\n"
        return result


class LinkedElement(BaseElement):
    """ Common purpose linked element, 'name' is printed with :ref:."""

    def __init__(self, name: str, kind: str, env: Environment):
        super().__init__(name, kind, env)

    def to_rst(self) -> str:
        """ Print name of this class as a 'kind' type anchor. """
        anchor_prefix = self.env.get_anchor_prefix()
        target = ElementUID.create(anchor_prefix, self.kind, self.name)
        return "[%s]" % RstPrinter.hyperlink(self.name, target)


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
        target = ElementUID.create(anchor_prefix, self.kind, self.name)
        return "[%s] " % RstPrinter.hyperlink(self.name, target)


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
        target = ElementUID.create(anchor_prefix, self.kind, self.link_target_name)
        return "%s " % RstPrinter.hyperlink(self.name, target)


class ClassElement(BaseElement):
    """ Model.Class representation as parsed from XML. """

    KIND = "class"

    @dataclass
    class CodeSample:
        brief: str  # code snippet description
        code: str  # code snippet

    @dataclass
    class MemberList:
        class_name: str  # member introducing class
        members: List[MemberElement]  # members introduced by class_name

    def __init__(self, name: str, attributes: Dict, env: Environment):
        super().__init__(name, ClassElement.KIND, env)
        self.attributes = attributes
        self.members: List[MemberElement] = []
        self.methods: List[MethodElement] = []
        self.parents: List[ClassElement] = []  # names of parent classes
        self.children: List[str] = []  # names of children classes
        self.code_samples: List[ClassElement.CodeSample] = []
        self.code_lang: str = ""  # code block language (cpp, xml, ...) for all code snippets
        self.links: List[str] = []  # references from doxygen code (\see ...)
        self.link_elements: List[LinkedElement] = []  # resolved links as printable elements
        self.doc_group = ""  # attribute: doc group

        # attribute: xml schema groups
        # - attr specified but empty value string: "not in any group" (but in its own implicit group)
        # - attr not specified: "inherited from parent" (and in its own implicit group)
        # A class is always in its own schema group, i.e. the group named
        # after the class. This is independent of whether the Class:SchemaGroup
        # attribute is set in the XML. So start with self.name.
        self.schema_groups: List[str] = [self.name]

        # Remember whether the class has schema groups configured in the classmodel file. If
        # the attribute is not set (i.e. the xml element is missing entirely), the importer
        # must resolve schema groups from parent classes. Saved as extra state since
        # self.schema_groups can never be None or empty (will at least have the class itself)
        self.has_schema_groups_attr: bool = False

        # attribute: xml child group
        # - attr specified but empty value string: "has no child group"
        # - attr not specified: "inherit child group from parent class(es)"
        self.child_group: Optional[str] = None
        self.child_group_link_kind: str = ""

        # TODO: future, reintegrate into members + parents.
        self.members_by_class: List[ClassElement.MemberList] = []
        self.members_count: int = 0

    @property
    def namespace(self):
        return self.attributes.get(XML.ATTR_NAMESPACE, "")

    @property
    def parent_name(self):
        return self.attributes.get(XML.ATTR_PARENT, "")

    @property
    def parent_namespace(self):
        return self.attributes.get(XML.ATTR_PARENT_NAMESPACE, "")

    @property
    def mutable(self):
        return self.attributes.get(XML.ATTR_MUTABLE, False)

    @property
    def abstract(self):
        return self.attributes.get(XML.ATTR_ABSTRACT, False)

    @property
    def scriptable(self):
        return self.attributes.get(XML.ATTR_SCRIPTABLE, False)

    def lookup_member(self, name: str, type_element: TypeElement) -> Optional[MemberElement]:
        """Find member by name and type."""
        for m in self.members:
            if m.name == name and m.type_element == type_element:
                return m

        return None

    def has_member(self, name: str, type_element: TypeElement) -> bool:
        """Check if member with name and type exists."""
        return self.lookup_member(name, type_element) is not None

    def has_member_by_class(self, member_name: str, member_class: str) -> bool:
        for m in self.members:
            if m.name == member_name and m.type_element.type_name == member_class:
                return True

        return False

    def set_code(self, text: str) -> None:
        """
        Parse class model imported code snippet. Can be either the 'old' format
        with inline comments, i.e.:

        <!-- Brief -->
        <Code Snippet 1>

        <!-- Brief2 -->
        <Code Snippet 2>

        ... or an alternative (new) format, with comment lines declared by a '#':

        # Brief 1
        <Code Snippet 1>

        # Brief 2
        <Code Snippet 2>

        The old format results in a single entry list of {brief, code}
        with brief being empty, the latter format results in a list of
        [{brief, code}, {brief, code}, ...].

        On export, the old format appears as a single code block, the new one
        as separated blocks with brief and code being separately formatted.

        Reminder: Use '#' to still allow code snippet inline comments (XML, cpp).
        Do not interpret the old format as separate blocks.

        :param text: code snippet to parse
        """

        code_brief = ""
        code_sample = ""
        is_parsing_code = False

        comment_control_char = "#"

        # clear control chars, split by line, iterate by line
        # parse blocks, start over when a block ends: code was parsed
        # and a new comment line was found
        for line in EncodingHelper.replace_crlf(text).splitlines(True):
            if line.startswith(comment_control_char):

                # block ends, dump data, start over
                if is_parsing_code:
                    self.code_samples.append(ClassElement.CodeSample(brief=code_brief, code=code_sample))
                    code_brief = ""
                    code_sample = ""
                    is_parsing_code = False

                # omit first control character, then omit whitespaces
                code_brief += line[1:].lstrip()
            else:
                is_parsing_code = True
                code_sample += line

        # closure, append remaining information
        # if there is a brief but no code: drop it
        if code_sample:
            self.code_samples.append(ClassElement.CodeSample(brief=code_brief, code=code_sample))

    def to_rst(self) -> str:
        """ Convert the class model object to an RST string """

        class_str = RstPrinter.h3(self.name) + RstPrinter.paragraph(self.brief, True)
        out = ""
        if self.external_file:
            out += RstPrinter.include(self.external_file)

        if self.detailed:
            out += BaseElement.subsection_title(BaseElement.TITLE_DETAILS) + RstPrinter.paragraph(self.detailed)

        if self.link_elements:
            rst_links = [e.to_rst() for e in self.link_elements]
            links_text = " ".join(rst_links)
            out += RstPrinter.paragraph(f"See also: {links_text}")

        # iterate over list of {brief, code} dictionaries
        if self.code_samples:
            for caption_index, code_sample in enumerate(self.code_samples):
                # enumerate for multiple code samples only to not have "Example 1"
                # as the single only caption in the documentation for the current class
                caption = f"Example {caption_index + 1}" if len(self.code_samples) > 1 else "Example"

                # "Example: brief" or just "Example" if there is no brief
                if code_sample.brief:
                    code_caption = f"{caption}: {code_sample.brief}"
                else:
                    code_caption = caption

                out += RstPrinter.code_block(code_caption, code_sample.code, self.code_lang)

        # print members, grouped by inheriting class
        out_members = ""
        for member_list in self.members_by_class:
            members_str = ""

            # Print 'inherited from' for parent classes only.
            if member_list.class_name != self.name:
                parent_link = LinkedClassElement(member_list.class_name, self.env)
                members_str = RstPrinter.paragraph(f"Inherited from {parent_link.to_rst()}")

            for member in member_list.members:
                members_str += RstPrinter.list_element(member.to_rst())
            out_members += RstPrinter.paragraph(members_str)

        # any member data was written
        if out_members:
            out += BaseElement.subsection_title(f"Attributes ({self.members_count})") + out_members

        # print methods section
        if self.methods:
            methods_str = ""
            for method in self.methods:
                methods_str += RstPrinter.list_element(method.to_rst())
            out += BaseElement.subsection_title("Methods") + RstPrinter.paragraph(methods_str)

        # avoid empty class body before meta section to prevent sphinx build warnings
        if not out.strip():
            out += RstPrinter.paragraph("No info available.")

        ################################################################
        # List of meta info attributes
        ################################################################

        meta_items: List[str] = list()

        if self.abstract:
            meta_items.append("Abstract")

        if self.scriptable:
            meta_items.append("Scriptable")

        if self.mutable:
            meta_items.append("Mutable")

        # class parents
        if self.parents:
            elements_str = [LinkedClassElement(parent.name, self.env).to_rst() for parent in self.parents]
            parents_str = " <- ".join(elements_str)
            meta_items.append(f"Parents: {parents_str}")

        # child classes
        if self.children:
            elements_str = [LinkedClassElement(child_name, self.env).to_rst() for child_name in self.children]
            child_classes_str = " ".join(elements_str)
            meta_items.append(f"Children: {child_classes_str}")

        # doc group
        if self.doc_group:
            meta_items.append(f"Group: {LinkedDocGroupElement(self.doc_group, self.env).to_rst()}")

        # schema child group
        if self.child_group and self.child_group_link_kind:
            link = LinkedElement(self.child_group, self.child_group_link_kind, self.env)
            meta_items.append(f"Schema children: {link.to_rst()}")

        if meta_items:
            out += BaseElement.subsection_title("Meta")
            out += RstPrinter.list(meta_items)

        return f"{class_str}{out}"


class EnumeratorElement(BaseElement):
    """ Represent an object property"""

    def __init__(self, name: str, value: str, brief: str, detailed: str, env: Environment):
        super().__init__(name, "enumerator", env)
        self.value: str = value
        self.brief: str = brief
        self.detailed: str = detailed

    def to_rst(self) -> str:
        """
        Create an RST string for an enumerator, containing 'name', 'value' and 'brief' description.
        Does not print the value of an enumeration as it is not helpful from a readers perspective.
        """

        result = RstPrinter.bold(self.name)
        doc_string = self._build_doc_string()
        if doc_string:
            result += f": {doc_string}"

        result += "\n"
        return result


class EnumerationElement(BaseElement):
    """ Enumeration representation as parsed from XML """

    def __init__(self, name: str, attributes: Dict, env: Environment):
        super().__init__(name, "enumeration", env)
        self.attributes = attributes
        self.enumerators: List[EnumeratorElement] = []
        self.parents: List[EnumerationElement] = []

    @property
    def parent_name(self):
        return self.attributes.get(XML.ATTR_PARENT, "")

    def to_rst(self) -> str:
        """ Convert to RST string. """

        out = RstPrinter.h3(self.name)
        out += RstPrinter.paragraph(self.brief)

        if self.detailed:
            out += BaseElement.subsection_title(BaseElement.TITLE_DETAILS) + RstPrinter.paragraph(self.detailed)

        out += BaseElement.subsection_title("Enumerators")

        # May be empty if enumerators are derived from parent only.
        if self.enumerators:
            enumerators_str = ""
            for e in self.enumerators:
                enumerators_str += RstPrinter.list_element(e.to_rst())
            out += RstPrinter.paragraph(enumerators_str)

        # if available, also print all enumerators that origin from parent classes
        if self.parents:
            for parent_enum in self.parents:
                out += RstPrinter.paragraph(
                    "Inherited from %s" % LinkedEnumerationElement(parent_enum.name, self.env).to_rst())
                parent_enumerators = parent_enum.enumerators
                for p in parent_enumerators:
                    out += RstPrinter.list_element(p.to_rst())

                # sphinx: list must end with a new line
                out += "\n"

        return out


class ObjectElement(BaseElement):
    """ Object representation as parsed from XML. """

    def __init__(self, name: str, attributes: Dict, env: Environment):
        super().__init__(name, "object", env)
        self.methods: List[MethodElement] = []  # methods of this object
        self.properties: List[PropertyElement] = []  # properties of this object
        self.children: Dict[str, ObjectElement] = {}  # children of this objects (objects in return, recursive)
        self.attributes = attributes
        self._dynamic_class_name = ""
        self._dynamic_class_namespace = ""

    @property
    def class_name(self):
        return self.attributes.get(XML.ATTR_CLASS, "")

    @property
    def class_namespace(self):
        return self.attributes.get(XML.ATTR_CLASS_NAMESPACE, "")

    @property
    def readonly(self):
        return self.attributes.get(XML.ATTR_READ_ONLY, False)

    @property
    def dynamic_type(self):
        return self.attributes.get(XML.ATTR_DYNAMIC_TYPE, False)

    @property
    def dynamic_class_name(self):
        return self._dynamic_class_name

    @dynamic_class_name.setter
    def dynamic_class_name(self, name: str) -> None:
        self._dynamic_class_name = name

    @property
    def dynamic_class_namespace(self):
        return self._dynamic_class_namespace

    @dynamic_class_namespace.setter
    def dynamic_class_namespace(self, name: str) -> None:
        self._dynamic_class_namespace = name

    def to_rst(self) -> str:
        out = RstPrinter.h3(self.name)

        # external file content
        if self.external_file:
            out += RstPrinter.include(self.external_file)

        # print methods section
        if self.methods:
            out += BaseElement.subsection_title(BaseElement.TITLE_METHODS)
            methods_str = ""
            for method in self.methods:
                out += RstPrinter.list_element(method.to_rst())
            out += RstPrinter.paragraph(methods_str)

        # print properties section
        if self.properties:
            out += BaseElement.subsection_title("Properties")
            properties_str = ""
            for p in self.properties:
                out += RstPrinter.list_element(p.to_rst())
            out += RstPrinter.paragraph(properties_str)

        # print child objects
        if self.children:
            elements_str = [LinkedObjectElement(child_name, self.env).to_rst() for child_name in self.children]
            children_str = " ".join(elements_str)
            out += BaseElement.subsection_title("Meta") + RstPrinter.list_element(f"Children: {children_str}")

        return out


class GroupElement(BaseElement):
    """ Represent a doc group. """

    def __init__(self, name: str, kind: str, env: Environment):
        super().__init__(name, kind, env)
        self.items: Dict[str, LinkedElement] = {}  # all contained classes, objects as linked element

    def add_linked_class(self, name: str) -> None:
        self.items[name] = LinkedClassElement(name, self.env)

    def add_linked_object(self, name: str) -> None:
        self.items[name] = LinkedObjectElement(name, self.env)

    def to_rst(self) -> str:
        """ Print all items, separated by whitespace. """
        out = RstPrinter.h3("%s (%d)" % (self.name, len(self.items.values())))
        out += " ".join([item.to_rst() for item in self.items.values()])
        return out

    def count(self) -> int:
        return len(self.items)

    def contains(self, name: str) -> bool:
        return name in self.items


class DocGroupElement(GroupElement):
    KIND = "docgroup"

    def __init__(self, name: str, env: Environment):
        super().__init__(name, DocGroupElement.KIND, env)


class SchemaGroupElement(GroupElement):
    KIND = "schemagroup"

    def __init__(self, name: str, env: Environment):
        super().__init__(name, SchemaGroupElement.KIND, env)
