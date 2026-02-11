#!/usr/bin/python
"""Classmodel converter."""

import json
import logging
import os
from dataclasses import dataclass
from pathlib import Path
from typing import List
from xml.etree import ElementTree

from modules.classmodel.model import Model
from modules.classmodel.parser import Parser
from modules.classmodel.printer import ClassModelPrinter, ClassModelRefPrinter, ClassModelIndexPrinter, \
    ClassModelHtmlRefPrinter
from modules.classmodel.elementfilter import ElementFilter, ElementListFile

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class JSONConfig:
    input_file: str
    output_file: str
    include_list_file: str
    exclude_list_file: str
    external_files_path: str
    ref_prefix: str
    create_shortcut_file: bool
    create_index_file: bool


class ClassModelConverter:

    @staticmethod
    def _write_file(content: str, file_out: str, file_extension: str) -> None:
        """Prepare output directory and filename, write content to file."""

        head, tail = os.path.split(file_out)
        out_dir = Path(os.getcwd(), head)
        out_dir.mkdir(parents=True, exist_ok=True)

        filename = f"{tail}.{file_extension}"
        out_path = Path(out_dir, filename)
        try:
            with open(out_path, 'w') as fp:
                fp.write(content)
            logging.info(f"wrote file {out_path}")
        except OSError as e:
            logging.error(f"could not write file {out_path}: {e}")

    @staticmethod
    def _write_rst(model, file_out, anchor, write_shortcuts_file, write_index_file):
        """ Write RST format, also write shortcuts file if requested. """
        content_str = ClassModelPrinter.model_data_to_string(model, anchor)
        ClassModelConverter._write_file(content_str, file_out, "rst")

        if write_shortcuts_file:
            # RST shortcuts
            content_str = ClassModelRefPrinter.model_data_to_string(model, anchor)
            ClassModelConverter._write_file(content_str, file_out, "ref.rst")
            # HTML links
            content_str = ClassModelHtmlRefPrinter.model_data_to_string(model, anchor)
            ClassModelConverter._write_file(content_str, file_out, "refs.html")

        if write_index_file:
            content_str = ClassModelIndexPrinter.model_data_to_string(model)
            ClassModelConverter._write_file(content_str, file_out, "index.txt")

    @staticmethod
    def _run_for_job_config(config: JSONConfig):
        logging.info("processing file %s" % config.input_file)

        include_list = None
        if config.include_list_file:
            include_list = ElementListFile(config.include_list_file)

        exclude_list = None
        if config.exclude_list_file:
            exclude_list = ElementListFile(config.exclude_list_file)

        model = Model()
        element_filter = ElementFilter(include_list, exclude_list)
        p = Parser(config.ref_prefix, config.external_files_path, model, element_filter)
        try:
            p.parse_file(config.input_file)
        except ElementTree.ParseError as e:
            logging.error("job cancelled for %s: %s" % (config.input_file, e))
            return

        if config.output_file is not None:
            ClassModelConverter._write_rst(model=model, file_out=config.output_file, anchor=config.ref_prefix,
                                           write_shortcuts_file=config.create_shortcut_file,
                                           write_index_file=config.create_index_file)

    @staticmethod
    def _determine_prefix(prefix: str, output_file: str):
        """ Get prefix for use in references. Use explicit option or fallback to rst. """

        # Use prefix as configured
        if prefix:
            return prefix

        # Fallback to file base name as default
        if output_file:
            out_file_split = os.path.split(output_file)
            out_file_name = out_file_split[1]
            return os.path.splitext(out_file_name)[0]

    @staticmethod
    def _load_config(path) -> List[JSONConfig]:
        """ Load JSON 'files' section to individual config objects. """
        configs: List[JSONConfig] = list()
        with open(path) as json_config_file:
            data = json.load(json_config_file)
            for entry in data["files"]:
                prefix = ClassModelConverter._determine_prefix(entry.get("ref-prefix", ""), entry.get("output", ""))
                configs.append(
                    JSONConfig(
                        input_file=entry.get("input", ""),
                        output_file=entry.get("output", ""),
                        include_list_file=entry.get("include-list-file", ""),
                        exclude_list_file=entry.get("exclude-list-file", ""),
                        ref_prefix=prefix,
                        external_files_path=entry.get("external-files-path", "."),  # use cwd as default
                        create_shortcut_file=entry.get("generate-shortcut-file", False),
                        create_index_file=entry.get("generate-index-file", False)
                    )
                )

        return configs

    @staticmethod
    def run_config(config_path):
        """ Import configuration from external file. """

        job_config_files = ClassModelConverter._load_config(config_path)
        for job_config in job_config_files:
            ClassModelConverter._run_for_job_config(job_config)
