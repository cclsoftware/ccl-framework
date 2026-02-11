"""Unit tests for typescript module."""

import unittest

from modules.typescript.elements import ReturnTypeDef, ArgTypeDef, Param, Member, Method, ClassDef, Namespace, \
    ModelRoot

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ReturnTypeDefElementTest(unittest.TestCase):

    def test_to_string(self):
        """ ReturnTypeDef class string conversion test. """
        type_def = ReturnTypeDef("bool", "")
        self.assertEqual(type_def.to_string(), "boolean")
        type_def = ReturnTypeDef("", "double | boolean")
        self.assertEqual("number | boolean", type_def.to_string())
        type_def = ReturnTypeDef("variant", "")
        self.assertEqual(type_def.to_string(), "any")

        # 'void' is preserved as 'void
        type_def = ReturnTypeDef("void", "")
        self.assertEqual(type_def.to_string(), "void")

        # container type
        type_def = ReturnTypeDef("container", "Object")
        self.assertEqual(type_def.to_string(), "Object[]")
        type_def = ReturnTypeDef("container", "Object | Parameter")
        self.assertEqual(type_def.to_string(), "Object[] | Parameter[]")

    def test_unsupported_types(self):
        """ Value error raises on list of void return type. """
        type_def = ReturnTypeDef("container", "void")
        with self.assertRaises(ValueError):
            type_def.to_string()


class ArgTypeDefElementTest(unittest.TestCase):

    def test_to_string(self):
        """ ArgTypeDef class string conversion test. """
        type_def = ArgTypeDef("", "bool")
        self.assertEqual(type_def.to_string(), "boolean")
        type_def = ArgTypeDef("", "double | boolean")
        self.assertEqual(type_def.to_string(), "number | boolean")
        type_def = ArgTypeDef("", "variant")
        self.assertEqual(type_def.to_string(), "any")
        type_def = ArgTypeDef("variant", "")
        self.assertEqual(type_def.to_string(), "any")

        # 'void' translates to 'any'
        type_def = ArgTypeDef("void", "")
        self.assertEqual(type_def.to_string(), "any")


class ParamElementTest(unittest.TestCase):

    def test_to_string(self):
        """ Param class string conversion test. """
        param = Param("name", ArgTypeDef("string", ""))
        self.assertEqual(param.to_string(), "name: string")
        param = Param("name", ArgTypeDef("string", ""), "defaultvalue")
        self.assertEqual(param.to_string(), "name?: string")
        param = Param("name", ArgTypeDef("void", ""))
        self.assertEqual(param.to_string(), "name: any")


class MemberElementTest(unittest.TestCase):

    def test_to_string(self):
        """ Member class string conversion test w/o readonly """
        member = Member("name", ArgTypeDef("variant", ""), False)
        self.assertEqual(member.to_string(), "name: any;")
        member = Member("name", ArgTypeDef("string", ""), True)
        self.assertEqual(member.to_string(), "readonly name: string;")


class MethodElementTest(unittest.TestCase):

    def test_to_string(self):
        """ Method class string conversion test. """
        method = Method("getCount", ReturnTypeDef("int", ""))
        self.assertEqual(method.to_string(), "getCount: () => number;")

    def test_to_string_with_single_param(self):
        """ Method class with a single parameter converts to string as expected. """
        param_def = Param("value", ArgTypeDef("int", ""))
        method = Method("setValue", ReturnTypeDef("void", ""))
        method.add_param(param_def)
        self.assertEqual(method.to_string(), "setValue: (value: number) => void;")

    def test_to_string_with_multiple_param(self):
        """ Method class with multiple parameters converts to string correctly. """

        # param defs
        value_param_def = Param("value", ArgTypeDef("int", ""))
        text_param_def = Param("text", ArgTypeDef("string", ""))

        # define method definition to convert, add params
        method = Method("setFields", ReturnTypeDef("void", ""))
        method.add_param(value_param_def)
        method.add_param(text_param_def)

        self.assertEqual(method.to_string(), "setFields: (value: number, text: string) => void;")

    def test_to_string_with_multiple_param_comment(self):
        """ Method class with multiple parameters converts to string correctly, including comment. """

        # param defs
        value_param_def = Param("value", ArgTypeDef("int", ""))
        text_param_def = Param("text", ArgTypeDef("string", ""))

        # define method definition to convert, add params
        method = Method("setFields", ReturnTypeDef("void", ""), True)
        method.add_param(value_param_def)
        method.add_param(text_param_def)

        expected = "/**\n * Class script method 'setFields()'.\n * \n * @param value  type: number\n * @param text  type: string\n * @returns void\n */\nsetFields: (value: number, text: string) => void;"
        self.assertEqual(method.to_string(), expected)


class ClassDefElementTest(unittest.TestCase):

    def test_to_string(self):
        """ ClassDef class string conversion test. """
        # No parent
        class_def = ClassDef(name="Foo", namespace="", parent_name="", parent_namespace="", build_comment=False)
        self.assertEqual(class_def.to_string(), "export class Foo \n{ }")
        # Parent
        class_def = ClassDef(name="Foo", namespace="", parent_name="Parent", parent_namespace="")
        self.assertEqual(class_def.to_string(), "export class Foo extends Parent \n{ }")

    def test_to_string_with_comment(self):
        """ ClassDef class string conversion test, including comment. """
        class_def = ClassDef(name="Foo", namespace="", parent_name="", parent_namespace="", build_comment=True)
        expected = "/**\n * Script class 'Foo'.\n */\nexport class Foo \n{ }"
        self.assertEqual(class_def.to_string(), expected)


class NamespaceElementTest(unittest.TestCase):

    def test_to_string(self):
        """ Namespace class string conversion test. """
        namespace = Namespace("Foo")
        namespace.set_context(True)
        self.assertEqual(namespace.to_string(), "declare namespace Foo \n{ }")
        namespace.set_context(False)
        self.assertEqual(namespace.to_string(), "export namespace Foo \n{ }")


class DeclarationModelTest(unittest.TestCase):

    def test_to_string(self):
        """ ModelRoot elements translates correctly to string. """

        expected = "declare namespace CCL\n{\n  export class Param extends IParameter\n  {\n    readonly value: number;\n    isActive: () => number | boolean;\n    setActive: () => number;\n  }\n}\n"

        # build test document structure
        namespace = Namespace("CCL")
        namespace.set_context(toplevel=True)

        # create a class with members, methods
        param_class = ClassDef(name="Param", namespace="", parent_name="IParameter", parent_namespace="")
        param_class.add_property(Member("value", ArgTypeDef("number", ""), True))
        param_class.add_property(Method("isActive", ArgTypeDef("", "float | bool")))
        method_with_param = Method("setActive", ReturnTypeDef("int", ""))
        param_class.add_property(method_with_param)

        # add namespace to doc, as root element
        namespace.add_child(param_class)
        doc = ModelRoot()
        doc.add_child(namespace)

        # test: convert to string, verify against expectation
        self.assertEqual(doc.to_string(), expected)
