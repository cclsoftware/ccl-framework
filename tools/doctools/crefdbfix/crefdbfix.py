#!/usr/bin/python3
"""Helper script: omit redundant keys from doxyrest crefdb.py file.

Note: workaround, doxyrest should fix how the crefdb.py file is created.
It currently may contain redundant keys (invalid dict) and the keys are
not specific enough.
"""

import argparse
import logging

import pathlib

from modules.handler import CrefDBHandler

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

if __name__ == "__main__":
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)

    parser = argparse.ArgumentParser(description='Doxyrest crefdb helper script')
    parser.add_argument('infile', metavar='INFILE', help='crefdb input file')
    parser.add_argument('outfile', metavar='OUTFILE', help='crefdb output file')

    args = parser.parse_args()

    handler = CrefDBHandler(pathlib.Path(args.infile), pathlib.Path(args.outfile))
    handler.run()
