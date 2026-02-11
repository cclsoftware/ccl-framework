"""TypeDoc related definitions."""

from enum import IntEnum
from typing import TypeVar

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

JSONObject = TypeVar('JSONObject', bound=dict)


class TDReflectionKind(IntEnum):
    """TypeDoc JSOn reflection kind enumeration, see typedoc sources 'ReflectionType'."""

    PROJECT = 0x1,
    MODULE = 0x2,
    NAMESPACE = 0x4,
    ENUM = 0x8,
    ENUM_MEMBER = 0x10,
    VARIABLE = 0x20,
    FUNCTION = 0x40,
    CLASS = 0x80,
    INTERFACE = 0x100,
    CONSTRUCTOR = 0x200,
    PROPERTY = 0x400,
    METHOD = 0x800,
    CALL_SIGNATURE = 0x1000,
    INDEX_SIGNATURE = 0x2000,
    CONSTRUCTOR_SIGNATURE = 0x4000,
    PARAMETER = 0x8000,
    TYPE_LITERAL = 0x10000,
    TYPE_PARAMETER = 0x20000,
    ACCESSOR = 0x40000,
    GET_SIGNATURE = 0x80000,
    SET_SIGNATURE = 0x100000,
    OBJECT_LITERAL = 0x200000,
    TYPE_ALIAS = 0x400000,
    REFERENCE = 0x800000


class TDType:
    """TypeDoc JSON 'type' attribute values."""

    ARRAY = "array"
    STRING_LITERAL = "stringLiteral"
    REFLECTION = "reflection"
    INTRINSIC = "intrinsic"
    REFERENCE = "reference"
    UNION = "union"
    TYPE_PARAMETER = "typeParameter"


class TDSummaryKind:
    """TypeDoc JSON summary (comment) kind. """

    TEXT = "text"
    CODE = "code"


class TDAttribute:
    """TypeDoc JSON attributes and values."""

    ID = "id"
    NAME = "name"
    REFLECTION_KIND = "kind"
    FLAGS = "flags"
    CHILDREN = "children"
    KIND_STRING = "kindString"
    COMMENT = "comment"
    SUMMARY = "summary"
    SHORT_TEXT = "shortText"
    TEXT = "text"
    SOURCES = "sources"
    DEFAULT_VALUE = "defaultValue"
    GROUPS = "groups"
    TITLE = "title"
    SIGNATURES = "signatures"
    TYPE = "type"
    TYPES = "types"
    PARAMETERS = "parameters"
    ELEMENT_TYPE = "elementType"
    VALUE = "value"
    DECLARATION = "declaration"
    INHERITED_FROM = "inheritedFrom"
    INDEX_SIGNATURE = "indexSignature"
    IS_READONLY = "isReadonly"
    CONSTRAINT = "constraint"
