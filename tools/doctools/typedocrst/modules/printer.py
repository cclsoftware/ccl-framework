"""TypeDoc model printer.

Owns the Jinja2 environment, the small set of atomic RST-formatting filters
used by the templates, and the composite formatters that render a detail
child (property/method/enum member) or a comment list to its final RST
text. The kind-specific rendering choice (which composite formatter to
call) is made once in modules/controller.py; templates never branch on an
object's kind, they just place already-rendered text.
"""

import pathlib
from typing import List

from jinja2 import Environment, FileSystemLoader

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"

TEMPLATES_PATH = pathlib.Path(__file__).parent.parent / "templates"

CODE_BLOCK_LANG_JAVASCRIPT = "javascript"


def bold(text: str) -> str:
    """ Return text as bold. """
    return "**%s**" % text.strip()


def italic(text: str) -> str:
    """ Return text as italic. """
    return "*%s*" % text.strip()


def ref(text: str, target: str) -> str:
    """ Return text, target as :ref: link. """
    return ":ref:`%s <%s>`" % (text, target)


def autolink(link) -> str:
    """ Return a link as '[text] ', i.e. bracketed with a trailing space. """
    return "[%s] " % ref(link.text, link.target)


def link_ref(link) -> str:
    """ Return a LinkVM as a plain (unbracketed) :ref: link. """
    return ref(link.text, link.target)


def inherited_text(inherited) -> str:
    """ Return an InheritedFromVM as a link (if it has a target) or plain text. """
    if inherited.target:
        return autolink(inherited)
    return inherited.text


def paragraph(text: str, end_with_period: bool = False) -> str:
    """ Return text followed by a blank line, or "" if text is empty. """
    if not text:
        return ""

    adjusted_text = text
    if end_with_period and not text.endswith('.'):
        adjusted_text += '.'

    return "%s\n\n" % adjusted_text


def anchor(target: str) -> str:
    """ Return an RST anchor for 'target', as a paragraph. """
    return paragraph(".. _%s:" % target)


def heading1(text: str, capitalize: bool = True) -> str:
    """ Format 'text' as H1. """
    title_str = text.title() if capitalize else text
    tag = "#" * len(title_str)
    return paragraph("%s\n%s\n%s" % (tag, title_str, tag))


def heading2(text: str) -> str:
    """ Format 'text' as H2. """
    tag = "*" * len(text)
    return paragraph("%s\n%s\n%s" % (tag, text, tag))


def hr() -> str:
    """ Print a horizontal line ('transition marker'). """
    return paragraph("\n\n-----")


def code_block(code: str, caption: str, language: str = CODE_BLOCK_LANG_JAVASCRIPT) -> str:
    """
    Return caption and code as a code block.

    Note that an "\t%s" % (text) does not indent multi line strings correctly.
    Therefore, the passed in text must be indented with \t line by line.
    """
    code_text = ""
    for line in code.split('\n'):
        code_text += "\t%s\n" % line

    if not caption:
        return paragraph(".. code-block:: %s\n\n%s" % (language, code_text), end_with_period=False)

    caption_stripped = caption.strip().replace('\n', ' ').replace('\r', '')
    return paragraph(".. code-block:: %s\n\t:caption: %s\n\n%s" % (language, caption_stripped, code_text))


def replace_line(alias: str, value: str) -> str:
    """ Create a replace statement: '.. |alias| replace:: value\\n' (single trailing newline, not paragraphed). """
    return ".. |%s| replace:: %s\n" % (alias, value)


def comment_line(text: str) -> str:
    """ Print comment text, as a paragraph. """
    return paragraph(".. %s" % text)


def param_inline(param) -> str:
    """ Format a ParamVM as 'name: **type**', for use in an inline argument list. """
    return "%s: %s" % (param.name, bold(param.type))


def param_detail(param) -> str:
    """ Format a ParamVM as a detailed list item: '* name: **type** comment'. """
    return "* %s: %s %s" % (param.name, bold(param.type), param.comment)


def reflection_signature_line(sig) -> str:
    """ Format a reflection-based property signature as one paragraphed line, return type unbolded. """
    args_str = ", ".join(param_inline(p) for p in sig.params)
    return paragraph("%s (%s): %s" % (sig.name, args_str, sig.return_type))


def render_comment_segments(segments) -> str:
    """ Render a list of CommentSegmentVM as concatenated paragraphs/code blocks. """
    result = ""
    for seg in segments:
        if seg.kind == "text":
            result += paragraph(seg.text)
        else:
            result += code_block(seg.text, seg.caption)
    return result


def render_inherited_from(inherited) -> str:
    """ Format an InheritedFromVM as an 'inherited from ...' paragraph. """
    return paragraph("inherited from %s " % inherited_text(inherited))


def render_signature(sig) -> str:
    """ Format a SignatureVM: header line, comment, optional parameter list. """
    args_str = ", ".join(param_inline(p) for p in sig.params)
    result = paragraph("%s (%s): %s" % (bold(sig.name), args_str, bold(sig.return_type)))
    result += render_comment_segments(sig.comment)
    if sig.params:
        result += paragraph(bold("Parameters"))
        params_text = "".join("%s\n" % param_detail(p) for p in sig.params)
        result += paragraph(params_text)
    return result


def render_method_body(signatures) -> str:
    """ Format a method's signature list; used for both standalone method files and detail children. """
    inner = "".join(render_signature(sig) for sig in signatures)
    return paragraph(paragraph(inner))


def render_method(m) -> str:
    """ Format a method detail child: anchor + signature list. """
    return anchor(m.uid) + render_method_body(m.signatures)


def render_reflection_property(p) -> str:
    """ Format the reflection-typed body of a PropertyVM (function-valued property). """
    inner = "".join(reflection_signature_line(sig) for sig in p.signatures)
    if p.inherited_from:
        inner += render_inherited_from(p.inherited_from)
    return paragraph(inner) + paragraph(render_comment_segments(p.comment))


def render_non_reflection_property(p) -> str:
    """ Format the plain-typed body of a PropertyVM. """
    if p.readonly:
        header = "%s: %s %s" % (bold(p.name), p.type, italic("[read-only]"))
    else:
        header = "%s: %s" % (bold(p.name), p.type)

    result = paragraph(header)
    if p.inherited_from:
        result += render_inherited_from(p.inherited_from)
    result += paragraph(render_comment_segments(p.comment))
    return result


def render_property(p) -> str:
    """ Format a property detail child: anchor + reflection or plain body. """
    body = render_reflection_property(p) if p.is_reflection else render_non_reflection_property(p)
    return anchor(p.uid) + body


def render_enum_member(e) -> str:
    """ Format an enum member detail child: anchor + name + comment. """
    return anchor(e.uid) + "%s %s" % (bold(e.name), render_comment_segments(e.comment))


class Printer:
    """ Jinja2 template renderer for the converter output files. """

    def __init__(self, templates_path: pathlib.Path = TEMPLATES_PATH) -> None:
        self.environment = Environment(loader=FileSystemLoader(str(templates_path)),
                                       trim_blocks=True, lstrip_blocks=True, keep_trailing_newline=True)

        self.environment.filters['autolink'] = autolink
        self.environment.filters['link_ref'] = link_ref
        self.environment.filters['paragraph'] = paragraph
        self.environment.filters['anchor'] = anchor
        self.environment.filters['heading1'] = heading1
        self.environment.filters['heading2'] = heading2
        self.environment.filters['replace_line'] = replace_line
        self.environment.filters['comment_line'] = comment_line
        self.environment.globals['hr'] = hr
        self.environment.globals['bold'] = bold
        self.environment.globals['italic'] = italic

    def render_file(self, file_vm) -> str:
        return self.environment.get_template("%s_file.rst" % file_vm.shape).render(file=file_vm)

    def render_index(self, index_vm) -> str:
        return self.environment.get_template("index.rst").render(
            file=index_vm.file, root_anchor=index_vm.root_anchor, toc_files=index_vm.toc_files)

    def render_shortcuts(self, rows: List) -> str:
        return self.environment.get_template("shortcuts.rst").render(rows=rows)
