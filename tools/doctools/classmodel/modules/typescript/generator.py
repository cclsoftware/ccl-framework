#!/usr/bin/python
"""TypeScript generator."""

import json
import logging
import os
import pathlib

from jinja2 import Template
from modules.classmodel.model import Model
from modules.classmodel.parser import Parser
from modules.classmodel.elementfilter import ElementFilter
from modules.typescript.builder import DeclarationBuilder, DtsElementFilter
from modules.typescript.elements import Printable

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class TypeScriptGenerator:

    def __init__(self, config_path: pathlib.Path) -> None:
        with open(config_path) as json_config_file:
            self.json_config = json.load(json_config_file)

    def run(self) -> None:

        ####################################################
        # Step 1: Parse classmodel files, store in model
        ####################################################

        input_model_files = self.json_config.get("models", [])
        model = Model()
        for model_file in input_model_files:
            logging.info("processing file %s" % model_file)

            # TODO: future improvement, use own parser, do not depend on rst converter parser
            # No use for element filters here, typedoc generator uses own filter
            p = Parser(ref_prefix="", external_files_path="", model=model, element_filter=ElementFilter())
            p.parse_file(model_file)

        ####################################################
        # Step 2: Generate dts model
        ####################################################

        allowed_classes = self.json_config.get("classes", [])
        allowed_objects = self.json_config.get("objects", [])
        dts_filter = DtsElementFilter(allowed_classes, allowed_objects)

        generate_comments = self.json_config.get("comments", False)
        root_namespace = self.json_config.get("namespace", False)
        app_name = self.json_config.get("app")
        builder = DeclarationBuilder(model, root_namespace, dts_filter, generate_comments, app_name)
        dts: Printable = builder.run()

        ####################################################
        # Step 3: write output file
        ####################################################

        dts_str = dts.to_string()

        # Optional: embed into template
        template_file = self.json_config.get("template", "")
        if template_file:
            template_file_path = pathlib.Path(os.getcwd(), template_file)
            with template_file_path.open(mode="r") as f:
                template = Template(f.read())
                dts_str = template.render(typings=dts_str)

        output_file = pathlib.Path(os.getcwd(), self.json_config["output"])

        # Prepare output directory
        head, tail = os.path.split(output_file)
        out_dir = pathlib.Path(os.getcwd(), head)
        if not out_dir.exists():
            out_dir.mkdir(parents=True, exist_ok=True)

        with output_file.open(mode="w", encoding="utf-8") as f:
            f.write(dts_str)
