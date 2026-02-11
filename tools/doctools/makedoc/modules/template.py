#!/usr/bin/python
"""Makedoc configwriter module.
"""

import json
import logging
import os
import pathlib
import sys
from dataclasses import dataclass
from datetime import datetime

from typing import List, Optional

from jinja2 import Environment, FileSystemLoader

from modules.buildenv import BuildEnvironment
from modules.pathutil import format_path
from modules.tools import ToolHelper, Tools

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "../../../../../ccl/extras/python"))

from tools.repoinfo import RepositoryInfo

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class VendorConfig:
    """Spinx conf.py attributes: vendor specifics."""

    copyright: str  # copyright string
    author: str  # author name
    logo: str  # name of logo image file (without path)
    header: str  # header background color hex (example: #000000)


@dataclass
class SphinxConfig:
    """Spinx conf.py attributes: non-vendor specific."""

    warning_hint: str
    project_title: str
    exclude_patterns: str
    latex_index: str
    latex_file: str
    build_version: str
    sphinx_common_path: str
    cmakedoc_path: str  # path to cmakedoc tool
    doxyrest_path: str  # full path to doxyrest tool, platform dependent


@dataclass
class DoxyrestConfig:
    """Doxyrest config template variables."""

    doxyrest_path: str  # full path to doxyrest tool binary, platform dependent


@dataclass
class DoxygenConfig:
    """Doxy file config template"""

    doxy_common_path: str


class TemplateWriter:
    """Configuration file writer: reads an *.in file, replaces certain
    placeholder strings using Jinja2 and writes the eventual config
    file used by the documentation build."""

    def __init__(self, env: BuildEnvironment) -> None:
        self.env = env


class SphinxConfigWriter(TemplateWriter):
    """Prepare conf.py file."""

    OUTPUT_FILE = "conf.py"

    def __init__(self, project_path: pathlib.Path, output_path: pathlib.Path, env: BuildEnvironment) -> None:
        super().__init__(env)
        self.project_path = project_path
        self.output_path = output_path

    def _find_vendor_config_file(self, vendor: str) -> Optional[pathlib.Path]:
        """Lookup vendor configuration file from build/identities folder.

        In CCL framework repo context, look in build/identities of CCL framework repo.
        In meta repo context, look in all build/identities provided by repo.json.
        """

        # Default: CCL framework repo context.
        identity_folders: List[pathlib.Path] = [
            RepositoryInfo.get_ccl_build_identities_path()
        ]

        # Meta repo context.
        if self.env.repo_info:
            identity_folders = self.env.repo_info.get_paths(RepositoryInfo.CATEGORY_IDENTITIES)

        display_paths = [str(p) for p in identity_folders]
        logging.info(f"using identity folders: {display_paths}")

        vendor_name = vendor.lower()
        for folder in identity_folders:

            # Ensure constructed vendor_file path will be absolute and must not be resolved.
            assert folder.is_absolute()

            # Expected canonical path, example: "identities/ccl/makedoc.vendor.json"
            vendor_file = pathlib.Path(folder, vendor_name, "makedoc.vendor.json")
            if vendor_file.exists():
                return vendor_file

        return None

    def write(self, exclude_patterns: str, latex_index: str, latex_file: str, title: str, revision: str,
              vendor: str) -> None:
        """Copy over template conf.py, adjust documentation specific settings."""

        ##############################################
        # Get vendor info data from file.
        # Canonical file name is 'makedoc.vendor.json'
        ##############################################

        vendor_file = self._find_vendor_config_file(vendor.lower())
        if vendor_file:
            logging.info(f"using vendor file '{vendor_file}'")

        # Failure handled by keeping VendorInfo in default state.
        vendor_info = VendorConfig(copyright="", author="", header="", logo="")

        try:
            with vendor_file.open(mode="r") as f:
                json_data = json.load(f)

                # Logo path in makedoc.vendor.json may be relative to makedoc.vendor.json
                # file. Ensure it is passed forward to template as absolute path.
                logo_path = pathlib.Path(json_data.get("logo"))
                if not logo_path.is_absolute():
                    logo_path = pathlib.Path(vendor_file.parent, json_data.get("logo")).resolve()

                # Replace year placeholder in copyright.
                copyright_in = str(json_data.get("copyright"))
                copyright_formatted = copyright_in.format(year=datetime.now().year)

                vendor_info = VendorConfig(
                    copyright=copyright_formatted,
                    author=json_data.get("author"),
                    header=json_data.get("header"),
                    logo=format_path(logo_path)
                )
        except (OSError, AttributeError) as e:
            logging.error(f"could not open config file for vendor '{vendor}': {e}")

        ##############################################
        # Prepare vendor agnostic configuration.
        ##############################################

        build_version = f"Build Rev. {revision}" if revision else ""
        doxyrest_shared_path = self.env.tool_helper.find_doxyrest_sphinx_path()
        cmakedoc_path = self.env.tool_helper.find_python_tool(name=Tools.CMAKEDOC,
                                                              category=Tools.CATEGORY_DOCTOOLS).parent
        config = SphinxConfig(warning_hint="# !!! GENERATED FILE, DO NOT EDIT !!!",
                              project_title=title,
                              exclude_patterns=exclude_patterns,
                              latex_index=latex_index,
                              latex_file=latex_file,
                              build_version=build_version,
                              sphinx_common_path=format_path(self.env.sphinx_common_path),
                              cmakedoc_path=format_path(cmakedoc_path),
                              doxyrest_path=format_path(doxyrest_shared_path)
                              )

        ##############################################
        # Load and render template to string.
        ##############################################

        # TODO: modify \date in .tex file directly?
        # conf.py.in is in makedoc/ folder

        template_env = Environment(loader=FileSystemLoader(str(self.env.sphinx_common_path)))
        input_file = f"{SphinxConfigWriter.OUTPUT_FILE}.in"
        template = template_env.get_template(input_file)
        config_str = template.render(vendor=vendor_info, config=config)

        ##############################################
        # Write template string to file.
        ##############################################

        conf_file_path = self.get_output_file()
        with conf_file_path.open(mode="w") as f:
            f.write(config_str)
            logging.info(f"created sphinx configuration '{conf_file_path}'")

    def get_output_file(self) -> pathlib.Path:
        return pathlib.Path(self.output_path, SphinxConfigWriter.OUTPUT_FILE)

    def delete_file(self):
        os.remove(self.get_output_file())


class DoxyrestConfigWriter(TemplateWriter):
    """Write Doxyrest configuration lua file."""

    OUTPUT_FILE = "doxyrest-config.lua"

    def __init__(self, input_file: pathlib.Path, env: BuildEnvironment) -> None:
        super().__init__(env)
        self.input_file = input_file

    def run(self) -> None:
        doxyrest_shared_path = self.env.tool_helper.find_doxyrest_sphinx_path()
        config = DoxyrestConfig(
            doxyrest_path=format_path(doxyrest_shared_path)
        )

        # Load and render template to string.
        template_path = self.input_file.parent
        template_env = Environment(loader=FileSystemLoader(template_path))
        template = template_env.get_template(self.input_file.name)
        config_str = template.render(config=config)

        # Write template string to file.
        output_file_path = pathlib.Path(self.input_file.parent, DoxyrestConfigWriter.OUTPUT_FILE)
        with output_file_path.open(mode="w") as f:
            f.write(config_str)
            logging.info(f"created doxyrest configuration '{output_file_path}'")


class DoxygenConfigWriter(TemplateWriter):
    OUTPUT_FILE = "config.doxy"

    def __init__(self, input_file: pathlib.Path, env: BuildEnvironment) -> None:
        super().__init__(env)
        self.input_file = input_file

    def run(self) -> None:
        config = DoxygenConfig(
            doxy_common_path=format_path(self.env.doxygen_common_path)
        )

        # Load and render template to string.
        template_path = self.input_file.parent
        template_env = Environment(loader=FileSystemLoader(template_path))
        template = template_env.get_template(self.input_file.name)
        config_str = template.render(config=config)

        # Write template string to file.
        output_file_path = pathlib.Path(self.input_file.parent, DoxygenConfigWriter.OUTPUT_FILE)
        with output_file_path.open(mode="w") as f:
            f.write(config_str)
            logging.info(f"created doxy configuration '{output_file_path}'")
