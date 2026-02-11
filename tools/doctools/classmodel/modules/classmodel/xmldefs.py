"""Classmodel XML definitions."""

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class XML:
    """ Namespace for ClassModel XML related definitions. """

    # Element names
    ELEMENT_MODEL_CLASS_REPOSITORY = "Model.ClassRepository"
    ELEMENT_LIST = 'List'
    ELEMENT_MODEL_CLASS = 'Model.Class'
    ELEMENT_MODEL_ENUM = 'Model.Enumeration'
    ELEMENT_MODEL_OBJECT = 'Model.Object'
    ELEMENT_MODEL_DOCUMENTATION = 'Model.Documentation'
    ELEMENT_MODEL_MEMBER = 'Model.Member'
    ELEMENT_MODEL_METHOD = 'Model.Method'
    ELEMENT_MODEL_RETURN_VALUE = 'Model.ReturnValue'
    ELEMENT_MODEL_ENUMERATOR = 'Model.Enumerator'
    ELEMENT_MODEL_PROPERTY = 'Model.Property'
    ELEMENT_STRING = "String"
    ELEMENT_MODEL_METHOD_ARG = "Model.MethodArg"
    ELEMENT_ATTRIBUTE = "Attribute"
    ELEMENT_ATTRIBUTES = "Attributes"

    # Attribute names
    ATTR_ID = "id"
    ATTR_NAME = "name"
    ATTR_NAMESPACE = "namespace"
    ATTR_BRIEF = "brief"
    ATTR_DETAILED = "detailed"
    ATTR_CODE = "code"
    ATTR_LANGUAGE = "language"
    ATTR_TEXT = "text"
    ATTR_TYPE = "type"
    ATTR_TYPE_NAME = "typeName"
    ATTR_VALUE = "value"
    ATTR_DEFAULT_VALUE = "defaultValue"
    ATTR_PARENT = "parent"
    ATTR_PARENT_NAMESPACE = "parentNamespace"
    ATTR_CLASS = "class"
    ATTR_CLASS_NAMESPACE = "classNamespace"
    ATTR_READ_ONLY = "readOnly"
    ATTR_DYNAMIC_TYPE = "dynamicType"
    ATTR_SCRIPTABLE = "scriptable"
    ATTR_MUTABLE = "mutable"
    ATTR_ABSTRACT = "abstract"

    # Attribute values
    VALUE_MEMBERS = "members"
    VALUE_METHODS = "methods"
    VALUE_BRIEF = "brief"
    VALUE_DETAILED = "detailed"
    VALUE_CLASSES = "classes"
    VALUE_ENUMS = "enums"
    VALUE_ARGS = "args"
    VALUE_ENUMERATORS = "enumerators"
    VALUE_OBJECTS = "objects"
    VALUE_CHILDREN = "children"
    VALUE_PROPERTIES = "properties"
    VALUE_ATTRIBUTES = "attributes"
    VALUE_SCHEMA_GROUPS = "Class:SchemaGroups"
    VALUE_CHILD_GROUP = "Class:ChildGroup"
    VALUE_DOC_GROUP = "Class:DocGroup"
    VALUE_LINKS = "links"