"""Typescript definitions and types."""

from abc import abstractmethod, ABC
from typing import List, final, Optional

from modules.typescript.classmodel import ClassModelDefs
from modules.typescript.formatter import *
from modules.typescript.comment import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Printable(ABC):
    """ Class that can be converted to TypeScript string. """

    @abstractmethod
    def to_string(self) -> str:
        """ Convert to TypeScript string representation. """
        raise NotImplementedError


class Property(Printable):
    """ Class member variable or member function ('attribute'). """

    def __init__(self) -> None:
        super().__init__()

    def to_string(self) -> str:
        raise NotImplementedError


class Element(Printable):
    """ Model element that is either declared or exported, depending on scope.
    Stores the current scope ('namespace') as breadcrumbs list.
    """

    KIND_NAMESPACE = "namespace"
    KIND_CLASS = "class"
    KIND_OBJECT = "object"
    KIND_VARIABLE = "variable"

    def __init__(self):
        super().__init__()
        self.toplevel = False
        self.namespaces_path: List[str] = []  # parent namespaces

    def to_string(self) -> str:
        raise NotImplementedError

    @final
    def set_context(self, toplevel: bool) -> None:
        """ Set a context that may affect string conversion. """
        self.toplevel = toplevel

    @final
    def add_namespace_path(self, namespace: str) -> None:
        self.namespaces_path.append(namespace)

    def get_kind(self) -> str:
        raise NotImplementedError

    def get_name(self) -> str:
        raise NotImplementedError

    @final
    def _get_export_type(self) -> str:
        """ Declare on toplevel, export if nested. """
        return "declare" if self.toplevel else "export"


class ModelRoot(Printable):
    """ Declaration model root. """

    def __init__(self):
        super().__init__()
        self.children: List[Element] = []

    @final
    def to_string(self) -> str:
        result_str = ""
        for c in self.children:
            result_str += f"{c.to_string()}\n"

        # Make file end with newline.
        return f"{Formatter.apply_indentation(result_str)}\n"

    @final
    def add_child(self, child: Element) -> None:
        """ Flag added children as top level to handle declare vs export.
         Elements at root level must be 'declared', not 'exported'. """
        self.children.append(child)
        child.set_context(toplevel=True)

    @final
    def find_child(self, name: str, kind: str) -> Optional[Element]:
        """ Lookup a child element by name. """
        for c in self.children:
            if c.get_name() == name and c.get_kind() == kind:
                return c

        return None

    @final
    def get_path(self) -> str:
        return ""


class ScopedElement(Element):
    """ Element type with scope and child elements. """

    def __init__(self):
        super().__init__()
        self.children: List[Element] = []

    @final
    def to_string(self) -> str:
        # recursion end
        if not self.children:
            return "%s \n{ }" % self._get_header()

        return "%s\n{\n%s\n}" % (self._get_header(), self._get_children_as_string())

    def add_child(self, child: Element) -> None:
        """ Children added inside a scope are never toplevel. """
        self.children.append(child)
        child.set_context(toplevel=False)

    def add_property(self, prop: Property) -> None:
        self.children.append(prop)

    def get_kind(self) -> str:
        raise NotImplementedError

    def get_name(self) -> str:
        raise NotImplementedError

    def _get_header(self) -> str:
        return ""

    def _get_children_as_string(self) -> str:
        """
        :return: All children by calling to_string recursively, will indent accordingly.
        """
        children_strings = [c.to_string() for c in self.children]
        return "\n".join(children_strings)


class Namespace(ScopedElement):
    """ 'declare namespace' """

    def __init__(self, name: str) -> None:
        super().__init__()
        self.name = name

    @final
    def get_path(self) -> str:
        """ Return full namespace path. """

        # No parents
        if not self.namespaces_path:
            return self.name

        parents = ".".join(self.namespaces_path)
        return f"{parents}.{self.name}"

    @final
    def add_child(self, child: Element) -> None:
        """ Maintain namespace path. """
        super().add_child(child)
        child.add_namespace_path(self.name)

    @final
    def find_child(self, name: str, kind: str) -> Optional[Element]:
        """ Lookup a child element by name. """
        for c in self.children:
            if c.get_name() == name and c.get_kind() == kind:
                return c

        return None

    @final
    def get_kind(self) -> str:
        return Element.KIND_NAMESPACE

    @final
    def get_name(self) -> str:
        return self.name

    def _get_header(self) -> str:
        return f"{self._get_export_type()} namespace {self.name}"


class ClassDef(ScopedElement):
    """
    Export statement for classmodel 'Model.Class' element.
    Same as ObjectDef but can have a 'parent', used for 'extends'.
    """

    def __init__(self, name: str, namespace: str, parent_name: str, parent_namespace: str,
                 build_comment: bool = False) -> None:
        super().__init__()
        self.name = name
        self.namespace = namespace
        self.parent = parent_name
        self.parent_namespace = parent_namespace
        self.build_comment = build_comment

    def get_kind(self) -> str:
        return Element.KIND_CLASS

    @final
    def get_name(self) -> str:
        return self.name

    def _build_comment(self) -> str:
        """ Create a generic method comment. """
        comment = CommentStringBuilder()
        comment.add_class_brief(self.name)
        return comment.to_string()

    def _get_header_with_comment(self, object_str: str) -> str:
        """ Add optional comment to raw_name. """
        if self.build_comment:
            comment_str = self._build_comment()
            return f"{comment_str}{object_str}"
        else:
            return object_str

    def _get_header(self) -> str:
        if self.parent:
            parent_full_name = self.parent
            # Expose full parent class namespace if it is from a different namespace
            if self.parent_namespace and self.parent_namespace != self.namespace:
                parent_full_name = PathFormatter.to_typescript_namespace(f"{self.parent_namespace}::{self.parent}")

            class_str = f"{self._get_export_type()} class {self.name} extends {parent_full_name}"
            return self._get_header_with_comment(class_str)

        result_str = f"{self._get_export_type()} class {self.name}"
        return self._get_header_with_comment(result_str)


class TypeDef(Printable):
    """ Type definition, supports single or multi type. """

    TS_TYPE_VOID = "void"
    TS_TYPE_BOOLEAN = "boolean"
    TS_TYPE_NUMBER = "number"
    TS_TYPE_ANY = "any"

    def __init__(self, type_value: str, type_names_value: str = "") -> None:
        """
        Construction. Types are converted from ClassModel to TypeScript equivalents.

        @param namespace context in which this type appears
        @param type_value attribute value from classmodel
        @param type_names_value attribute value from classmodel
        """
        super().__init__()
        self.type = self._classmodel_to_typescript_type(type_value)
        self.type_names = self._convert_type_names(type_names_value)

    @final
    def to_string(self) -> str:
        """ Print types as OR concatenated strings or as single type name. """

        # No type names, just return 'type' as TypeScript equivalent type.
        if not self.type_names:
            return self._classmodel_to_typescript_type(self.type)

        # If type is "container", the types specified by type_names
        # are to be exported as an Array type with trailing "[]"
        as_array = self.type == ClassModelDefs.TYPE_CONTAINER
        return TypeDef._type_names_to_string(self.type_names, as_array)

    def _convert_type_names(self, names) -> List[str]:
        """ Split types from a combined string "bool | string", convert to TypeScript equivalents. """
        if not names:
            return []

        classmodel_types_list = [PathFormatter.to_typescript_namespace(x.strip()) for x in names.split('|')]
        return [self._classmodel_to_typescript_type(x) for x in classmodel_types_list]

    @abstractmethod
    def _map_void_type(self) -> str:
        """ Map classmodel 'void' type. """
        raise NotImplementedError

    def _classmodel_to_typescript_type(self, raw_type) -> str:
        """ Map classmodel provided types to TypeScript compatible types. """

        if raw_type == ClassModelDefs.TYPE_VOID:
            return self._map_void_type()
        if raw_type == ClassModelDefs.TYPE_BOOL:
            return TypeDef.TS_TYPE_BOOLEAN
        elif raw_type in ClassModelDefs.TYPE_NUMBERS:
            return TypeDef.TS_TYPE_NUMBER
        elif raw_type == ClassModelDefs.TYPE_VARIANT:
            return TypeDef.TS_TYPE_ANY
        else:
            return raw_type

    @staticmethod
    def _check_unsupported(type_value: str, as_array: bool) -> None:
        """ Perform sanity checks on the combination of type and typeName."""

        # Consider "Return a list of void" an error. Can only happen for
        # return types as argument types are mapping 'void' to 'any'.
        if as_array and type_value == TypeDef.TS_TYPE_VOID:
            raise ValueError("void[] return type")

    @staticmethod
    def _type_names_to_string(type_names: List[str], as_array: bool) -> str:
        """ Convert list of type names, optionally to array. """

        for type_name in type_names:
            TypeDef._check_unsupported(type_name, as_array)

        # append array brackets if needed
        modified_types = [f"{t}[]" for t in type_names] if as_array else type_names
        return " | ".join(modified_types)


class ReturnTypeDef(TypeDef):
    """ TypeDef specialization for use as return value type. """

    def _map_void_type(self) -> str:
        return TypeDef.TS_TYPE_VOID


class ArgTypeDef(TypeDef):
    """ TypeDef specialization for use as method argument type. """

    def _map_void_type(self) -> str:
        return TypeDef.TS_TYPE_ANY


class VariableDef(Element):
    """ declare var [NAME]: [TYPENAME]. """

    def __init__(self, name: str, class_name: str, class_namespace: str) -> None:
        super().__init__()
        self.name = name  # name of this object
        self.class_name = class_name  # type of this object
        self.class_namespace = class_namespace  # namespace of this object type

    @final
    def get_kind(self) -> str:
        return Element.KIND_VARIABLE

    @final
    def get_name(self) -> str:
        return self.name

    @final
    def to_string(self) -> str:
        # Reminder: expects class name which does not require a mapping
        # from native type to TS type such as int -> number, see TypeDef
        # for reference.
        object_class = f"{self.class_namespace}.{self.class_name}" if self.class_namespace else self.class_name
        return f"{self._get_export_type()} var {self.name}: {object_class};"


class Param(Printable):
    """
    Method parameter.

    Does not take a default value into account as argument default values
    are not supported in TypeScript declarations (d.ts).
    """

    def __init__(self, name: str, type_def: TypeDef, default_value: str = ""):
        super().__init__()
        self.name = name
        self.type_def = type_def
        self.default_value = default_value

    @final
    def to_string(self) -> str:
        """ Note: interpret incoming 'vargs' or '...' as variadic but always export as 'vargs'.
        TODO: Replace vargs workaround mapping when classmodel supports variadic arguments.
        """

        type_string = self.type_def.to_string()
        if self.name in ["vargs", "..."]:
            return f"...vargs: {type_string}[]"  # variadic
        elif self.default_value:
            return f"{self.name}?: {type_string}"  # optional
        else:
            return f"{self.name}: {type_string}"  # regular


class Member(Property):
    """ Class member. """

    def __init__(self, name: str, data_type: TypeDef, readonly: bool) -> None:
        super().__init__()
        self.name = name
        self.data_type = data_type
        self.readonly = readonly

    @final
    def to_string(self) -> str:
        output_readonly = "readonly" if self.readonly else ""
        output_type = self.data_type.to_string()
        return f"{output_readonly} {self.name}: {output_type};".strip()


class GuardMember(Property):
    def __init__(self, name: str, namespace: str) -> None:
        super().__init__()
        self.name = name
        self.namespace = namespace

    @final
    def to_string(self):
        """ Separate namespace by underscores A_B_C. """
        if self.namespace:
            adjusted_namespace = self.namespace.replace(".", "_")
            fullname = f"{adjusted_namespace}_{self.name}"
        else:
            fullname = self.name

        return f"private {PathFormatter.to_typescript_guard(fullname)}: never;"


class Method(Property):
    """ Class method. """

    def __init__(self, name: str, return_type: TypeDef, build_comment: bool = False) -> None:
        super().__init__()
        self.name = name
        self.params: List[Param] = []
        self.return_type: TypeDef = return_type
        self.build_comment = build_comment

    @final
    def to_string(self) -> str:
        params = [p.to_string() for p in self.params]
        params_string = ", ".join(params)
        method_str = f"{self.name}: ({params_string}) => {self.return_type.to_string()};"
        if self.build_comment:
            return f"{self._build_comment()}{method_str}"
        else:
            return method_str

    @final
    def add_param(self, param: Param) -> None:
        self.params.append(param)

    def _build_comment(self) -> str:
        """ Create a generic method comment. """
        comment = CommentStringBuilder()
        comment.add_method_brief(self.name)
        for p in self.params:
            comment.add_param(p.name, p.type_def.to_string(), p.default_value)
        comment.add_returns(self.return_type.to_string())
        return comment.to_string()
