#!/usr/bin/python
"""Generate cpp.hint file from C++ source code.

Usage:
    $ python cpphint.py [CONFIG FILE]

Example:
    $ python cpphint.py some_config.json


The configuration file denotes the source files to scan, the output
file to generate and a few additional constraints. Example file format:

####################### exampleconfig.json ##############################

{
    "description": "some config description",
    "extends":
    [
        "path/to/included/config1.json",
        "path/to/included/config2.json"
    ],
    "output": "cpp.hint",
    "sources":
    [
        {
            "path": "path/to/file.h",
            "comment": "some comment for file.h",
            "include_patterns":
            [
                "DEFINE_",
                "IMPLEMENT_
            ],
            "exclude_patterns":
            [
                "_LOG"
            ]


    ],
    "conditions":
    [
        {
        "name": "SOME_CONDITION",
        "value": true
        }
    ]
}

#########################################################################

Attributes description:

* "description":
    Optional, comment describing the config file.

* extends:
    Optional, list of config files to include in a cascading manner.
    Merges 'sources' and 'conditions' from nested files, ignores all
    other attributes. Supports recursion.

    Merge strategy for sources and conditions is depth-first, i.e.
    data from the innermost file is read first, then merged with data
    from outer files.

    A redundant 'condition' definition in an 'inner' file may be replaced
    by an 'outer' file, example:

    Inner extended file contains:
    "conditions":
    [
        {
            "name": "FOO"
            "value": true
        }
    ]

    Outer master file contains:
    "conditions":
    [
        {
            "name": "FOO"
            "value": false
        }
    ]

    -> Effective value will be "false".

* output:
    Optional, output hint file to generate. Visual Studio expects this
    file to be named 'cpp.hint'. Path can be relative to configuration
    file or absolute. Default value: "cpp.hint" located at folder
    containing the config json file.

* sources:
    List of files to scan for macros. Each file entry is defined as a sub-object
    with the following attributes:

    * path:
        Required path to the file to scan. Specify as absolute path or as path relative
        to the config.json file.
    * comment:
        Optional file comment
    * include_patterns:
        Optional macro name patterns to include in this file. Supports regular expressions.
    * exclude_patterns
        Optional macro name patterns to exclude in this file, supports regular expressions.

* conditions:
    Enables parser to handle conditional statements like:

        #ifdef FOO
            #define SOME_MACRO_IF_FOO
        #else
            #define SOME_MACRO_IF_NOT_FOO
        #endif

    Conditions are ignored by default, i.e. the parser scans everything, ignoring
    whether a macro condition is contained in a conditional statement block or not.
    To make the parser respect the condition "FOO", add the following to the
    configuration file:

    {
        "name": "FOO",
        "value": true
    }

    This causes the parser to scan macros from the 'true' block of the conditional
    statement but ignore macros defined in the 'false' (else) block.

    Note: This feature is limited to simple, un-nested single-condition statements
    like '#ifdef FOO' or '#ifndef FOO'. Avoid it if possible.

"""

import argparse
import json
import logging
import os
import pathlib
import sys

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"
__version__ = "0.0.9"

from modules.config import ConfigHandler
from modules.exporter import HintExporter
from modules.parser import MacroParser

cpphint_path = pathlib.Path(__file__).resolve()
cpphint_folder = cpphint_path.parent


def main() -> None:
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
    logging.info(f"CppHint v{__version__}, {__copyright__}")

    parser = argparse.ArgumentParser(description='Generate cpp.hint file from C/C++ source files.')
    parser.add_argument('config_file', metavar='CONFIG_FILE', help='Scan configuration file')

    args = parser.parse_args()
    config_file = pathlib.Path(args.config_file).absolute()

    if not config_file.exists():
        logging.error(f"config file '{config_file}' not found")
        sys.exit(1)

    config_file_path = config_file.parent

    config = ConfigHandler()
    if not config.load(config_file):
        sys.exit(1)

    parser = MacroParser(scan_path=config_file_path, sources=config.sources, conditions=config.conditions)
    macros = parser.run()

    outfile_path = config_file_path.joinpath(config.output_path)
    exporter = HintExporter(outfile_path)
    exporter.run(macros=macros)


if __name__ == "__main__":
    main()
