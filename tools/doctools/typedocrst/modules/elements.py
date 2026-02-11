"""TypeScript printable elements classes module."""

from modules.model import Model
from modules.rstprinter import RstPrinter
from modules.util import *
from modules.typedocjson import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class AutoLink:
    """ Build a link to either a file or anchor in same document depending on kind of object. """

    def __init__(self, model: Model, obj: JSONObject) -> None:
        self.obj = obj
        self.model = model

    def to_rst(self):
        object_kind_id = self.obj.get(TDAttribute.REFLECTION_KIND)
        if self.model.is_exported_to_separate_file(object_kind_id):
            return DetailsFileHyperLink(self.obj, self.model.get_namespace()).to_rst()
        else:
            return HyperLink(self.obj, self.model.get_namespace()).to_rst()


class HyperLink:
    """
    Build link from any JSON object. Uses the object
    kind (id) and id for creating a reference.
    """

    def __init__(self, obj: JSONObject, data_namespace: str) -> None:
        self.obj = obj
        self.data_namespace = data_namespace

    def to_rst(self) -> str:
        """ Generate RST hyperlink as [text] with UID as ref target """
        link_target = UID.create(self.obj, self.data_namespace)
        link_text = TypeDocUtil.get_object_name_safe(self.obj)
        return "[%s] " % RstPrinter.hyperlink(link_text, link_target)


class DetailsFileHyperLink:
    """ HyperLink to details file. """

    def __init__(self, obj: JSONObject, data_namespace: str) -> None:
        self.obj = obj
        self.data_namespace = data_namespace

    def to_rst(self) -> str:
        link_target = DetailsFileReference.create(self.obj, self.data_namespace)
        link_text = TypeDocUtil.get_object_name_safe(self.obj)
        return "[%s] " % RstPrinter.hyperlink(link_text, link_target)


class PropertyHyperLink(HyperLink):
    """
    Build link to a property, from id.
    Sets up a dummy object to use with HyperLink.
    """

    def __init__(self, name: str, id_value: int, data_namespace: str) -> None:
        self.property_obj = {
            TDAttribute.ID: id_value,
            TDAttribute.NAME: name,
            TDAttribute.REFLECTION_KIND: TDReflectionKind.PROPERTY
        }
        super(PropertyHyperLink, self).__init__(self.property_obj, data_namespace)


class Breadcrumb:
    """
    Show object path with references. Expects 'top level' elements
    only, i.e. elements that are exported to separate files.

    Example: "Home > CCL > AliasParam"
    """

    def __init__(self, model: Model, obj: JSONObject) -> None:
        self.model = model
        self.object = obj

    def to_rst(self) -> str:
        """ Retrieve object parent IDs from model, build breadcrumb string. """

        object_id = self.object.get(TDAttribute.ID)
        parents = self.model.get_object_parent_ids(object_id)
        if not parents:
            return ""

        # Manually add anchor for global scope. It is not stored in the model object hierarchy.
        global_scope_anchor = self.model.get_root_anchor()
        result = RstPrinter.hyperlink("Global", global_scope_anchor) + " > "

        for parent_id in parents:
            parent_object = self.model.get_object(parent_id)
            if parent_object is None:
                continue

            # Elements should be accessible as separate files only.
            parent_kind_id = parent_object.get(TDAttribute.REFLECTION_KIND, -1)
            assert (self.model.is_exported_to_separate_file(parent_kind_id)), "Unexpected breadcrumb object kind"

            # Build ref manually to not use the [target] format
            parent_name = parent_object.get(TDAttribute.NAME)
            ref = DetailsFileReference.create(parent_object, self.model.get_namespace())
            result += "%s > " % RstPrinter.hyperlink(parent_name, ref)

        return result[:-2]


class BaseObject:
    """ RST printable base object. """

    model = None  # provide children data if only IDs are available
    json_object = None  # the actual JSON tree object containing the data
    data_namespace = ""  # provide a global prefix for all hyperlink targets

    def __init__(self, model: Model, obj: JSONObject) -> None:
        self.model = model
        self.json_object = obj
        self.data_namespace = model.get_namespace()

    def uid(self) -> str:
        return UID.create(self.json_object, self.data_namespace)

    def details_file_reference(self) -> str:
        return DetailsFileReference.create(self.json_object, self.data_namespace)

    def to_rst(self) -> str:
        """ Convert element to and return as RST string. """
        raise NotImplementedError


class DetailsBase(BaseObject):
    """ Anchored content element. """

    def to_rst(self) -> str:
        """ Utilize to jump to this element from an overview section. """
        return RstPrinter.anchor(self.uid())


class GroupSummary(BaseObject):

    def to_rst(self) -> str:
        title_str = self.json_object.get(TDAttribute.TITLE)

        children_objects = self.model.get_object_children(self.json_object)
        children_str = ""
        for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
            children_str += AutoLink(self.model, child_object).to_rst()

        return "%s: %s" % (RstPrinter.bold(title_str), children_str)


class ParameterObject(BaseObject):

    def to_rst(self) -> str:
        param_name_str = self.json_object.get(TDAttribute.NAME)

        param_type_str = ""
        type_object = self.json_object.get(TDAttribute.TYPE, None)
        if type_object is not None:
            param_type_str = TypeObject(self.model, type_object).to_rst()

        return "%s: %s" % (param_name_str, RstPrinter.bold(param_type_str))


class DocumentedParameterObject(BaseObject):
    """ Parameter with comment text, use for signature. """

    def to_rst(self) -> str:
        param_name_str = self.json_object.get(TDAttribute.NAME)

        param_type_str = ""
        type_object = self.json_object.get(TDAttribute.TYPE, None)
        if type_object is not None:
            param_type_str = TypeObject(self.model, type_object).to_rst()

        comment_str = ParamCommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()

        return "%s: %s %s" % (param_name_str, RstPrinter.bold(param_type_str), comment_str)


class ParameterList(BaseObject):
    """ List of all parameters of a signature. """

    def to_rst(self) -> str:

        param_objects = self.json_object.get(TDAttribute.PARAMETERS)
        if not param_objects:
            return ""

        result_str = ""
        for p in param_objects:
            result_str += "%s\n" % RstPrinter.list_element(DocumentedParameterObject(self.model, p).to_rst())

        return result_str


class ArgumentList(BaseObject):

    def to_rst(self) -> str:

        # parameters, are optional
        param_str = ""
        param_objects = self.json_object.get(TDAttribute.PARAMETERS)
        if param_objects:
            for param_object in param_objects:
                param_str += "%s, " % ParameterObject(self.model, param_object).to_rst()
            param_str = param_str[:-2]

        return param_str


class SignatureObject(BaseObject):
    """ Signature of method. """

    def to_rst(self) -> str:
        # function name and return type
        return_type_str = TypeObject(self.model, self.json_object.get(TDAttribute.TYPE)).to_rst()

        # parameters, are optional
        param_str = ArgumentList(self.model, self.json_object).to_rst()

        function_name = "%s (%s): %s" % (
            RstPrinter.bold(self.json_object.get(TDAttribute.NAME)), param_str, RstPrinter.bold(return_type_str))

        # function name and comment
        result_str = RstPrinter.paragraph(function_name)
        result_str += CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()

        # detailed list of params, avoid empty list
        param_list_str = RstPrinter.paragraph(ParameterList(self.model, self.json_object).to_rst())
        if param_list_str:
            result_str += RstPrinter.paragraph(RstPrinter.bold("Parameters")) + param_list_str

        return result_str


class TypeObject(BaseObject):
    """ Param or return value type. """

    @staticmethod
    def parse_type_object(type_object: JSONObject) -> str:
        type_string = type_object.get(TDAttribute.TYPE)
        if type_string == TDType.ARRAY:
            array_type_name = type_object.get(TDAttribute.ELEMENT_TYPE).get(TDAttribute.NAME)
            return "%s []" % array_type_name
        elif type_string == TDType.STRING_LITERAL:
            return "string const"
        elif type_string == TDType.UNION:
            return TypeObject.union_type_to_rst(type_object)
        elif type_string == TDType.REFERENCE:
            return TypeObject.named_type_to_rst(type_object)
        elif type_string == TDType.INTRINSIC:
            return TypeObject.named_type_to_rst(type_object)
        elif type_string == TDType.TYPE_PARAMETER:
            return TypeObject.type_parameter_to_rst(type_object)
        else:
            return "UNSUPPORTED TYPE %s" % type_string

    @staticmethod
    def union_type_to_rst(_type_object: JSONObject) -> str:
        """ Concatenate union types to a | b | ... """

        result_str = ""
        types_list = _type_object.get(TDAttribute.TYPES, [])
        for t in types_list:
            # result_str += "%s | "  % t.get(TypeDoc.ATTR_NAME, "MISSING TYPE")
            result_str += "%s | " % TypeObject.parse_type_object(t)
        return result_str[:-3]

    @staticmethod
    def named_type_to_rst(_type_object: JSONObject) -> str:
        type_name = _type_object.get(TDAttribute.NAME)
        return type_name if type_name else "MISSING TYPE NAME"

    @staticmethod
    def type_parameter_to_rst(_type_object: JSONObject) -> str:
        type_name = _type_object.get(TDAttribute.NAME)
        constraint_object = _type_object.get(TDAttribute.CONSTRAINT)
        if not constraint_object:
            return "<%s>" % type_name

        constraint_type_name = constraint_object.get(TDAttribute.NAME)
        return "<%s: %s>" % (type_name, constraint_type_name)

    def to_rst(self) -> str:
        return TypeObject.parse_type_object(self.json_object)


class PropertyInheritanceInfo(BaseObject):

    def to_rst(self) -> str:
        """
        Create info text or hyperlink to parent property.

        For references, attempt to find the parent property by ID
        and use it to add a link instead. In the JSON file, the reference
        is denoted with the derived property ID, not the parent ID.
        This could be a bug in TypeDoc.
        """

        inheritance_type = self.json_object.get(TDAttribute.TYPE)

        if inheritance_type == TDType.REFERENCE:
            ref_name = self.json_object.get(TDAttribute.NAME)
            ref_name_split = ref_name.split(".")
            parent_ref_id = self.model.find_property_id(ref_name_split[0], ref_name_split[1])
            if parent_ref_id != self.model.INVALID_OBJECT_ID:
                link = PropertyHyperLink(ref_name, parent_ref_id, self.data_namespace)
                return link.to_rst()

        return self.json_object.get(TDAttribute.NAME)


class PropertyDetails(DetailsBase):
    """ Class property. """

    def reflection_property(self) -> str:
        property_name = self.json_object.get(TDAttribute.NAME)
        type_object = self.json_object.get(TDAttribute.TYPE)
        declaration_object = type_object.get(TDAttribute.DECLARATION)

        # assume existence of 'signatures' list or 'indexSignature' for a reflection based property
        signature_objects = declaration_object.get(TDAttribute.SIGNATURES)

        # fallback to single indexSignature requires list conversion
        if not signature_objects:
            signature_objects = list()
            signature_objects.append(declaration_object.get(TDAttribute.INDEX_SIGNATURE))

        # fallback, should not happen
        if not signature_objects:
            return RstPrinter.paragraph(property_name)

        # parse signatures
        result_str = ""
        for s in signature_objects:
            return_type_name = s.get(TDAttribute.TYPE).get(TDAttribute.NAME)
            args_str = ArgumentList(self.model, s).to_rst()
            full_property_str = "%s (%s): %s" % (property_name, args_str, return_type_name)
            result_str += RstPrinter.paragraph(full_property_str)

        # inheritance is per property, not per signature
        inherited_from_obj = self.json_object.get(TDAttribute.INHERITED_FROM)
        if inherited_from_obj:
            result_str += RstPrinter.paragraph(
                "inherited from %s " % PropertyInheritanceInfo(self.model, inherited_from_obj).to_rst())

        return RstPrinter.paragraph(result_str) + RstPrinter.paragraph(
            CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst())

    def non_reflection_property(self) -> str:
        type_name_str = TypeObject(self.model, self.json_object.get(TDAttribute.TYPE)).to_rst()

        # name, type, read-only flag
        property_str = "%s: %s" % (RstPrinter.bold(self.json_object.get(TDAttribute.NAME)), type_name_str)
        if ElementUtil.has_readonly_flag(self.json_object):
            property_str += " " + RstPrinter.italic("[read-only]")

        result_str = RstPrinter.paragraph(property_str)
        inherited_from_obj = self.json_object.get(TDAttribute.INHERITED_FROM)
        if inherited_from_obj:
            result_str += RstPrinter.paragraph(
                "inherited from %s " % PropertyInheritanceInfo(self.model, inherited_from_obj).to_rst())

        comment_str = CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()
        result_str += RstPrinter.paragraph(comment_str)

        return result_str

    def to_rst(self) -> str:
        result_str = super(PropertyDetails, self).to_rst()

        type_object = self.json_object.get(TDAttribute.TYPE)
        if type_object.get(TDAttribute.TYPE) == TDType.REFLECTION:
            # special case: reflection type properties are methods, not members
            result_str += self.reflection_property()
        else:
            result_str += self.non_reflection_property()

        return result_str


class GroupDetails(BaseObject):

    def to_rst(self) -> str:
        title_str = self.json_object.get(TDAttribute.TITLE)
        children_objects = self.model.get_object_children(self.json_object)

        # print Properties as a list to have them displayed more compact
        # if title_str != TypeDoc.VALUE_GROUP_TITLE_PROPERTIES:
        children_str = ""
        count = 0
        for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
            children_str += ObjectBuilder.create_details_object(child_object, self.model).to_rst()
            # don't end with a line (reported as sphinx transition end warning)
            if count < len(children_objects) - 1:
                children_str += RstPrinter.hr()
            count += 1

        return RstPrinter.h2(title_str) + children_str


class BriefCommentObject(BaseObject):
    """ Comment with only the shortText attribute. """

    def to_rst(self) -> str:
        """
        Process comment object to RST string, including brief and code example.
        """

        # is optional, check
        if not self.json_object:
            return ""

        return RstPrinter.paragraph(RstPrinter.italic(self.json_object.get(TDAttribute.SHORT_TEXT)))


class ParamCommentObject(BaseObject):
    """ Comment with only the 'text' attribute. """

    def to_rst(self) -> str:
        """
        Process comment object to RST string, including brief and code example.
        """

        # is optional, check
        if not self.json_object:
            return ""

        text = self.json_object.get(TDAttribute.TEXT)
        if not text:
            return ""

        # some comments can consist of only a \n which
        # causes a display artifact, remove them
        text = text.strip()
        if not text:
            return ""

        return RstPrinter.paragraph(RstPrinter.italic(text))


class CommentObject(BaseObject):
    """ Comment with shortText and code example. """

    def to_rst(self) -> str:
        """
        Process comment object to RST string, including brief and code example.
        """

        result_str = ""

        # is optional, check
        if not self.json_object:
            return result_str

        # comment is composed of a list of 'summary' objects
        for summary_child in self.json_object.get(TDAttribute.SUMMARY):
            summary_kind = summary_child.get(TDAttribute.REFLECTION_KIND)
            text = summary_child.get(TDAttribute.TEXT)

            # text paragraph
            if summary_kind == TDSummaryKind.TEXT:
                if text:
                    result_str += RstPrinter.paragraph(TypeDocUtil.cleanup_comment_text(text))

            # code snippet paragraph
            elif summary_kind == TDSummaryKind.CODE:
                code_example = TypeDocUtil.cleanup_comment_code(text)
                if code_example:
                    result_str += RstPrinter.code_block("Code Example", code_example,
                                                        RstPrinter.CODE_BLOCK_LANG_JAVASCRIPT)

        return result_str


class FileBase(BaseObject):
    """ File header: anchor, title and breadcrumbs. """

    def to_rst(self) -> str:
        result_str = RstPrinter.anchor(self.details_file_reference())

        name = self.json_object.get(TDAttribute.NAME)
        kind = self.json_object.get(TDAttribute.KIND_STRING)
        result_str += RstPrinter.h1("%s (%s)" % (name, kind), False)

        breadcrumb_str = Breadcrumb(self.model, self.json_object).to_rst()
        if breadcrumb_str:
            result_str += RstPrinter.paragraph(breadcrumb_str)

        return result_str


class MethodObject(BaseObject):
    """ Method by signatures. """

    def to_rst(self) -> str:
        # do not print the method name, iterate over the signatures instead

        signatures_str = ""
        signature_objects = self.json_object.get(TDAttribute.SIGNATURES)
        for signature_object in signature_objects:
            signatures_str += SignatureObject(self.model, signature_object).to_rst()

        result_str = RstPrinter.paragraph(signatures_str)
        return RstPrinter.paragraph(result_str)


class MethodDetails(DetailsBase):
    """ Method details with UID anchor. """

    def to_rst(self) -> str:
        result_str = super(MethodDetails, self).to_rst()
        result_str += MethodObject(self.model, self.json_object).to_rst()

        return result_str


class MethodFile(FileBase):
    """ Method as separate file. """

    def to_rst(self) -> str:
        result_str = super(MethodFile, self).to_rst()
        result_str += MethodObject(self.model, self.json_object).to_rst()

        return result_str


class VariableFile(FileBase):

    def to_rst(self) -> str:
        """
        In addition to header info (from base class), print the variable type as reference.
        """
        result_str = super(VariableFile, self).to_rst()
        result_str += VariableObject(self.model, self.json_object).to_rst()

        return result_str


class GroupSummaryFile(FileBase):
    """ Extended file: print group summary. """

    def to_rst(self) -> str:
        """
        Print summary, description, group content in detail.
        """

        result_str = super(GroupSummaryFile, self).to_rst()
        object_groups = self.json_object.get(TDAttribute.GROUPS)
        if not object_groups:
            return result_str

        # initial element comment, not defined for a specific child
        comment_str = CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()
        result_str += RstPrinter.paragraph(comment_str)

        # group summary with links to individual elements
        groups_summary_str = RstPrinter.h2("Summary")
        for group_object in object_groups:
            groups_summary_str += GroupSummary(self.model, group_object).to_rst()
        result_str += RstPrinter.paragraph(groups_summary_str)

        return result_str


class GroupDetailsFile(GroupSummaryFile):
    """ Extended file: print group details. """

    def to_rst(self) -> str:
        """ Print summary, description, group content in detail. """

        result_str = super(GroupDetailsFile, self).to_rst()

        object_groups = self.json_object.get(TDAttribute.GROUPS)
        if not object_groups:
            return result_str

        # group contained data in detail
        group_details_str = ""
        for group_object in object_groups:
            group_details_str += GroupDetails(self.model, group_object).to_rst()

        # summary file output, only introduction
        return result_str + group_details_str


class VariableObject(BaseObject):

    def build_type_link(self) -> str:
        """ Create hyperlink style RST text, supports reference type only. """

        # Link to type if reference.
        type_object = self.json_object.get(TDAttribute.TYPE, None)
        if not type_object:
            return ""

        if not type_object.get(TDAttribute.TYPE, "") == "reference":
            return ""

        # ID may be missing for external types (example: "Map").
        target_object_id = type_object.get(TDAttribute.ID, "")
        if not target_object_id:
            return ""

        target_obj = self.model.get_object(target_object_id)

        # Name is not the referenced object name, id matches.
        # Kind is required to determine the correct link type.
        class_obj = {
            TDAttribute.NAME: type_object.get(TDAttribute.NAME, ""),
            TDAttribute.ID: type_object.get(TDAttribute.ID, ""),
            TDAttribute.REFLECTION_KIND: target_obj.get(TDAttribute.REFLECTION_KIND, "")
        }

        return AutoLink(self.model, class_obj).to_rst()

    def to_rst(self) -> str:
        """ Add variable type as reference. """

        result_str = ""
        type_link = self.build_type_link()
        comment_str = CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()
        result_str += RstPrinter.paragraph(comment_str)

        if not type_link:
            return RstPrinter.paragraph(result_str)

        return RstPrinter.paragraph("%sType: %s" % (result_str, type_link))


class VariableDetails(DetailsBase):
    """ Variable details with UID anchor. """

    def to_rst(self) -> str:
        result_str = super(VariableDetails, self).to_rst()
        result_str += VariableObject(self.model, self.json_object).to_rst()

        return result_str


class EnumMemberDetails(DetailsBase):

    def to_rst(self) -> str:
        # omit default value to have less bloat in the output
        result_str = super(EnumMemberDetails, self).to_rst()
        enumerator_str = "%s" % self.json_object.get(TDAttribute.NAME)
        comment_str = CommentObject(self.model, self.json_object.get(TDAttribute.COMMENT)).to_rst()
        result_str += "%s %s" % (RstPrinter.bold(enumerator_str), comment_str)

        return result_str


class ShortcutBase:
    """
    Create a substituted reference that is stable and easier to use in handwritten
    texts, i.e. |ccl.element| over :ref:`Element <sdk-Class-Element>`.

    Use kind as additional prefix to make name unique. In some cases such as nested
    classes, a class is additionally exported as a namespace for its contained classes.
    """

    def __init__(self, obj: JSONObject, data_namespace: str, kind_string: str, full_name: str) -> None:
        self.obj = obj
        self.data_namespace = data_namespace
        self.full_name = full_name
        self.kind_string = kind_string

    def get_ref_target(self) -> str:
        raise NotImplementedError

    def to_rst(self) -> str:
        link_target = self.get_ref_target()
        link_text = TypeDocUtil.get_object_name_safe(self.obj)
        reference = RstPrinter.hyperlink(link_text, link_target)

        # Use lower case only. Some kind strings such as 'enumeration member'
        # also contain whitespaces, remove those too for consistent formatting.
        name_with_kind = "%s.%s" % (self.kind_string, self.full_name)
        formatted_name = name_with_kind.lower().replace(" ", "")

        return RstPrinter.replace(formatted_name, "[%s]" % reference)


class Shortcut(ShortcutBase):

    def get_ref_target(self) -> str:
        """ Stable ref based, i.e. uses the file anchor. """
        return DetailsFileReference.create(self.obj, self.data_namespace)


class PropertyShortcut(ShortcutBase):

    def get_ref_target(self) -> str:
        """ ID ref based, for properties. """
        return UID.create(self.obj, self.data_namespace)


class ElementUtil:

    @staticmethod
    def has_readonly_flag(obj: JSONObject) -> bool:
        # check if read-only flag is set
        flags = obj.get(TDAttribute.FLAGS, None)
        return flags and flags.get(TDAttribute.IS_READONLY, "")


class ObjectBuilder:

    @staticmethod
    def create_details_object(obj, model) -> DetailsBase:
        """ Object representation inside a file. """

        kind = obj.get(TDAttribute.REFLECTION_KIND)
        assert not model.is_exported_to_separate_file(kind)

        if kind == TDReflectionKind.ENUM_MEMBER:
            return EnumMemberDetails(model, obj)
        elif kind == TDReflectionKind.METHOD:
            return MethodDetails(model, obj)
        elif kind == TDReflectionKind.PROPERTY:
            return PropertyDetails(model, obj)
        elif kind == TDReflectionKind.CONSTRUCTOR:
            return MethodDetails(model, obj)

        # unexpected kind
        raise NotImplementedError

    @staticmethod
    def create_file_object(obj, model):
        """ Object representation as file. """

        kind = obj.get(TDAttribute.REFLECTION_KIND)

        # project is processed slightly different (index file over recursive export)
        if kind != TDReflectionKind.PROJECT:
            assert model.is_exported_to_separate_file(kind)

        if kind == TDReflectionKind.PROJECT:
            return GroupSummaryFile(model, obj)
        elif kind == TDReflectionKind.NAMESPACE:
            return GroupSummaryFile(model, obj)
        elif kind == TDReflectionKind.FUNCTION:
            return MethodFile(model, obj)
        elif kind == TDReflectionKind.VARIABLE:
            return VariableFile(model, obj)
        elif kind == TDReflectionKind.TYPE_ALIAS:
            return GroupSummaryFile(model, obj)
        elif kind == TDReflectionKind.ENUM:
            return GroupDetailsFile(model, obj)
        elif kind == TDReflectionKind.CLASS:
            return GroupDetailsFile(model, obj)
        elif kind == TDReflectionKind.INTERFACE:
            return GroupDetailsFile(model, obj)

        # unexpected kind
        raise NotImplementedError
