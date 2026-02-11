#!/usr/bin/python
"""TypeDoc JSON-to-RST Conversion Tool"""

import argparse
import logging
import os
import pathlib

from modules.converter import TypeDocConverter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

if __name__ == "__main__":
    """
    Define and parse arguments, forward to and call main().
    """

    # logger configuration
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)

    # args configuration
    parser = argparse.ArgumentParser(description='TypeDoc JSON-to-RST converter')
    parser.add_argument('infile', metavar='TYPEDOC_JSON_FILE_IN', help='TypeDoc generated JSON input file')
    parser.add_argument('outpath', metavar='RST_PATH_OUT', help='rest output files path')
    parser.add_argument('--refs', action='store_true', help='also write reference shortcuts file')

    args = parser.parse_args()
    write_reference_file = vars(args)['refs']

    # given "/some/path/out/file.rst", use "file" as a prefix for all reference links in the document
    in_file_name = os.path.basename(args.infile)

    # prefix for rst references
    (anchor_name, ext) = os.path.splitext(in_file_name)

    TypeDocConverter.run(pathlib.Path(args.infile), pathlib.Path(args.outpath), anchor_name, write_reference_file)
