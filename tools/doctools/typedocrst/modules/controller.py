"""TypeDoc view-model controller.

Walks the parsed Model/JSON object graph and produces plain, RST-free view
model objects (dataclasses) for the templates to render. All tree traversal,
sorting, and ID-based lookups that require the Model object graph live here;
no RST markup is produced in this module.
"""

from dataclasses import dataclass, field
from typing import List, Optional

from modules.model import Model
from modules.typedocjson import *
from modules.util import TypeDocUtil, UID, DetailsFileReference
from modules.printer import paragraph, italic, ref, render_comment_segments, render_property, render_method, \
    render_enum_member, render_method_body

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


@dataclass
class LinkVM:
    text: str
    target: str


@dataclass
class InheritedFromVM:
    text: str
    target: str = ""  # empty: plain text, no ref


@dataclass
class CommentSegmentVM:
    kind: str  # 'text' or 'code'
    text: str
    caption: str = ""


@dataclass
class ParamVM:
    name: str
    type: str
    comment: str = ""  # pre-rendered (possibly empty) comment text


@dataclass
class SignatureVM:
    name: str
    return_type: str
    params: List[ParamVM]
    comment: List[CommentSegmentVM]


@dataclass
class PropertyVM:
    uid: str
    name: str
    is_reflection: bool
    signatures: List[SignatureVM]  # reflection case only
    type: str  # non-reflection case only
    readonly: bool
    inherited_from: Optional[InheritedFromVM]
    comment: List[CommentSegmentVM]


@dataclass
class MethodVM:
    uid: str
    signatures: List[SignatureVM]


@dataclass
class EnumMemberVM:
    uid: str
    name: str
    comment: List[CommentSegmentVM]


@dataclass
class GroupVM:
    title: str
    links: List[LinkVM]  # for the summary section
    children: List[str]  # pre-rendered RST for the details section


@dataclass
class FileVM:
    shape: str  # 'group_summary' | 'group_details' | 'function' | 'variable'
    anchor: str
    name: str
    kind_string: str
    breadcrumb: List[LinkVM]
    comment_body: str  # pre-rendered RST
    groups: List[GroupVM] = field(default_factory=list)
    function_body: str = ""  # 'function' shape, pre-rendered RST
    variable_type_link: Optional[LinkVM] = None  # 'variable' shape


@dataclass
class IndexVM:
    root_anchor: str
    file: FileVM
    toc_files: List[str]


@dataclass
class ShortcutRowVM:
    heading_comment: Optional[str]  # group name for a leading '.. Name' comment line, else None
    alias: str
    value: str  # already bracketed link text, e.g. "[:ref:`x <y>`]"


def resolve_type_name(type_object) -> str:
    """ Resolve a TypeDoc 'type' JSON object to its display name, recursively. """

    if type_object is None:
        return ""

    type_string = type_object.get(TDAttribute.TYPE)
    if type_string == TDType.ARRAY:
        array_type_name = type_object.get(TDAttribute.ELEMENT_TYPE).get(TDAttribute.NAME)
        return "%s []" % array_type_name
    elif type_string == TDType.STRING_LITERAL:
        return "string const"
    elif type_string == TDType.UNION:
        types_list = type_object.get(TDAttribute.TYPES, [])
        return " | ".join(resolve_type_name(t) for t in types_list)
    elif type_string == TDType.REFERENCE or type_string == TDType.INTRINSIC:
        type_name = type_object.get(TDAttribute.NAME)
        return type_name if type_name else "MISSING TYPE NAME"
    elif type_string == TDType.TYPE_PARAMETER:
        type_name = type_object.get(TDAttribute.NAME)
        constraint_object = type_object.get(TDAttribute.CONSTRAINT)
        if not constraint_object:
            return "<%s>" % type_name
        return "<%s: %s>" % (type_name, constraint_object.get(TDAttribute.NAME))
    else:
        return "UNSUPPORTED TYPE %s" % type_string


def has_readonly_flag(obj: JSONObject) -> bool:
    flags = obj.get(TDAttribute.FLAGS, None)
    return bool(flags and flags.get(TDAttribute.IS_READONLY, ""))


def build_comment_segments(comment_obj) -> List[CommentSegmentVM]:
    """ Build the ordered list of text/code segments from a TypeDoc 'comment' object. """

    segments = []
    if not comment_obj:
        return segments

    for summary_child in comment_obj.get(TDAttribute.SUMMARY):
        summary_kind = summary_child.get(TDAttribute.REFLECTION_KIND)
        text = summary_child.get(TDAttribute.TEXT)

        if summary_kind == TDSummaryKind.TEXT:
            if text:
                segments.append(CommentSegmentVM(kind="text", text=TypeDocUtil.cleanup_comment_text(text)))
        elif summary_kind == TDSummaryKind.CODE:
            code_example = TypeDocUtil.cleanup_comment_code(text)
            if code_example:
                segments.append(CommentSegmentVM(kind="code", text=code_example, caption="Code Example"))

    return segments


def build_param_comment(comment_obj) -> str:
    """ Build a single parameter's comment text (italic, paragraphed), or "" if none. """

    if not comment_obj:
        return ""

    text = comment_obj.get(TDAttribute.TEXT)
    if not text:
        return ""

    text = text.strip()
    if not text:
        return ""

    return paragraph(italic(text))


def build_params(model: Model, signature_obj: JSONObject, with_comments: bool) -> List[ParamVM]:
    param_objects = signature_obj.get(TDAttribute.PARAMETERS)
    if not param_objects:
        return []

    params = []
    for p in param_objects:
        type_str = resolve_type_name(p.get(TDAttribute.TYPE, None))
        comment_str = build_param_comment(p.get(TDAttribute.COMMENT)) if with_comments else ""
        params.append(ParamVM(name=p.get(TDAttribute.NAME), type=type_str, comment=comment_str))

    return params


def build_signature(model: Model, signature_obj: JSONObject) -> SignatureVM:
    return SignatureVM(
        name=signature_obj.get(TDAttribute.NAME),
        return_type=resolve_type_name(signature_obj.get(TDAttribute.TYPE)),
        params=build_params(model, signature_obj, with_comments=True),
        comment=build_comment_segments(signature_obj.get(TDAttribute.COMMENT))
    )


def build_link(model: Model, obj: JSONObject) -> LinkVM:
    """ Link to 'obj', targeting its UID anchor (same document). """
    return LinkVM(text=TypeDocUtil.get_object_name_safe(obj), target=UID.create(obj, model.get_namespace()))


def build_details_file_link(model: Model, obj: JSONObject) -> LinkVM:
    """ Link to 'obj', targeting its details file anchor (separate document). """
    return LinkVM(text=TypeDocUtil.get_object_name_safe(obj),
                  target=DetailsFileReference.create(obj, model.get_namespace()))


def build_auto_link(model: Model, obj: JSONObject) -> LinkVM:
    """ Link to 'obj', choosing file-anchor or same-document anchor based on its kind. """
    object_kind_id = obj.get(TDAttribute.REFLECTION_KIND)
    if Model.is_exported_to_separate_file(object_kind_id):
        return build_details_file_link(model, obj)
    return build_link(model, obj)


def build_breadcrumb(model: Model, obj: JSONObject) -> List[LinkVM]:
    """ Build breadcrumb link chain, empty list if 'obj' has no parents (e.g. project root). """

    object_id = obj.get(TDAttribute.ID)
    parent_ids = model.get_object_parent_ids(object_id)
    if not parent_ids:
        return []

    links = [LinkVM(text="Global", target=model.get_root_anchor())]
    for parent_id in parent_ids:
        parent_object = model.get_object(parent_id)
        if parent_object is None:
            continue
        links.append(build_details_file_link(model, parent_object))

    return links


def build_inherited_from(model: Model, inherited_obj: JSONObject) -> InheritedFromVM:
    inheritance_type = inherited_obj.get(TDAttribute.TYPE)
    name = inherited_obj.get(TDAttribute.NAME)

    if inheritance_type == TDType.REFERENCE:
        name_parts = name.split(".")
        parent_id = model.find_property_id(name_parts[0], name_parts[1])
        if parent_id != Model.INVALID_OBJECT_ID:
            target = UID.create({TDAttribute.ID: parent_id}, model.get_namespace())
            return InheritedFromVM(text=name, target=target)

    return InheritedFromVM(text=name)


def build_property(model: Model, obj: JSONObject) -> str:
    """ Build a property detail child, pre-rendered to RST. """

    uid = UID.create(obj, model.get_namespace())
    name = obj.get(TDAttribute.NAME)
    type_object = obj.get(TDAttribute.TYPE)
    is_reflection = type_object.get(TDAttribute.TYPE) == TDType.REFLECTION

    inherited_from_obj = obj.get(TDAttribute.INHERITED_FROM)
    inherited_from = build_inherited_from(model, inherited_from_obj) if inherited_from_obj else None

    comment = build_comment_segments(obj.get(TDAttribute.COMMENT))

    if is_reflection:
        declaration_object = type_object.get(TDAttribute.DECLARATION)
        signature_objects = declaration_object.get(TDAttribute.SIGNATURES)
        if not signature_objects:
            signature_objects = [declaration_object.get(TDAttribute.INDEX_SIGNATURE)]

        signatures = []
        for s in signature_objects:
            signatures.append(SignatureVM(
                name=name,
                return_type=s.get(TDAttribute.TYPE).get(TDAttribute.NAME),
                params=build_params(model, s, with_comments=False),
                comment=[]
            ))

        p = PropertyVM(uid=uid, name=name, is_reflection=True, signatures=signatures, type="", readonly=False,
                       inherited_from=inherited_from, comment=comment)
    else:
        type_str = resolve_type_name(type_object)
        readonly = has_readonly_flag(obj)
        p = PropertyVM(uid=uid, name=name, is_reflection=False, signatures=[], type=type_str, readonly=readonly,
                       inherited_from=inherited_from, comment=comment)

    return render_property(p)


def build_method(model: Model, obj: JSONObject) -> str:
    """ Build a method detail child, pre-rendered to RST. """

    uid = UID.create(obj, model.get_namespace())
    signatures = [build_signature(model, s) for s in obj.get(TDAttribute.SIGNATURES)]
    return render_method(MethodVM(uid=uid, signatures=signatures))


def build_enum_member(model: Model, obj: JSONObject) -> str:
    """ Build an enum member detail child, pre-rendered to RST. """

    uid = UID.create(obj, model.get_namespace())
    comment = build_comment_segments(obj.get(TDAttribute.COMMENT))
    e = EnumMemberVM(uid=uid, name=obj.get(TDAttribute.NAME), comment=comment)
    return render_enum_member(e)


def build_detail_child(model: Model, obj: JSONObject) -> str:
    kind = obj.get(TDAttribute.REFLECTION_KIND)
    if kind == TDReflectionKind.ENUM_MEMBER:
        return build_enum_member(model, obj)
    elif kind in (TDReflectionKind.METHOD, TDReflectionKind.CONSTRUCTOR):
        return build_method(model, obj)
    elif kind == TDReflectionKind.PROPERTY:
        return build_property(model, obj)

    raise NotImplementedError("Unexpected detail child kind %s" % kind)


def build_group(model: Model, group_object: JSONObject, with_children: bool) -> GroupVM:
    children_objects = sorted(model.get_object_children(group_object), key=lambda k: k[TDAttribute.NAME])

    links = [build_auto_link(model, c) for c in children_objects]
    children = [build_detail_child(model, c) for c in children_objects] if with_children else []

    return GroupVM(title=group_object.get(TDAttribute.TITLE), links=links, children=children)


def build_file(model: Model, obj: JSONObject) -> FileVM:
    kind = obj.get(TDAttribute.REFLECTION_KIND)

    if kind != TDReflectionKind.PROJECT:
        assert Model.is_exported_to_separate_file(kind)

    if kind in (TDReflectionKind.PROJECT, TDReflectionKind.NAMESPACE, TDReflectionKind.TYPE_ALIAS):
        shape = "group_summary"
    elif kind in (TDReflectionKind.CLASS, TDReflectionKind.INTERFACE, TDReflectionKind.ENUM):
        shape = "group_details"
    elif kind == TDReflectionKind.FUNCTION:
        shape = "function"
    elif kind == TDReflectionKind.VARIABLE:
        shape = "variable"
    else:
        raise NotImplementedError("Unexpected file object kind %s" % kind)

    anchor = DetailsFileReference.create(obj, model.get_namespace())
    name = obj.get(TDAttribute.NAME)
    kind_string = obj.get(TDAttribute.KIND_STRING)
    breadcrumb = build_breadcrumb(model, obj)
    comment_body = paragraph(render_comment_segments(build_comment_segments(obj.get(TDAttribute.COMMENT))))

    groups = []
    object_groups = obj.get(TDAttribute.GROUPS)
    if object_groups:
        groups = [build_group(model, g, with_children=(shape == "group_details")) for g in object_groups]

    function_body = ""
    if shape == "function":
        signatures = [build_signature(model, s) for s in obj.get(TDAttribute.SIGNATURES)]
        function_body = render_method_body(signatures)

    variable_type_link = None
    if shape == "variable":
        variable_type_link = build_variable_type_link(model, obj)

    return FileVM(shape=shape, anchor=anchor, name=name, kind_string=kind_string, breadcrumb=breadcrumb,
                  comment_body=comment_body, groups=groups, function_body=function_body,
                  variable_type_link=variable_type_link)


def build_variable_type_link(model: Model, obj: JSONObject) -> Optional[LinkVM]:
    """ Link to a variable's type, reference types only (matches VariableObject.build_type_link). """

    type_object = obj.get(TDAttribute.TYPE, None)
    if not type_object:
        return None

    if type_object.get(TDAttribute.TYPE, "") != TDType.REFERENCE:
        return None

    target_object_id = type_object.get(TDAttribute.ID, "")
    if not target_object_id:
        return None

    target_obj = model.get_object(target_object_id)
    class_obj = {
        TDAttribute.NAME: type_object.get(TDAttribute.NAME, ""),
        TDAttribute.ID: type_object.get(TDAttribute.ID, ""),
        TDAttribute.REFLECTION_KIND: target_obj.get(TDAttribute.REFLECTION_KIND, "")
    }

    return build_auto_link(model, class_obj)


def build_index(model: Model, obj: JSONObject, toc_files: List[str]) -> IndexVM:
    return IndexVM(root_anchor=model.get_root_anchor(), file=build_file(model, obj), toc_files=toc_files)


def _build_shortcut_alias_value(model: Model, obj: JSONObject, kind_id, kind_string: str, full_name: str):
    if Model.is_exported_to_separate_file(kind_id):
        target = DetailsFileReference.create(obj, model.get_namespace())
    else:
        target = UID.create(obj, model.get_namespace())

    text = TypeDocUtil.get_object_name_safe(obj)
    alias = ("%s.%s" % (kind_string, full_name)).lower().replace(" ", "")
    value = "[%s]" % ref(text, target)
    return alias, value


def _sorted_children(model: Model, group_object: JSONObject) -> List[JSONObject]:
    return sorted(model.get_object_children(group_object), key=lambda k: k[TDAttribute.NAME])


def build_shortcut_rows(model: Model) -> List[ShortcutRowVM]:
    """ Flatten the recursive shortcut tree (see ShortcutListPrinter) into an ordered row list. """

    rows: List[ShortcutRowVM] = []

    def recurse(obj: JSONObject, kind_id, kind_string: str, full_name: str) -> None:
        group_objects = obj.get(TDAttribute.GROUPS)
        alias, value = _build_shortcut_alias_value(model, obj, kind_id, kind_string, full_name)

        if group_objects is None:
            rows.append(ShortcutRowVM(heading_comment=None, alias=alias, value=value))
            return

        rows.append(ShortcutRowVM(heading_comment=obj.get(TDAttribute.NAME), alias=alias, value=value))

        for group_object in group_objects:
            for child_object in _sorted_children(model, group_object):
                child_full_name = "%s.%s" % (full_name, child_object.get(TDAttribute.NAME))
                recurse(child_object, child_object.get(TDAttribute.REFLECTION_KIND),
                        child_object.get(TDAttribute.KIND_STRING), child_full_name)

    for group_object in sorted(model.root_object.get(TDAttribute.GROUPS), key=lambda k: k["title"]):
        for child_object in _sorted_children(model, group_object):
            recurse(child_object, child_object.get(TDAttribute.REFLECTION_KIND),
                    child_object.get(TDAttribute.KIND_STRING), child_object.get(TDAttribute.NAME))

    return rows
