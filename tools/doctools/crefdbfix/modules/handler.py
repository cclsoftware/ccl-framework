#!/usr/bin/python3
"""crefdb handler class:
Omit redundant keys from doxyrest generated crefdb dictionary.
"""

import logging
from pathlib import Path


class CrefDB:
    """Identifiers related to Doxyrest output."""

    DOXY_NAMESPACE = "doxid-namespace"
    DOXY_STRUCT = "doxid-struct"
    DICT_VAR_START = "crefdb = {"


class CrefDBHandler:
    """Read, unify and write crefdb.py file."""

    def __init__(self, infile: Path, outfile: Path):
        self.infile = infile
        self.outfile = outfile
        self.result = dict()

    def run(self) -> None:
        """Read input file dictionary, unify it, pretty-print
        it to output file.

        Note: do not import the original crefdb.py file as
        redundant keys - potentially the wrong ones - will be
        omitted during deserialization.
        """

        #######################################################
        # Step 1: parse and unify input dictionary.
        #######################################################

        self.result.clear()

        try:
            logging.info(f"reading file '{self.infile}'")
            with self.infile.open("r", encoding="utf-8") as f:
                lines = f.readlines()
        except IOError:
            logging.error(f"could not open file '{self.infile}'")
            return

        # Skip uninteresting lines in file header.
        read_mode = False
        for line in lines:
            # Start of dictionary.
            if CrefDB.DICT_VAR_START in line:
                read_mode = True
                continue
            # End of dictionary.
            if "}" in line:
                break
            # Within dictionary.
            if read_mode:
                tokens = line.split(": ")
                if len(tokens) == 2:
                    self._add_key(CrefDBHandler._strip(tokens[0]), CrefDBHandler._strip(tokens[1]))
                else:
                    logging.error(f"failed to parse line {line}")

        #######################################################
        # Step 2: pretty print result dict to output file.
        #######################################################

        content = "crefdb = {\n"
        for key, value in self.result.items():
            content += f"\t'{key}' : '{value}',\n"

        # omit trailing ',\n' for last element
        content = content[:-2]
        content += "\n}"

        try:
            with self.outfile.open("w", encoding="utf-8") as f:
                f.write(content)
                logging.info(f"wrote file '{self.outfile}'")
        except IOError:
            logging.error(f"could not open file '{self.outfile}'")

    @staticmethod
    def _strip(t: str) -> str:
        """Strip Python syntax and formatting related characters from t."""
        return t.strip().replace("'", "").replace(",", "")

    def _add_key(self, key: str, value: str) -> None:
        """Register unique key or log key conflict.

        Main purpose of this script is to ensure struct > namespace.
        An occurrence of namespace > struct is an error. This ensures
        that a link targets a specific class or interface, not the class
        listed as a member of an interface in a namespace overview.

        There may be other conflicts as well, examples: redundant
        defines ('DEBUG_LOG') or overloaded functions.
        """

        if key not in self.result:
            self.result[key] = value
            return

        existing = self.result[key]
        if CrefDB.DOXY_STRUCT in existing and CrefDB.DOXY_NAMESPACE in value:
            logging.info(f"fixed conflict for '{key}'")
        elif CrefDB.DOXY_NAMESPACE in existing and CrefDB.DOXY_STRUCT in value:
            logging.error(f"wrong target for '{key}'")
        else:
            logging.warning(f"unexpected conflict for '{key}'")
