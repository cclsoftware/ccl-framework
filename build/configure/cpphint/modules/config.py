"""CppHint configload module.
"""
import json
import logging
import pathlib
import re
from typing import List, Dict, Optional

from modules.shared import Condition, Source

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class CppHintConfig:
    """Attributes supported by cpphint.json config file."""

    # Root object attributes.
    ATTR_DESCRIPTION = "description"
    ATTR_EXTENDS = "extends"
    ATTR_OUTPUT = "output"
    ATTR_SOURCES = "sources"
    ATTR_CONDITIONS = "conditions"

    # 'Condition' sub-object attributes.
    ATTR_CONDITION_NAME = "name"
    ATTR_CONDITION_VALUE = "value"


class ConfigSourceEntry:
    """Attributes of 'source' sub-object."""

    ATTR_PATH = "path"
    ATTR_COMMENT = "comment"
    ATTR_INCLUDE_PATTERNS = "include_patterns"
    ATTR_EXCLUDE_PATTERNS = "exclude_patterns"


class ConfigHandler:

    def __init__(self):
        self.description: str = ""
        self.output_path: Optional[pathlib.Path] = None
        self.sources: List[Source] = list()
        self.conditions: List[Condition] = list()

    def load(self, config_file: pathlib.Path) -> bool:

        self._reset()

        ##########################################################
        # Load master config file
        ##########################################################

        logging.info(f"loading config '{config_file}")

        try:
            with config_file.open(mode='r') as f:
                data = json.load(f)

                self.description = data.get(CppHintConfig.ATTR_DESCRIPTION, "")

                output = data.get(CppHintConfig.ATTR_OUTPUT, "")
                self.output_path = ConfigHandler._setup_output_path(config_file.parent, output)

                extends = data.get(CppHintConfig.ATTR_EXTENDS, list())
                conditions = data.get(CppHintConfig.ATTR_CONDITIONS, list())
                sources = data.get(CppHintConfig.ATTR_SOURCES, list())
        except (OSError, json.JSONDecodeError) as e:
            logging.error(f"could not open config file '{config_file}': {e}")
            return False

        ######################################################################
        # Load sources and conditions from extended files recursively.
        ######################################################################

        merged_sources: List[Source] = list()
        merged_conditions: List[Condition] = list()

        for extended_config in extends:
            self._load_extended_config_recursive(sources=merged_sources, conditions=merged_conditions,
                                                 including_file=config_file, included_file=extended_config)

        # Add sources and conditions from master config.
        config_file_path = config_file.parent
        converted_sources = self._load_sources(config_file_path, sources)
        converted_conditions = self._load_conditions(conditions)
        ConfigHandler._merge_sources(merged_sources, converted_sources)
        ConfigHandler._merge_conditions(merged_conditions, converted_conditions)

        self.sources = merged_sources
        self.conditions = merged_conditions

        return True

    def _reset(self):
        self.sources = list()
        self.conditions = list()
        self.description = ""
        self.output_path = None

    @staticmethod
    def _setup_output_path(config_file_path: pathlib.Path, output_file_str: str) -> pathlib.Path:
        """Resolve output path. If no output value is given, default to 'cpp.hint'
        at config file folder location. Otherwise, resolve relative-to-config path.
        Returned path is absolute.
        """

        if not output_file_str:
            return pathlib.Path(config_file_path, "cpp.hint").resolve()

        path = pathlib.Path(output_file_str)
        if not path.is_absolute():
            path = config_file_path.joinpath(path).resolve()

        return path

    @staticmethod
    def _load_extended_config_recursive(sources: List[Source], conditions: List[Condition],
                                        including_file: pathlib.Path, included_file: str) -> None:
        """Attempt to oad sources and conditions from an extended config file in a recursive manner.

        :param sources:  [out] sources to add to
        :param conditions:  [out] conditions to add to, output argument
        :param including_file:  config file that includes 'included_file'
        :param included_file:  config included by 'including file'
        """

        config_file = including_file.parent.joinpath(included_file).resolve()
        try:
            with config_file.open(mode='r') as f:
                data = json.load(f)
                extends = data.get(CppHintConfig.ATTR_EXTENDS, list())
                extended_sources = data.get(CppHintConfig.ATTR_SOURCES, list())
                extended_conditions = data.get(CppHintConfig.ATTR_CONDITIONS, list())
                logging.info(f"including sources and conditions from extended config '{config_file}'")
        except (OSError, json.JSONDecodeError) as e:
            logging.error(f"could not open extended config file '{config_file}': {e}")
            return

        for extended_config in extends:
            ConfigHandler._load_extended_config_recursive(sources=sources, conditions=conditions,
                                                          including_file=config_file, included_file=extended_config)

        # Merge data
        config_file_path = config_file.parent
        converted_sources = ConfigHandler._load_sources(config_file_path, extended_sources)
        converted_conditions = ConfigHandler._load_conditions(extended_conditions)
        ConfigHandler._merge_sources(sources, converted_sources)
        ConfigHandler._merge_conditions(conditions, converted_conditions)

    @staticmethod
    def _load_conditions(conditions_raw: List[dict]) -> List[Condition]:
        """Convert conditions from dictionary to dataclass types."""

        result: List[Condition] = list()
        for cond in conditions_raw:
            name = cond.get(CppHintConfig.ATTR_CONDITION_NAME, "")
            assert name
            value = cond.get(CppHintConfig.ATTR_CONDITION_VALUE, False)
            result.append(Condition(name, value))

        return result

    @staticmethod
    def _compile_patterns(patterns: List[str]) -> List[re.Pattern]:
        """Compile list of regular expression patterns from string."""

        result = list()
        for pattern in patterns:
            result.append(re.compile(pattern))

        return result

    @staticmethod
    def _load_sources(config_path: pathlib.Path, sources: List[Dict]) -> List[Source]:
        """Make all file paths absolute, using config_path as reference."""

        result: List[Source] = list()
        for source in sources:

            # Path is mandatory for source to be processable.
            path_str = source.get(ConfigSourceEntry.ATTR_PATH, "")
            if not path_str:
                logging.error(f"source '{source}' in config '{config_path}' is missing 'path' attribute")
                continue

            # Make relative paths absolute.
            path = pathlib.Path(path_str)
            if not path.is_absolute():
                path = config_path.joinpath(path).resolve()

            comment = source.get(ConfigSourceEntry.ATTR_COMMENT, "")
            include_patterns = source.get(ConfigSourceEntry.ATTR_INCLUDE_PATTERNS, list())
            exclude_patterns = source.get(ConfigSourceEntry.ATTR_EXCLUDE_PATTERNS, list())

            result.append(Source(path=path, comment=comment,
                                 include_patterns=ConfigHandler._compile_patterns(include_patterns),
                                 exclude_patterns=ConfigHandler._compile_patterns(exclude_patterns)))

        return result

    @staticmethod
    def _merge_source(source: Source, other: Source) -> None:
        """Overwrite attributes in 'source' with attributes from 'other'."""
        source.comment = other.comment
        source.include_patterns = other.include_patterns
        source.exclude_patterns = other.exclude_patterns

    @staticmethod
    def _merge_sources(sources: List[Source], sources_to_add: List[Source]) -> None:
        """Contribute to 'sources', existing entries are
         updated, missing entries are added.
        """

        for source in sources_to_add:
            existing = next((s for s in sources if s == source), None)
            if existing:
                ConfigHandler._merge_source(existing, source)
            else:
                sources.append(source)

    @staticmethod
    def _merge_condition(condition: Condition, other: Condition) -> None:
        """Overwrite condition value with 'other' value."""
        condition.value = other.value

    @staticmethod
    def _merge_conditions(conditions: List[Condition], conditions_to_add: List[Condition]) -> None:
        """Contribute to 'conditions', existing entries are
         updated, missing entries are added.
        """

        for condition in conditions_to_add:
            existing = next((c for c in conditions if c == condition), None)
            if existing:
                ConfigHandler._merge_condition(existing, condition)
            else:
                conditions.append(condition)
