#!/usr/bin/python
"""ClassModel XML to reStructuredText converter."""

import argparse
import logging
from modules.classmodel.converter import ClassModelConverter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"
__version__ = "1.0.0"


def main() -> None:
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
    logging.info(f"ClassModel-RST Converter v{__version__}, {__copyright__}")

    parser = argparse.ArgumentParser(description='ClassModel Converter')
    parser.add_argument('config', metavar='CONFIGURATION', help='configuration file')
    args = parser.parse_args()

    ClassModelConverter.run_config(args.config)


if __name__ == "__main__":
    main()
