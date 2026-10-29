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
# Filename    : coremacros.wasm.cmake
# Description : WebAssembly CMake Macros
#
#************************************************************************************************

set (EXTENSIONS_BINARY_SUBDIRECTORY "wasm" CACHE STRING "")

if (DEFINED ENV{EMSDK_PYTHON})
	set (Python3_EXECUTABLE "$ENV{EMSDK_PYTHON}")
endif ()
find_package (Python3 REQUIRED COMPONENTS Interpreter)

ccl_find_file (CCL_EXTRACT_PLUGIN_EXPORTS_SCRIPT NAMES extractpluginexports.py HINTS "${CCL_REPOSITORY_ROOT}/build/wasm")

# Generate an export definition for a target.
# @group wasm
# @param {STRING} target  Name of the target to add resource files to.
# @param args  [variadic] List of functions to export.
macro (ccl_generate_export_file target)
	cmake_parse_arguments (export_params "PRIVATE" "" "" ${ARGN})
	set (exports ${export_params_UNPARSED_ARGUMENTS})
	foreach (exported_function IN ITEMS ${exports})
		if (${target}_exported_functions)
			string (APPEND ${target}_exported_functions ",_${exported_function}")
		else ()
			set (${target}_exported_functions "-sEXPORTED_FUNCTIONS=_${exported_function}")
		endif ()
	endforeach ()
	if (${target}_exported_functions)
		target_link_options (${target} PRIVATE "${${target}_exported_functions}") 
	endif ()
endmacro ()

# Apply WebAssembly-specific compiler and linker flags
# @group wasm
# @param {STRING} target  Name of the target to configure.
macro (ccl_apply_wasm_flags target)
	set (emscripten_flags "${VENDOR_EMSCRIPTEN_FLAGS}")
	separate_arguments (emscripten_flags)
	target_compile_options (${target} PRIVATE ${emscripten_flags})
	set (emscripten_flags "${VENDOR_EMSCRIPTEN_LINKER_FLAGS}")
	separate_arguments (emscripten_flags)
	target_link_options (${target} PRIVATE ${emscripten_flags})
endmacro ()

# Configure a self-contained module
# @group wasm
# @param {STRING} target  Name of the target to configure.
macro (ccl_configure_selfcontained_module target)
	ccl_apply_wasm_flags (${target})
	ccl_get_build_output_directory (buildoutput_directory)
	target_link_directories (${target} PRIVATE "${buildoutput_directory}" "${buildoutput_directory}/Plugins")
endmacro ()

# Configure an application target
# @group wasm
# @param {STRING} target  Name of the target to configure.
macro (ccl_configure_app target)
	ccl_configure_selfcontained_module (${target})
	set (thread_pool_size_flag "")
	if (VENDOR_WASM_USE_PTHREADS)
		set (thread_pool_size "navigator.hardwareConcurrency")
		if (VENDOR_WASM_THREAD_POOL_SIZE)
			set (thread_pool_size "${VENDOR_WASM_THREAD_POOL_SIZE}")
		endif ()
		set (thread_pool_size_flag "-sPTHREAD_POOL_SIZE=${thread_pool_size}")
	endif ()
	target_link_options (${target} PRIVATE -lembind -sMAIN_MODULE=2 ${thread_pool_size_flag} -sEXPORTED_RUNTIME_METHODS=callMain,ccall,cwrap,FS)
	target_link_options (${target} PRIVATE -frtti) # required for embind
	if (CCL_HTML_SHELL_FILE)
		target_link_options (${target} PRIVATE "--shell-file=${CCL_HTML_SHELL_FILE}")
	endif ()
	ccl_find_file (cclInit NAMES cclinit.js HINTS "${CCL_DIR}/platform/wasm/js")
	if (cclInit)
		target_link_options (${target} PRIVATE "--pre-js=${cclInit}")
		target_sources (${target} PRIVATE ${cclInit})
	endif ()
endmacro ()

# Configure a core plug-in library target
# @group wasm
# @param {STRING} target  Name of the target to configure.
macro (ccl_configure_core_plugin target)
	ccl_apply_wasm_flags (${target})
endmacro ()

# Finalize plug-in export handling for a target
# @group wasm
# @param {STRING} target  Name of the target to finalize plug-in exports for.
macro (ccl_finalize_plugin_exports target)
	if (${target}_plugin_so_files)
		unset (satisfied_by_files)
		ccl_collect_shared_link_libraries (${target} satisfied_by_files)
		foreach (plugin_target IN LISTS ${target}_plugin_target_names)
			ccl_collect_shared_link_libraries (${plugin_target} satisfied_by_files)
		endforeach ()

		set (plugin_so_list_file "${CMAKE_BINARY_DIR}/${target}_plugin_exports/_plugins.txt")
		list (JOIN ${target}_plugin_so_files "\n" plugin_so_list_content)
		file (GENERATE OUTPUT "${plugin_so_list_file}" CONTENT "${plugin_so_list_content}\n")

		set (satisfied_by_file "${CMAKE_BINARY_DIR}/${target}_plugin_exports/_satisfiedby.txt")
		list (JOIN satisfied_by_files "\n" satisfied_by_content)
		file (GENERATE OUTPUT "${satisfied_by_file}" CONTENT "${satisfied_by_content}\n")

		set (required_exports_file "${CMAKE_BINARY_DIR}/${target}_plugin_exports/_required_exports.txt")
		add_custom_command (
			OUTPUT "${required_exports_file}"
			COMMAND Python3::Interpreter "${CCL_EXTRACT_PLUGIN_EXPORTS_SCRIPT}" "${EMSCRIPTEN_ROOT_PATH}"
				"${required_exports_file}"
				--plugins "@${plugin_so_list_file}"
				--satisfied-by "@${satisfied_by_file}"
			DEPENDS ${${target}_plugin_so_files} ${satisfied_by_files} "${CCL_EXTRACT_PLUGIN_EXPORTS_SCRIPT}"
				"${plugin_so_list_file}" "${satisfied_by_file}"
			COMMENT "Analyzing plug-in export requirements for ${target}"
		)

		set (baseline_exports_file "${CMAKE_BINARY_DIR}/${target}_plugin_exports/_baseline_exports.txt")
		set (baseline_exports "_main")
		if (VENDOR_EMSCRIPTEN_EXPORTED_FUNCTIONS)
			list (APPEND baseline_exports ${VENDOR_EMSCRIPTEN_EXPORTED_FUNCTIONS})
		endif ()
		string (REPLACE ";" "\n" baseline_exports_content "${baseline_exports}")
		file (WRITE "${baseline_exports_file}" "${baseline_exports_content}\n")

		set (combined_exports_file "${CMAKE_BINARY_DIR}/${target}_plugin_exports/_plugin_exports.txt")
		add_custom_command (
			OUTPUT "${combined_exports_file}"
			COMMAND ${CMAKE_COMMAND} -E cat "${baseline_exports_file}" "${required_exports_file}" > "${combined_exports_file}"
			DEPENDS "${baseline_exports_file}" "${required_exports_file}"
			COMMENT "Combining plug-in export requirements for ${target}"
		)
		target_link_options (${target} PRIVATE "-sEXPORTED_FUNCTIONS=@${combined_exports_file}")
		target_sources (${target} PRIVATE "${combined_exports_file}")
	endif ()
endmacro ()

# Process target dependencies.
# @group wasm
# @param {STRING} target  Name of the target to process dependencies for.
# @param {STRING} ITEMS  [variadic] List of dependency target names.
macro (ccl_process_dependencies target)
	cmake_parse_arguments (dependency_params "" "" "ITEMS" ${ARGN})

	foreach (item IN ITEMS ${dependency_params_ITEMS})
		if (NOT TARGET ${item})
			continue ()
		endif ()

		ccl_check_plugin (${item} is_plugin)
		if (is_plugin)
			target_link_options (${target} PRIVATE --preload-file=$<TARGET_FILE:${item}>@/Plugins/$<TARGET_FILE_NAME:${item}>)

			ccl_list_append_once (${target}_plugin_so_files "$<TARGET_FILE:${item}>")
			ccl_list_append_once (${target}_plugin_target_names "${item}")
			if (NOT ${target}_plugin_exports_deferred)
				set (${target}_plugin_exports_deferred TRUE)
				cmake_language (DEFER DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" CALL ccl_finalize_plugin_exports "${target}")
			endif ()
		endif ()
	endforeach ()
endmacro ()

# Install assets
# @group wasm
# @param {STRING} target  Name of the target to add asset files to.
# @param {PATH} PATH  [optional] Destination subdirectory.
# @param {FILEPATH} FILES  [variadic] Asset files.
# @param {PATH} DIRECTORY  [variadic] Asset directories.
macro (ccl_process_assets target)
	cmake_parse_arguments (asset_params "" "PATH" "DIRECTORY;FILES" ${ARGN})

	if (asset_params_DIRECTORY)
		cmake_path (CONVERT "${asset_params_DIRECTORY}" TO_CMAKE_PATH_LIST assets NORMALIZE)
		foreach (asset IN ITEMS ${assets})
			get_filename_component (asset_dir "${asset}" NAME_WLE)
			target_link_options (${target} PRIVATE "--preload-file=${asset}@/${asset_params_PATH}/${asset_dir}")
		endforeach ()
	else ()
		cmake_path (CONVERT "${asset_params_FILES}" TO_CMAKE_PATH_LIST assets NORMALIZE)
		foreach (asset IN ITEMS ${assets})
			get_filename_component (asset_name "${asset}" NAME)
			target_link_options (${target} PRIVATE "--preload-file=${asset}@/${asset_params_PATH}/${asset_name}")
		endforeach ()
	endif ()
endmacro ()

# Link resource files to a binary.
# @group wasm
# @param {STRING} target  Name of a target to add resources to.
# @param {STRING} path  Subdirectory to place the resources in.
# @param args  Resource files.
macro (ccl_process_resources target config path)
	ccl_link_resources ("${target}" "${config}" "${path}" ${ARGN})
endmacro ()

# Configure the project vendor.
# @group wasm
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Installation directories
	set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "app")
	set (VENDOR_PLUGINS_RUNTIME_DIRECTORY "plugins")
endmacro ()

# Setup platform-specific options to pass to CMake for external projects
# @group wasm
# @param {STRING} target  Name of target to setup platform options for.
macro (ccl_setup_external_project_platform_options target)
	string (APPEND ${target}_cxx_flags " ${VENDOR_EMSCRIPTEN_FLAGS}")
	string (APPEND ${target}_c_flags " ${VENDOR_EMSCRIPTEN_FLAGS}")
	string (APPEND ${target}_cxx_link_flags " ${VENDOR_EMSCRIPTEN_LINKER_FLAGS}")
	string (APPEND ${target}_c_link_flags " ${VENDOR_EMSCRIPTEN_LINKER_FLAGS}")

	string (REPLACE ";" "|" ${target}_find_root_path "${CMAKE_FIND_ROOT_PATH}")
	string (REPLACE ";" "|" ${target}_library_path "${CMAKE_LIBRARY_PATH}")
	string (REPLACE ";" "|" ${target}_program_path "${CMAKE_PROGRAM_PATH}")
	string (REPLACE ";" "|" ${target}_prefix_path "${CMAKE_PREFIX_PATH}")

	string (REPLACE ";" "|" ${target}_module_path "${CMAKE_MODULE_PATH}")

	list (APPEND ${target}_platform_options
		-DCMAKE_CROSSCOMPILING=${CMAKE_CROSSCOMPILING}
		-DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN_ROOT_PATH}/cmake/Modules/Platform/Emscripten.cmake
		-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
		-DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
		-DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
		-DCMAKE_CXX_COMPILER_TARGET=${CMAKE_CXX_COMPILER_TARGET}
		-DCMAKE_ASM_COMPILER_TARGET=${CMAKE_ASM_COMPILER_TARGET}
		-DCMAKE_AR=${CMAKE_AR}
		-DCMAKE_RANLIB=${CMAKE_RANLIB}
		-DCMAKE_C_COMPILER_AR=${CMAKE_C_COMPILER_AR}
		-DCMAKE_C_COMPILER_RANLIB=${CMAKE_C_COMPILER_RANLIB}
		-DCMAKE_CXX_COMPILER_AR=${CMAKE_CXX_COMPILER_AR}
		-DCMAKE_CXX_COMPILER_RANLIB=${CMAKE_CXX_COMPILER_RANLIB}
		-DCMAKE_FIND_ROOT_PATH=${${target}_find_root_path}
		-DCMAKE_SYSROOT=${CMAKE_SYSROOT}
		-DCMAKE_LIBRARY_PATH=${${target}_library_path}
		-DCMAKE_PROGRAM_PATH=${${target}_program_path}
		-DCMAKE_PREFIX_PATH=${${target}_prefix_path}
		-DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
		-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM}
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY}
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=${CMAKE_TRY_COMPILE_TARGET_TYPE}
		-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON
		-DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
		-DCMAKE_SHARED_LIBRARY_CREATE_C_FLAGS=${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS}
		-DCMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS=${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS}
		"-DCMAKE_MODULE_PATH=${target}_module_path"
	)
endmacro ()

# Override install command to include WebAssembly-specific files.
# @group wasm
# @param args  [variadic] See CMake documentation for install command
function (install)
	# Always perform the real install unchanged first.
	_install (${ARGN})

	list (FIND ARGN "TARGETS" _targets_idx)
	list (FIND ARGN "IMPORTED_RUNTIME_ARTIFACTS" _imported_idx)
	if (_targets_idx EQUAL -1 AND _imported_idx EQUAL -1)
		return ()
	endif ()

	set (_section_keywords
		ARCHIVE LIBRARY RUNTIME OBJECTS FRAMEWORK BUNDLE
		PRIVATE_HEADER PUBLIC_HEADER RESOURCE
		INCLUDES EXPORT RUNTIME_DEPENDENCY_SET
	)

	set (_targets "")
	set (_runtime_destination "")
	set (_library_destination "")
	set (_in_targets FALSE)
	set (_current_section "")
	set (_expect_dest FALSE)

	list (LENGTH ARGN _n)
	math (EXPR _last "${_n} - 1")

	foreach (_i RANGE 0 ${_last})
		list (GET ARGN ${_i} _tok)

		if (_tok IN_LIST _section_keywords)
			set (_in_targets FALSE)
			set (_current_section ${_tok})
			set (_expect_dest FALSE)
			continue ()
		endif ()

		if (_tok STREQUAL "TARGETS")
			set (_in_targets TRUE)
			set (_current_section "")
			continue ()
		endif()

		if (_tok STREQUAL "IMPORTED_RUNTIME_ARTIFACTS")
			set (_in_targets TRUE)
			set (_current_section "")
			continue ()
		endif()
		
		if (_in_targets)
			list (APPEND _targets ${_tok})
			continue ()
		endif()

		if (_current_section STREQUAL "RUNTIME" OR _current_section STREQUAL "LIBRARY")
			if (_tok STREQUAL "DESTINATION")
			set (_expect_dest TRUE)
			elseif (_expect_dest)
			if (_current_section STREQUAL "RUNTIME")
				set (_runtime_destination ${_tok})
			else ()
				set (_library_destination ${_tok})
			endif ()
			set (_expect_dest FALSE)
			endif ()
		endif ()
	endforeach ()

	if (_targets STREQUAL "")
		return ()
	endif()

	foreach (_tgt ${_targets})
		if (TARGET ${_tgt})
			get_target_property (_type ${_tgt} TYPE)

			if (_type STREQUAL "EXECUTABLE" AND NOT _runtime_destination STREQUAL "")
				install (FILES
					$<TARGET_FILE_DIR:${_tgt}>/$<TARGET_FILE_BASE_NAME:${_tgt}>.wasm
					$<TARGET_FILE_DIR:${_tgt}>/$<TARGET_FILE_BASE_NAME:${_tgt}>.js
					$<TARGET_FILE_DIR:${_tgt}>/$<TARGET_FILE_BASE_NAME:${_tgt}>.data
					DESTINATION ${_runtime_destination}
					OPTIONAL
				)
				install (FILES
					$<TARGET_FILE_DIR:${_tgt}>/$<TARGET_FILE_BASE_NAME:${_tgt}>.wasm.map
					DESTINATION symbols
					OPTIONAL
				)
			elseif ((_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY") AND NOT _library_destination STREQUAL "")
				install (FILES
					$<TARGET_FILE_DIR:${_tgt}>/$<TARGET_FILE_NAME:${_tgt}>.map
					DESTINATION symbols
					OPTIONAL
				)
			endif ()
		endif ()
	endforeach ()
endfunction ()
