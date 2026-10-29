"""Element classes module."""

from dataclasses import dataclass
from typing import Dict, List, Optional

from modules.classmodel.xmldefs import XML

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


def _replace_crlf(text: str) -> str:
    """ Replace CRLF with LF to fix superfluous new lines when parsing classmodel files
    that originate from a Windows host. """
    return text.replace("\r\n", "\n")


class BaseElement:
    """ Base class for class, enumeration or object Model elements. """

    def __init__(self, name: str, kind: str) -> None:
        self.name: str = name  # element name (e.g.: class or method name)
        self.attributes: dict = {}  # all XML element attributes
        self.brief: str = ""  # class brief description
        self.detailed: str = ""  # class detailed description
        self.kind = kind

    def set_detailed(self, details: str) -> None:
        """ Replace CRLFs as they cause unwanted additional paragraphs. """
        self.detailed = _replace_crlf(details)

    def build_doc_string(self) -> str:
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


class TypeElement:
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

    def is_enum(self) -> bool:
        """ Check typeName over type! """
        return self.type_name == XML.VALUE_TYPE_ENUM

    def is_object(self) -> bool:
        """ Check type over typeName. """
        return self.type == XML.VALUE_TYPE_OBJECT


class MethodArgumentElement(BaseElement):
    """ Represents a method argument """

    def __init__(self, name: str, type_element: TypeElement, default_value: str):
        super().__init__(name, "method-arg")
        self.type_element: TypeElement = type_element
        self.default_value: str = default_value


class MethodElement(BaseElement):
    """ Represent a class or object method"""

    def __init__(self, name: str, return_type: TypeElement, arguments: List[MethodArgumentElement]):
        super().__init__(name, "method")
        self.return_type: TypeElement = return_type
        self.arguments: List[MethodArgumentElement] = arguments


class PropertyElement(BaseElement):
    """ Represent an object property"""

    def __init__(self, name: str, return_type: TypeElement):
        super().__init__(name, "property")
        self.return_type: TypeElement = return_type


class MemberElement(BaseElement):
    """ Represent an object member. """

    def __init__(self, name: str, type_element: TypeElement, readonly: bool, brief: str, detailed: str):
        super().__init__(name, "member")
        self.type_element: TypeElement = type_element
        self.brief: str = brief
        self.detailed: str = detailed
        self.readonly: bool = readonly
        self.inherited: bool = False

    def is_enumeration(self) -> bool:
        return self.type_element.is_enum()

    def is_object_type(self) -> bool:
        return self.type_element.is_object()


class ClassElement(BaseElement):
    """ Model.Class representation as parsed from XML. """

    KIND = "class"

    @dataclass
    class CodeSample:
        brief: str  # code snippet description
        code: str  # code snippet

    def __init__(self, name: str, attributes: Dict):
        super().__init__(name, ClassElement.KIND)
        self.attributes = attributes
        self.members: List[MemberElement] = []
        self.methods: List[MethodElement] = []
        self.parents: List[ClassElement] = []  # names of parent classes
        self.children: List[str] = []  # names of children classes
        self.code_samples: List[ClassElement.CodeSample] = []
        self.code_lang: str = ""  # code block language (cpp, xml, ...) for all code snippets
        self.links: List[str] = []  # references from doxygen code (\see ...)
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

    @property
    def namespace(self):
        return self.attributes.get(XML.ATTR_NAMESPACE, "")

    @property
    def qualified_name(self) -> str:
        """ Namespace-qualified registry key ("Namespace::Name"), used to disambiguate
        classes that share a bare name across different namespaces. This is an internal
        lookup key, not a user-facing identifier (see PathFormatter.build_full_name for
        the TypeScript-facing full name). """
        return f"{self.namespace}::{self.name}" if self.namespace else self.name

    @property
    def qualified_parent_name(self) -> str:
        """ Namespace-qualified lookup key for this class's parent, mirroring
        'qualified_name'. Empty if the class has no parent. """
        if not self.parent_name:
            return ""
        return f"{self.parent_namespace}::{self.parent_name}" if self.parent_namespace else self.parent_name

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
        for line in _replace_crlf(text).splitlines(True):
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


class EnumeratorElement(BaseElement):
    """ Represent an object property"""

    def __init__(self, name: str, value: str, brief: str, detailed: str):
        super().__init__(name, "enumerator")
        self.value: str = value
        self.brief: str = brief
        self.detailed: str = detailed


class EnumerationElement(BaseElement):
    """ Enumeration representation as parsed from XML """

    def __init__(self, name: str, attributes: Dict):
        super().__init__(name, "enumeration")
        self.attributes = attributes
        self.enumerators: List[EnumeratorElement] = []
        self.parents: List["EnumerationElement"] = []

    @property
    def parent_name(self):
        return self.attributes.get(XML.ATTR_PARENT, "")

    @staticmethod
    def qualified_name(class_name: str, enum_name: str) -> str:
        """ Registry key ("Class.Enum") an enumeration is registered/looked up under. """
        return f"{class_name}.{enum_name}"


class ObjectElement(BaseElement):
    """ Object representation as parsed from XML. """

    def __init__(self, name: str, attributes: Dict):
        super().__init__(name, "object")
        self.methods: List[MethodElement] = []  # methods of this object
        self.properties: List[PropertyElement] = []  # properties of this object
        self.children: Dict[str, "ObjectElement"] = {}  # children of this objects (objects in return, recursive)
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
