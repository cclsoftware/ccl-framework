"""Typescript declaration file structure builder."""

import logging
import re
from typing import TypeVar, Generic, Dict, Union

from modules.classmodel.elements import ClassElement, ObjectElement, TypeElement
from modules.classmodel.model import Model
from modules.typescript.elements import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class DtsElementFilter:

    def __init__(self, classes: List[str], objects: List[str]) -> None:
        self.allowed_classes = classes
        self.allowed_objects = objects

    def match(self, element_kind: str, element_name: str) -> bool:
        """ Check if kind/name combination should be discarded."""

        if element_kind == Element.KIND_CLASS:
            return self._match(element_name, self.allowed_classes)
        elif element_kind == Element.KIND_OBJECT:
            return self._match(element_name, self.allowed_objects)

        logging.error(f"filter: unexpected element {element_kind}.{element_name}")

        return False

    @staticmethod
    def _match(name: str, container) -> bool:
        for c in container:
            if "*" in c:
                if re.search(c, name):
                    return True
            else:
                if c == name:
                    return True

        return False

    def matching_objects(self, container: Dict) -> List:
        return self._matching_items(Element.KIND_OBJECT, container)

    def matching_classes(self, container: Dict) -> List:
        return self._matching_items(Element.KIND_CLASS, container)

    def _matching_items(self, kind: str, container: Dict) -> List:
        """ Filter list, return as list. """
        return [c for c in container.values() if self.match(kind, c.name)]


class DeclarationBuilder:
    TypeDefType = TypeVar("TypeDefType", bound=TypeDef)
    NamespaceBucket = TypeVar("NamespaceBucket", bound=Union[Namespace, ModelRoot])

    def __init__(self, model: Model, root_namespace: str, element_filter: DtsElementFilter,
                 build_comments: bool, app_name: str) -> None:
        self.model = model
        self.element_filter = element_filter
        self.build_comments = build_comments  # build comments for elements that support them
        self.root_namespace = root_namespace
        self.app_name = app_name  # prefix for app level dynamic types

    def _add_methods(self, element: ScopedElement, model_element: Union[ClassElement, ObjectElement],
                     namespace: str, build_comments: bool) -> None:
        """ Parse methods provided by element and add TypeScript equivalent members. """
        for method in model_element.methods:
            type_element = method.return_type
            type_def = self._build_type_def(ReturnTypeDef, namespace, type_element)
            method_def = Method(method.name, type_def, build_comments)

            for arg in method.arguments:
                type_element = arg.type_element
                arg_type_def = self._build_type_def(ArgTypeDef, namespace, type_element)
                param = Param(arg.name, arg_type_def, arg.default_value)
                method_def.add_param(param)

            element.add_property(method_def)

    def _lookup_class_namespace(self, class_name: str) -> Optional[str]:
        """ Lookup namespace of 'class_name' safely.
        TODO: remove? namespace should always be provided (not for arg types though)
        """

        if not class_name:
            return ""

        class_element = self.model.classes.get(class_name, None)
        return "" if class_element is None else class_element.namespace

    def _resolve_type_namespaces(self, scope: str, type_name: str) -> str:
        """ Add namespaces to typenames string, example: 'UID | str' -> 'CCL.UID | str'
        Only affects type names that are classes registered in the model.
        """

        result = []
        type_name_tokens = [s.strip() for s in type_name.split("|")]
        for type_name in type_name_tokens:
            namespace = self._lookup_class_namespace(type_name)
            if namespace and namespace != scope:
                result.append(f"{namespace}.{type_name}")
            else:
                result.append(type_name)

        return " | ".join(result)

    def _build_type_def(self, constructed_type: Generic[TypeDefType], namespace: str,
                        element: TypeElement) -> TypeDefType:
        """ Build TypeDef based object via generic type argument. Resolve type names
        corresponding to current namespace scope. """

        type_names = self._resolve_type_namespaces(scope=namespace, type_name=element.type_name)
        return constructed_type(element.type, type_names)

    def _build_class_def(self, class_element: ClassElement, build_comments: bool,
                         top_level_namespace_bucket: NamespaceBucket) -> ClassDef:
        """
        Build typescript class definition from classmodel class element attributes.

        :param class_element: class element as parsed from classmodel XML file
        :param build_comments: add generic documentation
        :return: typescript class definition, add to namespace!
        """

        parent_class_namespace = ""
        parent_class_name = class_element.parent_name
        if parent_class_name:
            parent_class_namespace = class_element.parent_namespace
            # Ideally, this information is always provided by the classmodel file. Be tolerant
            # here and look for a matching class in the model class repository. May yield
            # false positive.
            if not parent_class_namespace:
                logging.warning(
                    f"missing parent namespace for class '{class_element.name}' parent '{parent_class_name}'")
                parent_class_namespace = self._lookup_class_namespace(parent_class_name)
                logging.error(
                    f"parent class namespace not found for class '{class_element.name}' parent '{parent_class_name}'")

        # For nested classes, the nesting will already be included in the namespace
        # by the namespace so only use the class basename here.
        # Example:
        #   namespace = CCL.ScriptingHost
        #   class name = ScriptingHost.Console
        #   class base name = Console
        class_basename = PathFormatter.get_class_basename(class_element.name)
        class_def = ClassDef(class_basename, class_element.namespace, parent_class_name, parent_class_namespace,
                             build_comments)

        guard_def = GuardMember(name=PathFormatter.get_class_basename(class_element.name)
                                , namespace=top_level_namespace_bucket.get_path())
        class_def.add_property(guard_def)

        # Generate class properties.
        for member in class_element.members:
            type_element = member.type_element
            type_def = self._build_type_def(ArgTypeDef, class_element.namespace, type_element)
            member_def = Member(member.name, type_def, member.readonly)
            class_def.add_property(member_def)

        # Generate class methods.
        self._add_methods(element=class_def, model_element=class_element, namespace=class_element.namespace,
                          build_comments=build_comments)
        return class_def

    @staticmethod
    def _get_or_create_namespace_bucket(root: ModelRoot, ns_path: str) -> NamespaceBucket:
        """ Lookup namespace object for given namespace. """

        # No namespace, use root.
        if not ns_path:
            return root

        parent = root
        ns = None

        # Resolve namespace hierarchy.
        for token in PathFormatter.tokenize_namespace(ns_path):
            ns = parent.find_child(token, Element.KIND_NAMESPACE)
            if ns is None:
                ns = Namespace(token)
                parent.add_child(ns)
            parent = ns

        return ns

    def _build_classes(self, namespace_bucket: NamespaceBucket) -> None:
        """
        Export classes. Resolve nested classes by introducing a namespace
        for the outer class type.

        Example:

          class = "Foo.ChildClass" in outer namespace "Bar" with Foo
          having a property "ChildClass" of type Foo.ChildClass.

        declare namespace Bar
        {
            export class Foo
            {
                ChildProperty: Foo.ChildClass
            }

            export namespace Foo
            {
                export class ChildClass
                {
                }
            }
        }
        """

        # Reminder: class name may be nested ("Foo.Bar").
        for class_element in self.element_filter.matching_classes(self.model.classes):
            # If the class name has a trailing nested class identifier such as "Foo.Bar",
            # always export the class as "Bar" but under the "Foo" namespace.
            full_namespace = PathFormatter.build_full_class_namespace(class_element.name, class_element.namespace)
            target_namespace = self._get_or_create_namespace_bucket(namespace_bucket, full_namespace)
            class_def = self._build_class_def(class_element, self.build_comments, target_namespace)
            target_namespace.add_child(class_def)

    def _build_dynamic_class_name(self, class_name: str) -> str:
        """ Prefix class with app name.
        TODO: future, support multiple of same type (one type per object).
        """
        return f"{self.app_name}{class_name}"

    def _add_dynamic_class(self, object_element: ObjectElement, namespace_bucket: NamespaceBucket) -> None:
        """ Generate and add a class def for an object referenced class. """

        if not object_element.dynamic_type:
            return

        # Prefix is added to name which can be nested: example Foo.Bar -> AppFoo.Bar.
        class_name = self._build_dynamic_class_name(object_element.class_name)

        # Namespace name the new type is added to, independent of the namespace resulting from nesting.
        class_namespace = namespace_bucket.get_path()

        # Nested classes such as 'Foo.Bar' are exported as 'Bar' in a 'Foo' so use 'Bar' as name.
        class_basename = PathFormatter.get_class_basename(class_name)
        class_def = ClassDef(name=class_basename, namespace=class_namespace, parent_name=object_element.class_name,
                             parent_namespace=object_element.class_namespace, build_comment=self.build_comments)

        # Override or extend parent class properties for the new type. Two cases:
        # 1) the property does not exist in the base class (check name + type) -> add it
        # 2) the property exists, but it also uses a dynamic type -> override it by type
        class_element: ClassElement = self.model.classes.get(object_element.class_name, None)
        if class_element is None:
            logging.error(f"could not find class {object_element.class_name} for object {object_element.name}")
            return

        # Consistency check: dynamic type should be 'mutable' attributed class.
        if not class_element.mutable:
            logging.error(f"object {object_element.name} dynamic type {object_element.class_name} is not mutable")
            return

        for child_obj in self.element_filter.matching_objects(object_element.children):

            # Property for case #1
            obj_member_name = child_obj.name
            obj_member_class_name = child_obj.class_name
            obj_member_class_namespace = child_obj.class_namespace

            # Property for case #2: peak at the dynamic type of this object
            if child_obj.dynamic_type:
                obj_member_class_name = self._build_dynamic_class_name(obj_member_class_name)
                obj_member_class_namespace = class_namespace

            # TODO: do not repeat same namespace in export? See/could use build_type_def () for ArgTypeDef.
            if not class_element.has_member_by_class(member_name=obj_member_name, member_class=obj_member_class_name):
                object_member_full_class_name = PathFormatter.build_full_name(class_name=obj_member_class_name,
                                                                              class_namespace=obj_member_class_namespace)
                type_def = ArgTypeDef(type_value="object", type_names_value=object_member_full_class_name)
                member_def = Member(name=obj_member_name, data_type=type_def, readonly=child_obj.readonly)
                class_def.add_property(prop=member_def)

        # Add to namespace bucket, creates namespace for nested class.
        full_namespace = PathFormatter.build_full_class_namespace(class_name, class_namespace)
        target_namespace = self._get_or_create_namespace_bucket(namespace_bucket, full_namespace)
        target_namespace.add_child(class_def)
        logging.info(f"added dynamic class '{class_name}'")

        # Save dynamic type in object but keep original class.
        object_element.dynamic_class_name = class_name
        object_element.dynamic_class_namespace = class_namespace

    def _process_object(self, object_element: ObjectElement, namespace_bucket: NamespaceBucket) -> None:
        self._add_dynamic_class(object_element, namespace_bucket)
        for object_child in self.element_filter.matching_objects(object_element.children):
            self._process_object(object_child, namespace_bucket)

    def _build_dynamic_classes(self, namespace_bucket: NamespaceBucket) -> None:
        """ Recurse objects, detect dynamic types and create new types on-the-fly. """

        for object_element in self.element_filter.matching_objects(self.model.objects):
            self._process_object(object_element, namespace_bucket)

    def _build_variables(self, namespace_bucket: NamespaceBucket) -> None:
        """ Export model objects to variables, ignores object children. The inner object
         structure is inferred from the object class type.
        """

        for obj in self.element_filter.matching_objects(self.model.objects):
            # Use generated type for dynamic type objects.
            object_class_name = obj.class_name if not obj.dynamic_type else obj.dynamic_class_name
            object_class_namespace = obj.class_namespace if not obj.dynamic_type else obj.dynamic_class_namespace

            # Denote namespace if the object class is in a different namespace than the current bucket.
            namespace = object_class_namespace if object_class_namespace != namespace_bucket.get_path() else ""
            variable_def = VariableDef(name=obj.name, class_name=object_class_name, class_namespace=namespace)

            namespace_bucket.add_child(variable_def)

    def run(self) -> Printable:
        """ Build TypeScript declaration from classmodel. Use configured namespace as top level
        namespace if available. By design, always export variables to root level.

        :return: typescript declaration model
        """

        root = ModelRoot()
        namespace = self._get_or_create_namespace_bucket(root=root, ns_path=self.root_namespace)

        self._build_classes(namespace_bucket=namespace)
        self._build_dynamic_classes(namespace_bucket=namespace)
        self._build_variables(namespace_bucket=root)

        return root
