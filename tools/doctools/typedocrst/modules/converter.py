#!/usr/bin/python
"""TypeDoc JSON-to-RST Converter"""

import logging
import os
import pathlib
from pathlib import Path

from modules.parser import Parser, ParserConfig
from modules.model import Model
from modules.filewriter import FileWriter, RecursiveObjectFileWriter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class TypeDocConverter:

    @staticmethod
    def write_full_documentation(file_in: pathlib.Path, model: Model, config: ParserConfig) -> None:
        """
        Write summary file and top level element individual files.
        """

        # collect these to set them in index file toc tree
        toc_tree_files = list()
        recursive_writer = RecursiveObjectFileWriter(model, config)
        recursive_writer.write_files(toc_tree_files)

        index_file_path = pathlib.Path(config.out_path, "index.rst")
        FileWriter.write_index(model, file_in, index_file_path, toc_tree_files)

    @staticmethod
    def write_shortcut_list(file_in: pathlib.Path, model: Model, config: ParserConfig) -> None:
        """
        Write single file containing shortcuts to all top level elements.
        """
        index_ref_file_path = pathlib.Path(config.out_path, "index.ref.rst")
        FileWriter.write_shortcut_list(model, file_in, index_ref_file_path)

    @staticmethod
    def run(file_in: pathlib.Path, out_path: pathlib.Path, anchor_name: str, write_reference_file: bool) -> None:
        """
        Parse JSON 'file_in', generate output files. Writes
        a summary as well as individual files for various top
        level objects
        """
        config = ParserConfig(
            in_file=file_in,
            out_path=out_path,
            anchor=anchor_name,
            write_refs=write_reference_file
        )

        # set file prefix as model data namespace, is used
        # as global prefix for in-file object references
        model = Model(config.anchor)
        p = Parser(config, model)

        logging.info("reading file %s" % file_in)
        p.run()

        # determine object parents in hierarchical order
        model.build_object_parents_dict()

        # ensure output path exists
        if not out_path.exists():
            logging.info(f"output path {out_path} not found, creating directory")
            out_path.mkdir(parents=True, exist_ok=True)

        # remove suffix to append .rst and .ref.rst if needed
        TypeDocConverter.write_full_documentation(file_in, model, config)
        if config.write_refs:
            TypeDocConverter.write_shortcut_list(file_in, model, config)
