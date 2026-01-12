#!/usr/bin/python
"""PO normalizer script.
"""

import argparse
import io
import logging

import polib

__copyright__ = "Copyright (c) 2025 PreSonus Software Ltd."
__version__ = "1.0.0"


def main() -> None:
	parser = argparse.ArgumentParser(description="Normalize PO")
	parser.add_argument("pofile", type=str, help="po file to normalize")
	parser.add_argument("-s", "--silent", action='store_true', help="Only print warnings and errors")
	args = parser.parse_args()

	logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
	if args.silent == True:
		logging.getLogger().setLevel(logging.WARNING)

	path = args.pofile

	logging.info(f"NormalizePO v{__version__}, {__copyright__}")
	logging.info(f"Normalizing file '{path}'")

	def sortPoLibKey(e: polib.POEntry):
		return e.msgctxt or "", e.msgid, e.comment or ""

	file = polib.pofile(path, wrapwidth=-1)
	file.sort(key=sortPoLibKey)

	contents = file.__unicode__()

	# remove extra empty comment at the beginning of the file
	if contents.startswith("#\n"):
		contents = contents[2:]

	with io.open(path, 'w', encoding=file.encoding, newline='\n') as fhandle:
		if not isinstance(contents, str):
			contents = contents.decode(file.encoding)

		fhandle.write(contents)

if __name__ == '__main__':
	main()
