#!/usr/bin/python3
"""CMake Doc generator.

Create reference documentation from cmake macros and functions.
"""

import argparse
import logging
import os
import pathlib
import sys

from modules.converter import CMakeDocConverter
from modules.parser import ParserError

__copyright__ = "Copyright (c) 2025 CCL Software Licensing GmbH"


if __name__ == "__main__":
    """
    Define and parse arguments, forward to and call main().
    """

    # logger configuration
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)

    # args configuration
    parser = argparse.ArgumentParser(description='CMake function and macro reference converter')
    parser.add_argument('outfile', metavar='RST_FILE_OUT', help='rst file to write')
    parser.add_argument('infiles', metavar='CMAKE_FILES_IN', help='.cmake files to parse', nargs="*")

    args = parser.parse_args()
    project_root = pathlib.Path(os.path.abspath(__file__)).parent

    try:
        CMakeDocConverter.run(args.infiles, args.outfile, project_root)
    except ParserError as e:
        logging.error(e)
        sys.exit(1)
