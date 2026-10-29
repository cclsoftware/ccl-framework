"""Classmodel XML parser."""

import xml.etree.ElementTree as ElementTree
from typing import List, Dict, Optional

from modules.classmodel.elements import TypeElement, MemberElement, EnumeratorElement, MethodArgumentElement, \
    MethodElement, PropertyElement, ClassElement, EnumerationElement, ObjectElement
from modules.classmodel.model import Model
import logging

from modules.classmodel.elementfilter import ElementFilter
from modules.classmodel.xmldefs import XML

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


class Parser:
    """
    ClassModel XML parser.
    """

    def __init__(self, model: Model, element_filter: ElementFilter) -> None:
        self.model = model
        self.xml_ns = ""
        self.element_filter = element_filter

    def _get_brief(self, element):
        """
        Extract 'brief' from <Model.Documentation> by parsing all <String> children.

        Example XML:

        <Element ... > <-- element
            <Model.Documentation x:id="doc">
                <String x:id="brief" text="default window style: title bar"/>
                (...)
            </Model.Documentation>
            (...)
        </Element>

        :param element: <Model.Documentation> element
        :return: concatenated 'brief' from all <String> children
        """
        out = ""
        for doc_element in element.findall(XML.ELEMENT_MODEL_DOCUMENTATION):
            for string_element in doc_element.findall(XML.ELEMENT_STRING):
                if string_element.attrib[self._get_ns_attribute(XML.ATTR_ID)] == XML.VALUE_BRIEF:
                    out += string_element.attrib[XML.ATTR_TEXT]

        return out

    def _get_detailed(self, element):
        """
        Extract 'brief' from <Model.Documentation> by parsing all <String> children.

        Example XML:

        <Element ... > <-- element
            <Model.Documentation x:id="doc">
                <String x:id="detailed" text="default window style: title bar"/>
                (...)
            </Model.Documentation>
            (...)
        </Element>

        :param element: <Model.Documentation> element
        :return: concatenated 'detailed' from all <String> children
        """
        out = ""
        for doc_element in element.findall(XML.ELEMENT_MODEL_DOCUMENTATION):
            for string_element in doc_element.findall(XML.ELEMENT_STRING):
                if string_element.attrib[self._get_ns_attribute(XML.ATTR_ID)] == XML.VALUE_DETAILED:
                    out += string_element.attrib[XML.ATTR_TEXT]

        return out

    @staticmethod
    def _parse_type_element(element) -> TypeElement:
        """
        Parse an element with "type" and "typeName" attributes to TypeElement.

        Example XML:
            <Model.ReturnValue x:id="retval" type="object" typeName="TimeSignature"/>
        """

        type_value = element.attrib.get(XML.ATTR_TYPE, "ERROR_TYPE")
        type_name_value = element.attrib.get(XML.ATTR_TYPE_NAME, "")
        return TypeElement(type_value, type_name_value)

    def _get_ns_attribute(self, attr: str) -> str:
        """
        Get attribute name with prepended XML namespace.
        Reminder: this method exists due to ElementTree not auto-adding
        namespaces when using attrib to query element attributes.
        """
        return "{" + self.xml_ns + "}" + attr

    def _parse_members(self, element) -> List[MemberElement]:
        """
        Parse all <Model.Member> elements under <List>

        Example XML:

        <List x:id="members"> <-- element
            <Model.Member name="image" type="string"/>
            <Model.Member name="icon" type="string"/>
            (...)
        </List>

        <Model.Member name="options" type="string" typeName="enum">

        :param element: <List> element parenting <Model.Member> elements
        :return: list of member dicts
        """
        members = []
        unique_member_names = set()
        for child in element.findall(XML.ELEMENT_MODEL_MEMBER):
            type_element = Parser._parse_type_element(child)

            member_name = child.attrib.get(XML.ATTR_NAME, None)
            if member_name is None:
                continue

            # class model may list redundant members resulting from inheritance, ignore here
            if member_name in unique_member_names:
                continue

            members.append(MemberElement(member_name,
                                         type_element,
                                         child.attrib.get(XML.ATTR_READ_ONLY, False),
                                         self._get_brief(child),
                                         self._get_detailed(child)))

            unique_member_names.add(member_name)

        return members

    def _parse_enumerators(self, element) -> List[EnumeratorElement]:
        """
        Parse all <Model.Enumerator> which are children of a <Model.Enumeration>

        Example XML:

        <List x:id="enumerators"> <-- element
            <Model.Enumerator name="windowstyle" value="2">
                <Model.Documentation x:id="doc">
                    <String x:id="brief" text="default window style: title bar"/>
                </Model.Documentation>
            </Model.Enumerator>
        (...)
        </List>

        :param element: <List> element
        :return: list of all Enumerators as dicts
        """
        enumerators = []
        for child in element.findall(XML.ELEMENT_MODEL_ENUMERATOR):
            enumerators.append(EnumeratorElement(child.attrib[XML.ATTR_NAME],
                                                 child.attrib[XML.ATTR_VALUE],
                                                 self._get_brief(child),
                                                 self._get_detailed(child)
                                                 ))

        return enumerators

    def _parse_methods(self, element) -> List[MethodElement]:
        """
        Parse all <Model.Method> under <List> of methods element.

        Example XML:

        <List x:id="methods"> <-- element
            <Model.Method name="UID">
                <Model.ReturnValue x:id="retval" type="void"/>
                <List x:id="args">
                    <Model.MethodArg name="object_or_string" type="void"/>
                    (...)
                </List>
            </Model.Method>
            <Model.Method name="foo">
            (...)
        </List>

        :param element: <List> methods element
        :return: list of all methods read from XML
        """

        methods = []
        for child in element.findall(XML.ELEMENT_MODEL_METHOD):
            # ReturnValue
            return_type = None
            for return_value_element in child.findall(XML.ELEMENT_MODEL_RETURN_VALUE):
                return_type = Parser._parse_type_element(return_value_element)
            # MethodArg list
            arguments = []
            for arg_list_element in child.findall(XML.ELEMENT_LIST):
                if arg_list_element.attrib[self._get_ns_attribute(XML.ATTR_ID)] == XML.VALUE_ARGS:
                    for arg_element in arg_list_element.findall(XML.ELEMENT_MODEL_METHOD_ARG):
                        arguments.append(
                            MethodArgumentElement(arg_element.attrib[XML.ATTR_NAME],
                                                  Parser._parse_type_element(arg_element),
                                                  arg_element.attrib.get(XML.ATTR_DEFAULT_VALUE, "")))
            methods.append(MethodElement(child.attrib[XML.ATTR_NAME], return_type, arguments))

        return methods

    @staticmethod
    def _parse_class_attributes(target: ClassElement, element: ElementTree.Element) -> None:
        """
        Set target members from <Attribute> list.

        Example XML:

        <Attributes x:id="attributes"> <-- element
            <Attribute id="Class:DocGroup" value="Styles"/>
            <Attribute id="Class:ChildGroup" value="SomeGroup"/>
        </Attributes>

        """

        for child in element.findall(XML.ELEMENT_ATTRIBUTE):
            attr_id = child.attrib.get(XML.ATTR_ID)
            attr_value = child.attrib.get(XML.ATTR_VALUE, "")
            if attr_id == XML.VALUE_DOC_GROUP:
                target.doc_group = attr_value
            elif attr_id == XML.VALUE_SCHEMA_GROUPS:
                # The schema groups attribute may be an empty string, implying the class
                # is not in any group. Filter these out via split () without args, so they
                # do not appear in the output.
                for group in attr_value.split():
                    target.schema_groups.append(group.strip())

                # Save whether schema groups were provided for this class in the XML.
                # This is relevant to resolve inherited schema groups later on.
                # Reminder: explicit groups can be an empty string.
                target.has_schema_groups_attr = True

            elif attr_id == XML.VALUE_CHILD_GROUP:
                target.child_group = attr_value
            else:
                logging.warning(f"skipped class attribute {attr_id}")

    def _parse_properties(self, element) -> List[PropertyElement]:
        """ Read all Model.Property elements attached to 'element'. """
        properties = []
        for child in element.findall(XML.ELEMENT_MODEL_PROPERTY):
            type_element = Parser._parse_type_element(child)
            properties.append(
                PropertyElement(child.get(XML.ATTR_NAME, ""), type_element))

        return properties

    def _parse_children(self, element) -> Dict[str, ObjectElement]:
        """ Recursively parse all Model.Object elements that are children of 'element' """
        children_objects = dict()
        for child in element.findall(XML.ELEMENT_MODEL_OBJECT):
            child_object = self._parse_object(child)
            self._register_element(children_objects, child_object)

        return children_objects

    def parse_class_links(self, element) -> List[str]:
        """
        Parse links from a class documentation

        <Model.Documentation x:id="doc">
            <String x:id="brief" text="Defines an alignment in a Visual Style"/>
            <List x:id="links"> <-- element
                <String text="Style"/>
                <String text="TextBox"/>
                (...)
            </List>
        </Model.Documentation>
        """
        links = []
        for child in element.findall(XML.ELEMENT_STRING):
            # assume the link refers to a 'class' type element
            linked_class_name = child.attrib[XML.ATTR_TEXT]

            if self.element_filter.skip(element_type="class", element_name=linked_class_name):
                logging.debug("skipped class link to element '%s.%s'" % ("class", linked_class_name))
            else:
                links.append(child.attrib[XML.ATTR_TEXT])

        return links

    def _parse_class(self, element) -> ClassElement:
        """
        Parse <Model.Class> element: Extract brief, detailed, members and methods.

        Example XML (methods only, no members):

        <Model.Class name="Attributes" scriptable="1" parent="Object"> <-- element
            <List x:id="methods">
                <Model.Method name="getAttribute">
                    <Model.ReturnValue x:id="retval" type="void"/>
                </Model.Method>
                ...
            </List>
            (...)
        </Model.Class>

        :param element: <Model.Class> XML node
        :return: class information represented as RST conform string
        """

        class_name = element.attrib.get(XML.ATTR_NAME, "")
        class_result = ClassElement(name=class_name, attributes=element.attrib)

        for child in element:
            if child.tag == XML.ELEMENT_MODEL_DOCUMENTATION:
                # <Model.Documentation>: parse class brief and details
                for string_element in child:
                    attr_id = string_element.attrib[self._get_ns_attribute(XML.ATTR_ID)]
                    if attr_id == XML.ATTR_BRIEF:
                        class_result.brief = string_element.attrib[XML.ATTR_TEXT]
                    elif attr_id == XML.ATTR_DETAILED:
                        class_result.set_detailed(string_element.attrib[XML.ATTR_TEXT])
                    elif attr_id == XML.ATTR_CODE:
                        class_result.set_code(string_element.attrib[XML.ATTR_TEXT])
                    elif attr_id == XML.ATTR_LANGUAGE:
                        class_result.code_lang = string_element.attrib[XML.ATTR_TEXT]
                    elif attr_id == XML.VALUE_LINKS:
                        class_result.links = self.parse_class_links(string_element)

            elif child.tag == XML.ELEMENT_LIST:
                # <List>: expect either members or methods
                attr_id = child.attrib[self._get_ns_attribute(XML.ATTR_ID)]
                if attr_id == XML.VALUE_MEMBERS:
                    class_result.members = self._parse_members(child)
                if attr_id == XML.VALUE_METHODS:
                    class_result.methods = self._parse_methods(child)
            elif child.tag == XML.ELEMENT_ATTRIBUTES:
                attr_id = child.attrib[self._get_ns_attribute(XML.ATTR_ID)]
                if attr_id == XML.VALUE_ATTRIBUTES:
                    Parser._parse_class_attributes(target=class_result, element=child)
            else:
                logging.warning("found unsupported element '%s' while parsing Model.Class" % child.tag)

        # Reminder: needs post-processing from model, see model.enrich()

        return class_result

    def _parse_enumeration(self, element) -> Optional[EnumerationElement]:
        """
        Parse <Model.Enumeration> element: Extract brief, detailed, enumerators and documentation

        Example XML:

        <Model.Enumeration name="TriggerView.gesturepriority"> <-- element
            <Model.Documentation x:id="doc">
                <String x:id="brief" text=""/>
            </Model.Documentation>
            <List x:id="enumerators">
                <Model.Enumerator name="low" value="0">
                    <Model.Documentation x:id="doc">
                        <String x:id="brief" text=""/>
                    </Model.Documentation>
                </Model.Enumerator>
                (...)
            </List>
        </Model.Enumeration>

        :param element: element of type <Model.Enumeration>
        :return: the enumeration element as rst string
        """

        # Do not check for 'is not element' here as it will
        # return True if the element has no child elements.
        if element is None:
            return None

        enum_name = element.attrib.get(XML.ATTR_NAME, "")
        enum_result = EnumerationElement(name=enum_name, attributes=element.attrib)

        for child in element:
            # Documentation: brief and details if available
            if child.tag == XML.ELEMENT_MODEL_DOCUMENTATION:
                for string_element in child:
                    # Can be 'brief' or 'detailed'
                    attr = string_element.attrib[self._get_ns_attribute(XML.ATTR_ID)]
                    if attr == XML.ATTR_BRIEF:
                        enum_result.brief = string_element.attrib[XML.ATTR_TEXT]
                    elif attr == XML.ATTR_DETAILED:
                        enum_result.set_detailed(string_element.attrib[XML.ATTR_TEXT])
            # List of enumerators
            elif child.tag == XML.ELEMENT_LIST:
                if child.attrib[self._get_ns_attribute(XML.ATTR_ID)] == XML.VALUE_ENUMERATORS:
                    enum_result.enumerators = self._parse_enumerators(child)

        # for later display, sort the enumerators by name
        enum_result.enumerators.sort(key=lambda x: x.name, reverse=False)
        return enum_result

    def _parse_object(self, element) -> ObjectElement:
        """
        _parse_object

        Example XML:

        <List x:id="objects">
            <Model.Object name="Host"> <-- element
                <List x:id="methods">
                    <Model.Method name="UID">
                        <Model.ReturnValue x:id="retval" type="void"/>
                    </Model.Method>
                    <Model.Method name="Url">
                        <Model.ReturnValue x:id="retval" type="void"/>
                    </Model.Method>
                    (...)
        """

        object_name = element.attrib.get(XML.ATTR_NAME, "")
        object_result = ObjectElement(name=object_name, attributes=element.attrib)

        # parse <List> elements that are direct children of root
        for list_element in element.findall(XML.ELEMENT_LIST):
            list_type = list_element.attrib[self._get_ns_attribute(XML.ATTR_ID)]
            if list_type == XML.VALUE_METHODS:
                # process list parenting Model.Method
                object_result.methods = self._parse_methods(list_element)
            elif list_type == XML.VALUE_CHILDREN:
                object_result.children = self._parse_children(list_element)
            elif list_type == XML.VALUE_PROPERTIES:
                object_result.properties = self._parse_properties(list_element)
            else:
                logging.warning("found unexpected <List> element ID while parsing Model.Object element %s" % list_type)

        # model objects have no list of <Attribute>

        return object_result

    def _build_class_hierarchy(self, xml_root) -> Dict[str, str]:
        """
        Build a dictionary of class->parent tuples, keyed by namespace-qualified name (see
        ClassElement.qualified_name) so that classes sharing a bare name across different
        namespaces get distinct hierarchy entries. Registers 'None' as parent if a class has
        no parent set.

        :param: xml_root class model XML file root node
        :return: a dictionary of qualified class:parent tuples
        """
        cache = dict()

        for class_element in xml_root.iter(XML.ELEMENT_MODEL_CLASS):
            class_name = class_element.attrib[XML.ATTR_NAME]
            if self.element_filter.skip(element_type="class", element_name=class_name):
                logging.debug(
                    "skipped class hierarchy reference for element '%s.%s'" % ("class", class_name))
                continue

            class_namespace = class_element.attrib.get(XML.ATTR_NAMESPACE, "")
            qualified_name = f"{class_namespace}::{class_name}" if class_namespace else class_name

            parent_name = class_element.attrib.get(XML.ATTR_PARENT, None)
            if parent_name:
                parent_namespace = class_element.attrib.get(XML.ATTR_PARENT_NAMESPACE, "")
                parent_name = f"{parent_namespace}::{parent_name}" if parent_namespace else parent_name

            cache[qualified_name] = parent_name

        return cache

    def _register_element(self, target_dict, element, key: Optional[str] = None) -> None:
        """
        Add an element to 'target_dict' if the element is not to be excluded.

        :param key: dict key to store the element under, defaults to 'element.name'.
        """

        if self.element_filter.skip(element_type=element.kind, element_name=element.name):
            logging.debug("skipped registration for element '%s.%s'" % (element.kind, element.name))
        else:
            target_dict[key if key is not None else element.name] = element

    def _register_class(self, class_element: ClassElement) -> None:
        """
        Register a parsed class under its namespace-qualified name (see
        ClassElement.qualified_name), so classes are always identified by the combination of
        name and namespace. Classmodel files may legitimately reuse the same bare class name
        in different namespaces (e.g. a generic global "Element" alongside
        "CCL::Gui::Element"); qualifying the key means those coexist instead of the
        second one being silently dropped. A genuine redefinition (same name *and* namespace)
        is still treated as redundant and skipped.

        Bare-name lookups (member/argument/return types, "\\see" links, dynamic object types
        - none of which carry namespace information) go through Model.lookup_class() instead
        of a direct dict access.
        """

        key = class_element.qualified_name
        if key in self.model.classes:
            logging.warning("skipped redundant class definition '%s'" % key)
            return

        self._register_element(self.model.classes, class_element, key=key)

    @staticmethod
    def _extract_x_namespace_value(input_file: str) -> str:
        """
        Extract :x namespace value from XML the hard way as ElementTree does not provide access to it.

        Example:
        * Input line: <Model.ClassRepository xmlns:x="https://ccl.dev/xml" name="Surface Elements">
        * Output: https://ccl.dev/xml
        """
        attrib_search = "xmlns:x="
        with open(input_file, encoding='utf-8') as f:
            for line in f:
                if attrib_search not in line:
                    continue
                # strip beginning part (+ 1, include first ")
                result = line[line.find(attrib_search) + len(attrib_search) + 1:]
                # strip end part after "
                return result[0:result.find('\"')]
        return ""

    def parse_file(self, filename: str) -> None:
        """
        Parse *classModel file XML, store data in model.

        Example XML:

        <?xml version="1.0" encoding="UTF-8"?>
            <Model.ClassRepository xmlns:x="https://ccl.dev/xml" name="Core Skin Elements">
                <List x:id="classes">
                    <Model.Class name="AlignView" parent="ContainerView">
                    <List x:id="members">
                        <Model.Member name="textalign" type="string"/>
                    </List>
                </Model.Class>
                <List x:id="enums">
                (...)

        :param filename: classModel file to parse
        :return: single string containing the classModel file in RST format
        """

        self.xml_ns = self._extract_x_namespace_value(filename)
        root = ElementTree.parse(filename).getroot()
        if root is None:
            return

        # build dictionary to resolve class hierarchies
        self.model.class_hierarchy = self._build_class_hierarchy(root)
        self.model.root_name = root.attrib.get(XML.ATTR_NAME, filename)

        # parse <List> elements that are direct children of root
        for list_element in root.findall(XML.ELEMENT_LIST):
            # create a global section for each <List> element (classes, enums)
            list_type = list_element.attrib[self._get_ns_attribute(XML.ATTR_ID)]
            if list_type == XML.VALUE_CLASSES:
                for child in list_element.findall(XML.ELEMENT_MODEL_CLASS):
                    class_element = self._parse_class(child)
                    self._register_class(class_element)
            elif list_type == XML.VALUE_ENUMS:
                for child in list_element.findall(XML.ELEMENT_MODEL_ENUM):
                    enumeration_element = self._parse_enumeration(child)
                    if enumeration_element.name in self.model.enumerations:
                        logging.warning(
                            "skipped redundant enumeration definition '%s'" % enumeration_element.name)
                        continue
                    self._register_element(self.model.enumerations, enumeration_element)
            elif list_type == XML.VALUE_OBJECTS:
                for child in list_element.findall(XML.ELEMENT_MODEL_OBJECT):
                    object_element = self._parse_object(child)
                    self._register_element(self.model.objects, object_element)
            else:
                logging.warning("found unexpected element while parsing file %s" % filename)

        # Enrich data.
        self.model.post_process()
