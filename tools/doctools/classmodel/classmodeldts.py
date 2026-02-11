#!/usr/bin/python3
"""Convert classmodel files to TypeScript script declaration files."""

import argparse
import logging
import pathlib

from modules.typescript.generator import TypeScriptGenerator

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"
__version__ = "1.0.0"


def main() -> None:
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
    logging.info(f"ClassModel-DTS Converter v{__version__}, {__copyright__}")

    parser = argparse.ArgumentParser(description='TypeScript Generator')
    parser.add_argument('config', metavar='CONFIG', help='json config file')
    args = parser.parse_args()

    generator = TypeScriptGenerator(pathlib.Path(args.config))
    generator.run()


if __name__ == "__main__":
    main()
