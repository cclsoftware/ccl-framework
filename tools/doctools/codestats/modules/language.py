#!/usr/bin/python
"""CodeStats tool language module.
"""

import json
from pathlib import Path
from typing import List, Dict, Set

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class LanguageManager:
    """Manage supported languages and file extensions per language. """

    def __init__(self) -> None:
        self.file_extensions: List[str] = []  # flattened file extensions list
        self.file_extensions_map: Dict[str, str] = {}  # map file extension to language name

    def _add_language(self, name: str, file_extensions: List[str]) -> None:
        """ Register new language, update lookup containers. """

        for extension in file_extensions:
            self.file_extensions_map[extension] = name

        self.file_extensions.extend(file_extensions)

    def load_from_config(self, path: Path):
        """ Read language config from JSON file. """

        with path.open('r') as lang_config_file:
            lang_config = json.load(lang_config_file)
            for language in lang_config['languages']:
                self._add_language(name=language['name'], file_extensions=language['extensions'])

    def get_language_name(self, file_extension: str) -> str:
        """ Retrieve language associated with file extension. """

        if file_extension not in self.file_extensions_map:
            return "Unknown"

        return self.file_extensions_map[file_extension]

    def get_file_extensions(self) -> List[str]:
        """ Get aggregated list of file extensions. """

        return self.file_extensions

    def get_language_names(self) -> Set[str]:
        """ Return all language names, unify when collecting. """

        return set(language_name for language_name in self.file_extensions_map.values())
