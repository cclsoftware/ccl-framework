#!/usr/bin/python3
"""Sphinx extension introducing the 'cmake' role, allowing
references to custom and external cmake commands.
"""
import json
import os
import warnings
from typing import Optional

from docutils import nodes
from sphinx import addnodes
from sphinx.application import Sphinx

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

this_dir = os.path.dirname(os.path.realpath(__file__))

cmakedb = {}


def get_cmake_ref_target(command: str) -> Optional[str]:
    """Retrieve cmake command ref target for 'command'."""

    if command in cmakedb:
        return cmakedb[command]

    warnings.warn('internal cmake target not found for cmake: ' + command, Warning, 2)
    return None


def create_xref_node(raw_text, text, target):
    """Create cross-references node. Resolved before writing output, in
    BuildEnvironment.resolve_references.
    """

    node = addnodes.pending_xref(raw_text)
    node['reftype'] = 'ref'
    node['refdomain'] = 'std'
    node['reftarget'] = target
    node['refwarn'] = True
    node['refexplicit'] = True
    node += nodes.Text(text, text)
    return node


def handle_cmake_role(typ, raw_text, text, lineno, inliner, options={}, content=[]):
    """Process cmake role: create xref to internal cmake command target."""

    if text.find(' ') == -1:
        node = nodes.literal(raw_text, '')
    else:
        node = nodes.inline(raw_text, '')

    target = get_cmake_ref_target(text)
    if target:
        node += create_xref_node(raw_text, text, target)
    else:
        node += nodes.Text(text, text)

    # Add unique (CSS) class so this node does not get transformed by other
    # extensions, see doxyrest RefTransform node matching for example.
    node['classes'] += ['cmake-ref']
    return [node], []


def handle_cmakelink_role(typ, raw_text, text, lineno, inliner, options={}, content=[]):
    """Process cmakelink role: create weblink to external cmake command documentation."""

    pattern = "https://cmake.org/cmake/help/latest/command/%s.html"
    url = pattern % (text,)
    node = nodes.reference(raw_text, text, refuri=url, **options)

    # Add unique (CSS) class so this node does not get transformed by other
    # extensions, see doxyrest RefTransform node matching for example.
    node['classes'] += ['cmake-link']
    return [node], []


def add_css(app: Sphinx) -> None:
    """ Register stylesheet."""

    supported_themes = ["sphinx_rtd_theme"]
    active_theme = app.config.html_theme
    if active_theme not in supported_themes:
        warnings.warn(f"unsupported theme {active_theme}")
        return

    if active_theme != "sphinx_rtd_theme":
        warnings.warn(f"unsupported theme {active_theme}")
        return

    css_file = f"cmakerole-{active_theme}.css"
    app.config.html_static_path += [this_dir + '/css/' + css_file]
    app.add_css_file(css_file)


def load_cmakedb(app: Sphinx) -> None:
    """ Load references db."""
    db_file = "cmakedb.json"

    for basedir, dirnames, filenames in os.walk(app.srcdir):
        if db_file in filenames:
            db_file_path = os.path.join(basedir, db_file)
            try:
                with open(db_file_path) as f:
                    db_file_data = json.load(f)
                    global cmakedb
                    # Append here as there could be multiple
                    # projects providing this file.
                    cmakedb.update(db_file_data)
            except OSError:
                warnings.warn(f"could not open db file {db_file_path}")
                return


def on_builder_inited(app: Sphinx):
    """Configure app and extension on builder init."""

    add_css(app)
    load_cmakedb(app)


def setup(app: Sphinx):
    """Extension setup, called by Sphinx."""

    app.add_role('cmake', handle_cmake_role)
    app.add_role('cmakelink', handle_cmakelink_role)
    app.connect('builder-inited', on_builder_inited)
