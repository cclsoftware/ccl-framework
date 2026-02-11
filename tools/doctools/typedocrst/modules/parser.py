"""TypeDoc JSON parser."""

import json
import pathlib
from dataclasses import dataclass

from modules.model import *
from modules.typedocjson import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


@dataclass
class ParserConfig:
    in_file: pathlib.Path  # file to parse
    out_path: pathlib.Path  # output path
    anchor: str  # for use in anchors in the rst output, excluding path
    write_refs: bool  # also generate references file


class Parser:
    """
    TypeDoc JSON parser, convert JSON objects to RST format.
    """

    def __init__(self, config: ParserConfig, model: Model) -> None:
        """
        Parameterized construction
        :param config: parser configuration
        """
        self.config = config
        self.classes_cache = dict()
        self.model = model

    def _parse_children(self, children: List[JSONObject]) -> None:
        """
        Children tree recursion
        :param children  all json objects found under "children" attribute
        """
        if children is None:
            return

        for c in children:
            # store in model
            child_id = c.get(TDAttribute.ID)
            self.model.add_object(child_id, c)

            # advance via recursion
            self._parse_children(c.get(TDAttribute.CHILDREN))

    def run(self) -> None:
        """Parse JSON file to model."""

        # data is stored as siblings
        with self.config.in_file.open() as f:
            self.model.root_object = json.load(f)

        # root element (id = 0) must not be added
        children_list = self.model.root_object.get(TDAttribute.CHILDREN)
        self._parse_children(children_list)

        logging.info(f"found {self.model.count_objects()} objects")
