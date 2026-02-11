"""TypeScript converter file writer support class module."""

import logging
import pathlib
from dataclasses import dataclass
from typing import List

from modules.typedocjson import *
from modules.model import Model
from modules.parser import ParserConfig
from modules.printer import ShortcutListPrinter
from modules.elements import ObjectBuilder
import os

from modules.rstprinter import RstPrinter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class TocTreeEntry:
    name: str  # for sorting
    filename: pathlib.Path


class FileWriter:

    @staticmethod
    def write_index(model, file_in: pathlib.Path, file_out: pathlib.Path, toc_tree_files: List[TocTreeEntry]) -> None:
        """ Write index (summary) file. """

        project_index_str = ObjectBuilder.create_file_object(model.root_object, model).to_rst()
        if not project_index_str:
            logging.error("failed to create output from input file %s " % file_in)
            return

        # the document title results from the typedoc config project name
        result = RstPrinter.anchor(model.get_root_anchor())
        result += project_index_str

        # include toc-tree for navigation purpose in both html and pdf use case
        result += RstPrinter.h2("Index")

        # create new list of filenames sorted by object name
        filenames = [entry.filename for entry in sorted(toc_tree_files, key=lambda k: k.name)]
        result += RstPrinter.toctree(filenames, 1, False)

        with file_out.open('w+') as fp:
            fp.write(result)
            logging.info("wrote index summary %s" % file_out)

    @staticmethod
    def build_unique_file_name(data_namespace: str, kind_string: str, object_name: str) -> str:
        """ Must be kind specific, names can redundantly exist on classes and namespaces. """
        file_name = "%s_%s_%s" % (data_namespace, kind_string, object_name)
        return file_name.lower().replace(" ", "")

    @staticmethod
    def write_object_file(model: Model, config: ParserConfig, obj: JSONObject, obj_full_name: str) -> str:
        """
        Create a single file for 'obj', interpreted as a top level object.
        Intended for use with classes or interfaces.

        :return name of generated file, excluding path and suffix
        """

        out_path = config.out_path
        data_namespace = config.anchor
        object_name = obj_full_name
        object_kind_string = obj.get(TDAttribute.KIND_STRING)

        # create filename containing the object name w/o suffix, the raw
        # name without suffix is needed for index toc tree
        toc_tree_entry_filename = FileWriter.build_unique_file_name(data_namespace, object_kind_string, object_name)
        file_path = pathlib.Path(out_path, toc_tree_entry_filename + ".rst")

        # generate file content, assume TopLevelFileObject structure
        rst_string = ObjectBuilder.create_file_object(obj, model).to_rst()

        with file_path.open('w+') as fp:
            fp.write(rst_string)
            logging.info("wrote %s for object %s" % (file_path, object_name))

        return toc_tree_entry_filename

    @staticmethod
    def write_shortcut_list(model: Model, file_in: pathlib.Path, file_out: pathlib.Path) -> None:

        data_str = ShortcutListPrinter.print(model)
        if not data_str:
            logging.error(f"failed to create output from input file {file_in}")
            return

        with file_out.open('w+') as fp:
            fp.write(data_str)
            logging.info(f"wrote shortcut references file {file_out}")


class RecursiveObjectFileWriter:
    """
    Generate individual files for all objects considered top level.
    Ensure full object name path to support redundant object names
    in different object trees.
    """

    def __init__(self, model: Model, config: ParserConfig) -> None:
        self.model = model
        self.config = config

    @staticmethod
    def build_hierarchic_name(parent: str, current: str) -> str:
        return "%s_%s" % (parent, current)

    def _recurse(self, obj, full_name, toc_tree_list: List[TocTreeEntry]) -> None:
        """
        Create shortcuts for all sub elements recursively. Stops at
        an element that has no more groups (methods, attributes, ...).
        """

        # must be supported kind
        object_kind_id = obj.get(TDAttribute.REFLECTION_KIND)
        if not self.model.is_exported_to_separate_file(object_kind_id):
            return

        # export to file, save file name for toc tree
        toc_tree_entry = FileWriter.write_object_file(self.model, self.config, obj, full_name)
        if toc_tree_entry:
            toc_tree_list.append(
                TocTreeEntry(
                    name=obj.get(TDAttribute.NAME),
                    filename=toc_tree_entry
                )
            )

        # Check for recursion end, i.e. no more groups contained). Assume
        # a property (method, ...), export as PropertyShortcut, ID based.
        group_objects = obj.get(TDAttribute.GROUPS)
        if group_objects is None:
            return

        # groups only contain IDs, resolve objects
        for group_object in group_objects:
            children_objects = self.model.get_object_children(group_object)
            for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
                child_full_name = RecursiveObjectFileWriter.build_hierarchic_name(full_name,
                                                                                  child_object.get(TDAttribute.NAME))
                self._recurse(child_object, child_full_name, toc_tree_list)

    def write_files(self, toc_tree_list: List[TocTreeEntry]) -> None:
        group_objects = self.model.root_object.get(TDAttribute.GROUPS)
        for group_object in sorted(group_objects, key=lambda k: k['title']):
            children_objects = self.model.get_object_children(group_object)
            for child_object in sorted(children_objects, key=lambda k: k[TDAttribute.NAME]):
                obj_name = child_object.get(TDAttribute.NAME)
                self._recurse(child_object, obj_name, toc_tree_list)
