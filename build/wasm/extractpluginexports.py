#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2026 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
#
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : extractpluginexports.py
# Description : Compute what a wasm app executable (MAIN_MODULE=2) must export/include so that its
#               plug-ins (SIDE_MODULE, loaded later via dlopen()) can resolve their imports -
#               without statically linking the plug-ins' binaries into the executable.
#
# Usage: extractpluginexports.py <emscripten_root> <exported_functions_out>
#            --plugins <plugin1.so> [<plugin2.so> ...]
#            --satisfied-by <other1.so> [<other2.so> ...]
#
# <plugins> are the SIDE_MODULEs this app dlopen()s that are not otherwise linked at all.
# <satisfied-by> are side modules the app DOES already link normally (e.g. cclsystem, cclgui) -
# their own exports satisfy plug-in imports directly at runtime (module-to-module, via the
# dynamic linker) and must not be force-demanded from the executable itself.
#
# Any argument may instead be given as "@<file>", which is replaced by that file's lines
#
#************************************************************************************************

import sys


def expand_response_files(args):
	expanded = []
	for arg in args:
		if arg.startswith('@'):
			with open(arg[1:]) as response_file:
				expanded.extend(line.strip() for line in response_file if line.strip())
		else:
			expanded.append(arg)
	return expanded


def read_symbol_kinds(webassembly, dylib_path):
	imports = webassembly.get_imports(dylib_path)
	names = {
		imported.field for imported in imports
		if imported.kind in (webassembly.ExternType.FUNC, webassembly.ExternType.GLOBAL, webassembly.ExternType.TAG)
	}
	# invoke_* thunks are synthesized by the dynamic linker itself at runtime; never export these.
	names = {name for name in names if not name.startswith('invoke_')}
	weak_imports = set(webassembly.get_weak_imports(dylib_path))
	return names, weak_imports


WASM_MAGIC = b'\0asm'


def is_wasm_file(path):
	try:
		with open(path, 'rb') as dylib_file:
			return dylib_file.read(len(WASM_MAGIC)) == WASM_MAGIC
	except OSError:
		return False


def read_exports(webassembly, dylib_path):
	if not is_wasm_file(dylib_path):
		return set()
	exports = {exported.name for exported in webassembly.get_exports(dylib_path)}
	return {name.removeprefix('__em_js__') for name in exports}


def main():
	emscripten_root = sys.argv[1]
	exported_functions_out = sys.argv[2]
	rest = expand_response_files(sys.argv[3:])

	plugins = []
	satisfied_by = []
	current = None
	for arg in rest:
		if arg == '--plugins':
			current = plugins
		elif arg == '--satisfied-by':
			current = satisfied_by
		else:
			current.append(arg)

	sys.path.insert(0, emscripten_root)
	from tools import webassembly

	strong_imports = set()
	for plugin in plugins:
		names, weak = read_symbol_kinds(webassembly, plugin)
		strong_imports |= (names - weak)

	already_satisfied = set()
	for dylib in satisfied_by:
		already_satisfied |= read_exports(webassembly, dylib)
	# Other plug-ins can satisfy each other's imports too (including their own imports of their own
	# exports, a common PIC pattern for taking the address of e.g. a function-local static).
	for plugin in plugins:
		already_satisfied |= read_exports(webassembly, plugin)

	# Only symbols that nothing else already provides must be forced into (and exported from) the
	# executable itself - the rest resolve module-to-module via the dynamic linker at dlopen() time.
	must_export = sorted(strong_imports - already_satisfied)

	with open(exported_functions_out, 'w', newline='\n') as out:
		# '_' prefix matches Emscripten's EXPORTED_FUNCTIONS naming convention (tools/shared.py:asmjs_mangle).
		out.writelines(f'_{name}\n' for name in must_export)


if __name__ == '__main__':
	main()
