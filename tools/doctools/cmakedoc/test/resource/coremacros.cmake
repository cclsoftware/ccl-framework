#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : coremacros.cmake
# Description : Core CMake Macros
#
#************************************************************************************************

include_guard (DIRECTORY)

# This revision number is used by macros to check if cached data is still valid.
# Whenever you move or rename a directory which might be used in any calls to ccl_find_path, increment this number
set (CCL_CMAKE_REVISION_NUMBER 4 CACHE INTERNAL "" FORCE)

set (corelib_MACROS_FILE ${CMAKE_CURRENT_LIST_FILE})

set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
get_filename_component (platformdir ${platformdir} ABSOLUTE)
find_file (corelib_PLATFORMMACROS_FILE NAMES "coremacros.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings.")
mark_as_advanced (corelib_PLATFORMMACROS_FILE)

if (EXISTS "${corelib_PLATFORMMACROS_FILE}")
	include ("${corelib_PLATFORMMACROS_FILE}")
else ()
	set (corelib_PLATFORMMACROS_FILE "")
	message (DEBUG "Could not find a vendor specific config file")
endif ()

# Try to find a platform configuration file for a target.
# @param {STRING} target  Name of a target to find a platform-specific configuration file for.
macro (ccl_find_platform_file target)
	if ("${${target}_PLATFORM_FILE}" STREQUAL "")
		if ("${VENDOR_PLATFORM}" STREQUAL "")
			message (FATAL_ERROR "VENDOR_PLATFORM not set.")
		endif ()
	endif ()
	set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
	set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
	get_filename_component (platformdir ${platformdir} ABSOLUTE)
	message (DEBUG "Searching for ${target}.${VENDOR_PLATFORM}.cmake in ${platformdir};${platformdir2};${CMAKE_CURRENT_LIST_DIR}")
	find_file (${target}_PLATFORM_FILE NAMES "${target}.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings used when compiling ${target}.")
	mark_as_advanced (${target}_PLATFORM_FILE)
endmacro ()

# Include platform specific cmake files for a list of targets.
# Looks for files in the form of `<target>.<platform>.cmake`.
# @param args  [variadic] A list of target names.
macro (ccl_include_platform_specifics)
	set (platform_files "")
	foreach (target IN ITEMS ${ARGN})
		ccl_find_platform_file (${target})	
		if (EXISTS "${${target}_PLATFORM_FILE}")
			list (APPEND platform_files "${${target}_PLATFORM_FILE}")
			list (APPEND "${target}_sources" "${${target}_PLATFORM_FILE}")
			source_group ("cmake" FILES "${${target}_PLATFORM_FILE}")
		else ()
			message (DEBUG "Could not find ${target}.${VENDOR_PLATFORM}.cmake")
		endif ()
		if (EXISTS "${VENDOR_PLATFORMMACROS_FILE}")
			list (APPEND "${target}_sources" "${VENDOR_PLATFORMMACROS_FILE}")
			source_group ("cmake" FILES ${VENDOR_PLATFORMMACROS_FILE})
		endif ()
		list (APPEND "${target}_sources" "${corelib_MACROS_FILE}")
		source_group ("cmake" FILES ${corelib_MACROS_FILE})
		if (EXISTS "${corelib_PLATFORMMACROS_FILE}")
			list (APPEND "${target}_sources" "${corelib_PLATFORMMACROS_FILE}")
			source_group ("cmake" FILES ${corelib_PLATFORMMACROS_FILE})
		endif ()
		if (EXISTS "${VENDOR_MACROS_FILE}")
            list (APPEND "${target}_sources" "${VENDOR_MACROS_FILE}")
			source_group ("cmake" FILES ${VENDOR_MACROS_FILE})
        endif ()
	endforeach ()

	foreach (file IN ITEMS ${platform_files})
		include (${file})
	endforeach ()

	source_group ("CMake Rules" REGULAR_EXPRESSION "^$")
	source_group ("cmake" REGULAR_EXPRESSION "CMakeLists\\.txt|\\.cmake$")
	source_group ("cmake\\rules" REGULAR_EXPRESSION "\\.rule$")
endmacro ()

# Get the default build output directory, optionally appending a subdirectory.
# @param {PATH} result  Output argument, the vendor and platform specific build directory.
# @param {STRING} subdir  [optional] Additional subdirectory, e.g. "Plugins".
macro (ccl_get_build_output_directory result)
	if (${ARGC} GREATER 1)
		set (subdir "/${ARGV1}")
	else ()
		set (subdir "")
	endif ()

	set (subdirectory "${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/$<CONFIG>${subdir}")
	set (${result} "${VENDOR_OUTPUT_DIRECTORY}/${subdirectory}")
endmacro ()

# Get the build output path of a library.
# @param {PATH} result  Output argument, vendor and platform specific library build directory.
# @param {STRING} name  Library name.
# @param {STRING} type  Build type (import, static, shared).
# @param {STRING} subdir  [optional] Additional subdirectory, e.g. "Plugins" or an absolute build output directory.
macro (ccl_get_library_path result name type)
	if (${ARGC} GREATER 3)
		if (IS_ABSOLUTE "${ARGV3}")
			set (library_dir "${ARGV3}")
		else ()
			ccl_get_build_output_directory (library_dir "${ARGV3}")
		endif ()
	else ()
		ccl_get_build_output_directory (library_dir)
	endif ()

	if ("${type}" STREQUAL "STATIC")
		set (suffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
		set (prefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
	elseif ("${type}" STREQUAL "SHARED")
		set (suffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")
		set (prefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
	elseif ("${type}" STREQUAL "MODULE")
		set (suffix "${CMAKE_SHARED_MODULE_SUFFIX}")
		set (prefix "${CMAKE_SHARED_MODULE_PREFIX}")
	elseif ("${type}" STREQUAL "LINK")
		set (suffix "${CMAKE_LINK_LIBRARY_SUFFIX}")
		set (prefix "${CMAKE_LINK_LIBRARY_PREFIX}")
	endif ()

	set (${result} "${library_dir}/${prefix}${name}${suffix}") 
endmacro ()

# Preconfigure a target to match vendor guidelines.
# @param {STRING} target  Name of the target to configure.
macro (ccl_configure_target target)
	get_target_property (target_type ${target} TYPE)

	if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
		# use vendor-defined language standard if available, but allow fallback to C++98
		if (VENDOR_CXX_STANDARD)
			target_compile_features (${target} PUBLIC cxx_std_${VENDOR_CXX_STANDARD})
		endif ()
		set_target_properties (${target} PROPERTIES CXX_STANDARD_REQUIRED OFF)
		set_target_properties (${target} PROPERTIES CXX_EXTENSIONS OFF)

		# use custom folder structure in projects
		set_target_properties (${target} PROPERTIES USE_FOLDERS ON)

		# Add include search paths
		target_include_directories (${target} PRIVATE ${REPOSITORY_ROOT})
		target_include_directories (${target} PRIVATE ${corelib_BASEDIR})
		target_include_directories (${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../source")

		# Define DEBUG for debug targets
		target_compile_definitions (${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

		if (MSVC)
			set_target_properties (${target} PROPERTIES PDB_NAME "$(TargetName)")
			set_target_properties (${target} PROPERTIES COMPILE_PDB_NAME "$(TargetName)")
			target_compile_definitions (${target} PRIVATE UNICODE _UNICODE)
		endif ()

		if (XCODE)
			if (IOS)
				# iOS does not support dylibs or .bundle packages for plug-ins, only frameworks
				if (target_type STREQUAL "SHARED_LIBRARY")
					set_target_properties (${target} PROPERTIES
						FRAMEWORK TRUE
					)
					target_compile_options (${target} PRIVATE "-fno-constant-cfstrings")
				endif ()
			else ()
				if(target_type STREQUAL "MODULE_LIBRARY")
			 		set_target_properties (${target} PROPERTIES 
						BUNDLE TRUE
					)
					target_compile_options (${target} PRIVATE "-fno-constant-cfstrings")
				endif()
			endif ()

			set_target_properties (${target} PROPERTIES
				MACOSX_BUNDLE_COPYRIGHT "${${target}_COPYRIGHT}"
				MACOSX_BUNDLE_BUNDLE_VERSION "${${target}_VERSION_BUILD}"
				MACOSX_BUNDLE_SHORT_VERSION_STRING "${${target}_SHORT_VERSION}"
				MACOSX_FRAMEWORK_BUNDLE_VERSION "${${target}_SHORT_VERSION}"
				MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${${target}_SHORT_VERSION}"

				XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "${CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH}"
				XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "" # do not add anything to the CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS array
				XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "${CMAKE_XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES}"
				XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "${CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS}"
				XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "${CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL}"
				XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "${CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN}"
				XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "${CMAKE_XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN}"
			)

			get_target_property (is_framework ${target} FRAMEWORK) # there is no MACOSX_FRAMEWORK_COPYRIGHT as of cmake 3.26.3 : use workaround with a user defined Xcode variable
			if (is_framework)
				set_target_properties (${target} PROPERTIES
					XCODE_ATTRIBUTE_FRAMEWORK_COPYRIGHT "${${target}_COPYRIGHT}"
				)
			endif ()

		endif ()

		if(PROJECT_VENDOR AND NOT ${target}_VENDOR)
			ccl_set_vendor (${target} ${PROJECT_VENDOR})
		endif ()
	endif ()
	
	# use custom output directory
	if (${ARGC} GREATER 1)
		ccl_get_build_output_directory (CORE_BUILDOUTPUT_DIRECTORY ${ARGV1})
	else ()
		ccl_get_build_output_directory (CORE_BUILDOUTPUT_DIRECTORY)
	endif ()

	if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
		set_target_properties (${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CORE_BUILDOUTPUT_DIRECTORY}")
		set_target_properties (${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CORE_BUILDOUTPUT_DIRECTORY}")
		set_target_properties (${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CORE_BUILDOUTPUT_DIRECTORY}")
		set_target_properties (${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	endif ()

	# add platform implementation files and other platform-specific definitions
	ccl_include_platform_specifics (${target})

	# allow platform implementation file to overwrite the PACKAGE_ID
	if (XCODE)
		set_target_properties (${target} PROPERTIES
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${${target}_PACKAGE_ID}"
		)
	endif ()

	if (COMMAND ccl_process_plugins)
		ccl_process_plugins (${target})
	endif ()
endmacro ()

# Add a library, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
# @param {STRING} type  Target type, e.g. STATIC, MODULE, INTERFACE.
# @param {STRING} SUBDIR  [optional] Subdirectory to place the build artifact in, e.g. "Plugins".
# @param {STRING} VENDOR  [optional] Vendor ID, used to determine version information. See ccl_set_vendor.
# @param {FILEPATH} VERSION_FILE  [optional] Path to a version header file.
# @param {STRING} VERSION_PREFIX  [optional] Prefix used in the version header file, e.g. APP or PLUG.
macro (ccl_add_library target type)
	cmake_parse_arguments (target_params "" "SUBDIR;VENDOR;VERSION_FILE;VERSION_PREFIX" "" ${ARGN})

	if(IOS AND ${type} STREQUAL "MODULE")
		add_library (${target} SHARED)
	else ()
		add_library (${target} ${type})
	endif ()
	
	if (target_params_VENDOR)
		ccl_set_vendor (${target} ${target_params_VENDOR})
	endif ()
	
	if (target_params_VERSION_FILE)
		ccl_read_version (${target} "${target_params_VERSION_FILE}" ${target_params_VERSION_PREFIX})
	endif ()
	
	ccl_configure_target (${target} ${target_params_SUBDIR})
endmacro ()

# Add a library using core, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
# @param {STRING} type  Target type, e.g. STATIC, MODULE, INTERFACE.
# @param {STRING} SUBDIR  [optional] Subdirectory to place the build artifact in, e.g. "Plugins".
# @param {STRING} VENDOR  [optional] Vendor ID, used to determine version information. See ccl_set_vendor.
# @param {FILEPATH} VERSION_FILE  [optional] Path to a version header file.
# @param {STRING} VERSION_PREFIX  [optional] Prefix used in the version header file, e.g. APP or PLUG.
macro (ccl_add_core_library target type)	
	ccl_add_library (${target} ${type} ${ARGN})

	list (APPEND ${target}_sources ${corelib_malloc_sources})
	source_group ("source\\libs" FILES ${corelib_malloc_sources})
	target_link_libraries (${target} PUBLIC corelib)
	target_compile_definitions (${target} PRIVATE __DSP_NO_CCL_FRAMEWORK__)
endmacro ()

# Add a plug-in library, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
macro (ccl_add_core_plugin_library target type)
	ccl_add_core_library (${target} ${type} ${ARGN})
	ccl_export_symbols (${target} CoreGetClassInfoBundle)
endmacro ()

# Add an application, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
# @param {STRING} SUBDIR  [optional] Subdirectory to place the build artifact in, e.g. "Plugins".
# @param {STRING} VENDOR  [optional] Vendor ID, used to determine version information. See ccl_set_vendor.
# @param {FILEPATH} VERSION_FILE  [optional] Path to a version header file.
# @param {STRING} VERSION_PREFIX  [optional] Prefix used in the version header file, e.g. APP or PLUG.
# @param flags  [variadic] Use BUNDLE for applications that contain resources. Use GUI for applications that have an application window, implies BUNDLE. 
macro (ccl_add_app target)
	set (executable_flags "")	

	cmake_parse_arguments (app_params "GUI;BUNDLE" "SUBDIR;VENDOR;VERSION_FILE;VERSION_PREFIX" "" ${ARGN})
	if (app_params_GUI)
		set (app_params_BUNDLE ON)
		if ("${VENDOR_PLATFORM}" STREQUAL "win")
			list (APPEND executable_flags WIN32)
		endif ()
	endif ()
	if (app_params_BUNDLE)
		if (XCODE)
			list (APPEND executable_flags MACOSX_BUNDLE)
		endif ()
	endif ()

	if ("${VENDOR_PLATFORM}" STREQUAL "android")
		add_library (${target} SHARED)
	else ()
		add_executable (${target} ${executable_flags})
	endif ()
	
	if (app_params_VENDOR)
		ccl_set_vendor (${target} ${app_params_VENDOR})
	endif ()
	
	if (app_params_VERSION_FILE)
		ccl_read_version (${target} "${app_params_VERSION_FILE}" ${app_params_VERSION_PREFIX})
	endif ()

	if (XCODE)
		set_target_properties (${target} PROPERTIES
			XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
			XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
		)
		#code signing identity
		if(IOS)
			set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${SIGNING_TEAMID_IOS}")
		else()
			set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${SIGNING_TEAMID_MAC}")
		endif()
	endif ()
	
	ccl_configure_target (${target} ${app_params_SUBDIR})
endmacro ()

# Add an application using core, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
# @param {STRING} SUBDIR  [optional] Subdirectory to place the build artifact in, e.g. "Plugins".
# @param {STRING} VENDOR  [optional] Vendor ID, used to determine version information. See :cmake:`ccl_set_vendor`.
# @param {FILEPATH} VERSION_FILE  [optional] Path to a version header file.
# @param {STRING} VERSION_PREFIX  [optional] Prefix used in the version header file, e.g. APP or PLUG.
# @param flags  [variadic] Use BUNDLE for applications that contain resources. Use GUI for applications that have an application window, implies BUNDLE. 
macro (ccl_add_core_app target)
	ccl_add_app (${target} ${ARGN})
	list (APPEND ${target}_sources ${corelib_malloc_sources})
	target_link_libraries (${target} PUBLIC corelib)
	target_compile_definitions (${target} PRIVATE __DSP_NO_CCL_FRAMEWORK__)
endmacro ()

# Set the product name for a target.
# Depending on the platform, this name might be used to set the name of the resulting binary or the IDE project name.
# @param {STRING} target  Name of the target.
# @param {STRING} name  Product name.
macro (ccl_set_product_name target name)
	set_target_properties (${target} PROPERTIES PROJECT_LABEL "${name}")

	if (NOT "${VENDOR_PLATFORM}" STREQUAL "android")
		set_target_properties (${target} PROPERTIES OUTPUT_NAME "${name}")
	endif ()
endmacro ()

# Set the target vendor.
# The vendor name is used to find signing certificates as well as copyright information.
# @param {STRING} target  Name of the target.
# @param {STRING} vendor Vendor name.
macro (ccl_set_vendor target vendor)
	set (${target}_VENDOR "${vendor}")

	set (vendor_id ${vendor})
	include (${REPOSITORY_IDENTITIES_DIR}/${vendor}/identity.cmake)
	include (${REPOSITORY_IDENTITIES_DIR}/${vendor}/signing.cmake)
	
	# set target-specific vendor information
	foreach (var VENDOR_NAME VENDOR_COPYRIGHT_YEAR VENDOR_COPYRIGHT VENDOR_WEBSITE VENDOR_MAIL VENDOR_PACKAGE_DOMAIN VENDOR_MIME_TYPE)
		set (${target}_${var} "${${var}}")
	endforeach ()
	
	# generate a vendor.h header file
	set (vendor_include_directory "${CMAKE_CURRENT_BINARY_DIR}/vendor/${vendor}")
	file (MAKE_DIRECTORY "${vendor_include_directory}")
	configure_file ("${REPOSITORY_IDENTITIES_DIR}/shared/vendor.h.in" "${vendor_include_directory}/vendor.h.new")
	file (COPY_FILE "${vendor_include_directory}/vendor.h.new" "${vendor_include_directory}/vendor.h" ONLY_IF_DIFFERENT)
	target_include_directories (${target} BEFORE PRIVATE "${vendor_include_directory}")
	
	# apply platform-specific configuration
	if (COMMAND ccl_configure_vendor)
		ccl_configure_vendor (${target} ${vendor})
	endif ()
endmacro ()

# Set the project vendor.
# The project vendor name is used to set installation directories, the code signing identity and other properties that apply to all targets in the project.
# Most applications won't call this macro directly. Instead, set the variable `PROJECT_VENDOR` to your vendor identity right after the first `project` call.
# @param {STRING} vendor Vendor name.
macro (ccl_set_project_vendor vendor)
	set (PROJECT_VENDOR "${vendor}")

	set (vendor_id ${vendor})
	include (${REPOSITORY_IDENTITIES_DIR}/${vendor}/identity.cmake)
	include (${REPOSITORY_IDENTITIES_DIR}/${vendor}/signing.cmake)
	
	foreach (var VENDOR_NAME VENDOR_COPYRIGHT_YEAR VENDOR_COPYRIGHT VENDOR_WEBSITE VENDOR_MAIL VENDOR_PACKAGE_DOMAIN VENDOR_MIME_TYPE VENDOR_INSTALL_SUBDIR VENDOR_PUBLISHER)
		set (PROJECT_${var} "${${var}}")
	endforeach ()
	
	# generate a vendor.h header file
	set (vendor_include_directory "${CMAKE_CURRENT_BINARY_DIR}/vendor/${vendor}")
	file (MAKE_DIRECTORY "${vendor_include_directory}")
	configure_file ("${REPOSITORY_IDENTITIES_DIR}/shared/vendor.h.in" "${vendor_include_directory}/vendor.h.new")
	file (COPY_FILE "${vendor_include_directory}/vendor.h.new" "${vendor_include_directory}/vendor.h" ONLY_IF_DIFFERENT)
	list (PREPEND VENDOR_INCLUDE_DIRS "${vendor_include_directory}")
	
	# apply platform-specific configuration
	if (COMMAND ccl_configure_project_vendor)
		ccl_configure_project_vendor (${vendor})
	endif ()
endmacro ()


# Add source files for an optional feature to a target.
# @param {STRING} target  Name of the target to add source files to.
# @param {STRING} optionname  Name of an option to check.
# @param {STRING} description  Description of the optional feature.
# @param {BOOL} default  Default value of the option.
# @param {STRING} sources  Name of a variable that contains the source files of the feature.
macro (ccl_add_optional target optionname description default sources)
	option (${optionname} ${description} ${default})
	if (${${optionname}})
		list (APPEND ${target}_sources ${${sources}})
	endif ()
endmacro ()

# Add resource files to a target.
# All resource files are placed in a flat resource directory.
# You may place all resource files in a subdirectory using an optional `PATH <subdirectory>` argument.
# @param {STRING} target  Name of the target to add resource files to.
# @param args  [variadic] Resource files.
macro (ccl_add_resources target)
	cmake_parse_arguments (resource_params "" "CONFIG;PATH" "" ${ARGN})
	if(NOT resource_params_CONFIG)
		set (resource_params_CONFIG "")
	endif ()
	if(NOT resource_params_PATH)
		set (resource_params_PATH "")
	endif ()
	set (resources ${resource_params_UNPARSED_ARGUMENTS})
	if ("${resource_params_CONFIG}" STREQUAL "" OR APPLE OR ANDROID)
		list (APPEND ${target}_sources ${resources})
	else ()
		list (APPEND ${target}_sources $<$<CONFIG:${resource_params_CONFIG}>:${resources}>)	
	endif ()
	source_group ("resource/${resource_params_PATH}" FILES ${resources})
	if (COMMAND ccl_process_resources)
		ccl_process_resources ("${target}" "${resource_params_CONFIG}" "${resource_params_PATH}" ${resources})
	endif ()
endmacro ()

# Generate an export definition for a target.
# @param {STRING} target  Name of the target to add resource files to.
# @param args  [variadic] List of functions to export.
macro (ccl_export_symbols target)
	if (COMMAND ccl_generate_export_file)
		ccl_generate_export_file ("${target}" ${ARGN})
	endif ()
endmacro ()

# Add dependencies to a target.
# Unlike CMake's add_dependencies, this macro skips non-existing targets silently.
# @param {STRING} target  Name of the target to add dependencies to.
# @param args  [variadic] Dependencies.
macro (ccl_add_dependencies target)
	set (items "${ARGN}")
	foreach (item IN LISTS items)
		if (TARGET "${item}")
			add_dependencies (${target} "${item}")
		endif ()
	endforeach ()
endmacro ()

# Find a directory containing a named file.
# This command is equivalent to the built-in find_path CMake provides.
# The only exception is that it checks for a revision number to make sure that CMake 
# updates paths after a developer moved or renamed a previously cached directoy.
# Whenever you move or rename a directory which might be used in any calls to ccl_find_path, 
# update the revision number CCL_CMAKE_REVISION_NUMBER at the top of this file
macro (ccl_find_path var)
	if (CCL_CMAKE_REVISION_NUMBER GREATER ${var}_CMAKE_REVISION_NUMBER)
		unset (${var} CACHE)
		unset (${var})
	endif ()
	find_path (${var} ${ARGN})
	set (${var}_CMAKE_REVISION_NUMBER ${CCL_CMAKE_REVISION_NUMBER} CACHE INTERNAL "" FORCE)
endmacro ()

# Preprocess a header file and read the values of peprocessor definitions.
# @param {FILEPATH} file  Source file with preprocessor definitions.
# @param args  [variadic] List of pairs <var> <macro>, where <var> is the name of an output variable and <macro> is a macro to evaluate and store into <var>.
macro (ccl_evaluate_macros file)
	cmake_parse_arguments (evaluation_params "" "" "INCLUDE_DIRECTORIES;MACROS" ${ARGN})
	list (APPEND evaluation_params_INCLUDE_DIRECTORIES ${VENDOR_INCLUDE_DIRS})
	list (APPEND evaluation_params_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/vendor/ccl")

	set (evaluation_code "
		#include \"${file}\"
		#define STRINGIFY(s) STRINGIFY_HELPER(s)
		#define STRINGIFY_HELPER(s) #s
	")

	# loop over variables and macros and prepare a source file to compile
	set (items "${evaluation_params_MACROS};${evaluation_params_UNPARSED_ARGUMENTS}")
	set (var "")
	set (macro "")
	foreach (item IN LISTS items)
		if ("${item}" STREQUAL "")
			continue ()
		endif ()
		if ("${var}" STREQUAL "")
			set (var "${item}")
			continue ()
		endif ()
		set (macro "${item}")

		string (APPEND evaluation_code "
			#ifndef ${macro}
			#define ${macro} \"\"
			#endif
			#pragma message (\"${macro} %%\" STRINGIFY (${macro}) \"%%\")
		")
		
		set (var "")
	endforeach ()
	
	string (APPEND evaluation_code "
		int main () { return 0; }
	")
	
	# compile the source file and save the output in a variable
	file (WRITE "${CMAKE_BINARY_DIR}/evaluate.cpp" "${evaluation_code}")
	try_compile (
		compile_result_unused
		"${CMAKE_BINARY_DIR}"
		SOURCES "${CMAKE_BINARY_DIR}/evaluate.cpp"
		OUTPUT_VARIABLE evaluatedstring
		CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${evaluation_params_INCLUDE_DIRECTORIES}"
	)
	
	# loop over variables and macros again and save the evaluated macros to the according variables
	set (var "")
	set (macro "")
	
	set (any_nonempty OFF)
	foreach (item IN LISTS items)
		if ("${var}" STREQUAL "")
			set (var "${item}")
			continue ()
		endif ()
		set (macro "${item}")
		set (${var} "")

		string (REGEX MATCH " ${macro} %%([^%]+)%%" evaluated_macro "${evaluatedstring}")
	
		if (NOT "${CMAKE_MATCH_1}" STREQUAL "")
			set (any_nonempty ON)
		endif ()

		separate_arguments (macro_tokens UNIX_COMMAND "${CMAKE_MATCH_1}")
		string (CONCAT ${var} ${macro_tokens})

		set (var "")
	endforeach ()

	if (NOT any_nonempty)
		message (NOTICE "Include directories: ${evaluation_params_INCLUDE_DIRECTORIES}")
		message (NOTICE "Evaluation output:\n${evaluatedstring}")
		message (FATAL_ERROR "Macro evaluation failed!")
	endif ()
endmacro ()

# Preprocess a version header file and read version information.
# Defines the following variables:
#
# * <target>_VERSION
# * <target>_VERSION_MAJOR
# * <target>_VERSION_MINOR
# * <target>_VERSION_REVISION
# * <target>_VERSION_BUILD
# * <target>_NAME
# * <target>_COMPANY_NAME
# * <target>_COPYRIGHT
# * <target>_WEBSITE
# * <target>_PACKAGE_DOMAIN
# * <target>_PACKAGE_ID
# * <target>_MIME_TYPE
#
# @param {STRING} target  Prefix of the result variables.
# @param {FILEPATH} versionfile  Version header file of the target.
# @param {STRING} prefix  [optional] Prefix of preprocessor definitions in the header file, e.g. APP.
# @param {STRING} fallback  [optional] Fallback prefix for fields that are not defined in the version header, e.g. CCL.
macro (ccl_read_version target versionfile)
	if (${ARGC} GREATER 2)
		set (prefix ${ARGV2})
	else ()
		set (prefix "")
	endif ()
	if (${ARGC} GREATER 3)
		set (fallback ${ARGV3})
	else ()
		set (fallback "CCL")
	endif ()

	# Check the timestamp of the version header first and skip the try_compile call if the file did not change.

	file (TIMESTAMP "${versionfile}" target_timestamp)
	file (TIMESTAMP "${corelib_DIR}/public/coreversion.h" core_timestamp)

	if (NOT ${target}_VERSION OR NOT ${target}_SHORT_VERSION OR NOT ${target}_ID
		OR NOT target_timestamp OR NOT "${target_timestamp}" STREQUAL "${VERSION_FILE_${target}_TIMESTAMP}"
		OR NOT core_timestamp OR NOT "${core_timestamp}" STREQUAL "${VERSION_FILE_CORE_TIMESTAMP}"
	)

		set (VERSION_FILE_${target}_TIMESTAMP "${target_timestamp}" CACHE STRING "" FORCE)
		set (VERSION_FILE_CORE_TIMESTAMP "${core_timestamp}" CACHE STRING "" FORCE)
		
		if(TARGET ${target})
			get_target_property (target_INCLUDE_DIRS ${target} INCLUDE_DIRECTORIES)
		else ()
			set (target_INCLUDE_DIRS "")
		endif ()
		ccl_evaluate_macros ("${versionfile}"
		
			MACROS
			ver_major "${prefix}_VERSION_MAJOR"
			ver_major2 "${prefix}_VER_MAJOR"
			ver_major3 "VERSION_MAJOR"
			ver_major4 "VER_MAJOR"

			ver_minor "${prefix}_VERSION_MINOR"
			ver_minor2 "${prefix}_VER_MINOR"
			ver_minor3 "VERSION_MINOR"
			ver_minor4 "VER_MINOR"

			ver_revision "${prefix}_VERSION_REVISION"
			ver_revision2 "${prefix}_VER_REVISION"
			ver_revision3 "VERSION_REVISION"
			ver_revision4 "VER_REVISION"

			ver_build "${prefix}_VERSION_BUILD"
			ver_build2 "${prefix}_VER_BUILD"
			ver_build3 "VERSION_BUILD"
			ver_build4 "VER_BUILD"

			ver_version "${prefix}_VERSION"
			ver_versionstring "${prefix}_VERSION_STRING"
			
			ver_shortversion "${prefix}_SHORT_VERSION"

			ver_name "${prefix}_NAME"
			ver_productname "${prefix}_PRODUCT_NAME"

			ver_company "${prefix}_COMPANY"
			ver_companyname "${prefix}_COMPANY_NAME"
			ver_authorname "${prefix}_AUTHOR_NAME"
			ver_vendor "${prefix}_VENDOR"

			ver_copyright "${prefix}_COPYRIGHT"
			ver_authorcopyright "${prefix}_AUTHOR_COPYRIGHT"

			ver_website "${prefix}_WEBSITE"
			ver_vendorurl "${prefix}_VENDOR_URL"

			ver_packagedomain "${prefix}_PACKAGE_DOMAIN"

			ver_packageid "${prefix}_PACKAGE_ID"
			
			ver_id "${prefix}_ID"

			ver_filedescription "${prefix}_FILE_DESCRIPTION"
			
			ver_mimetype "${prefix}_MIME_TYPE"
			
			INCLUDE_DIRECTORIES 
			${target_INCLUDE_DIRS}
		)

		if ("${ver_major}" STREQUAL "")
			set (ver_major "${ver_major2}")
		endif ()
		if ("${ver_major}" STREQUAL "")
			set (ver_major "${ver_major3}")
		endif ()
		if ("${ver_major}" STREQUAL "")
			set (ver_major "${ver_major4}")
		endif ()
		if ("${ver_major}" STREQUAL "")
			set (ver_major "${${fallback}_VERSION_MAJOR}")
		endif ()

		if ("${ver_minor}" STREQUAL "")
			set (ver_minor "${ver_minor2}")
		endif ()
		if ("${ver_minor}" STREQUAL "")
			set (ver_minor "${ver_minor3}")
		endif ()
		if ("${ver_minor}" STREQUAL "")
			set (ver_minor "${ver_minor4}")
		endif ()
		if ("${ver_minor}" STREQUAL "")
			set (ver_minor "${${fallback}_VERSION_MINOR}")
		endif ()

		if ("${ver_revision}" STREQUAL "")
			set (ver_revision "${ver_revision2}")
		endif ()
		if ("${ver_revision}" STREQUAL "")
			set (ver_revision "${ver_revision3}")
		endif ()
		if ("${ver_revision}" STREQUAL "")
			set (ver_revision "${ver_revision4}")
		endif ()
		if ("${ver_revision}" STREQUAL "")
			set (ver_revision "${${fallback}_VERSION_REVISION}")
		endif ()

		if ("${ver_build}" STREQUAL "")
			set (ver_build "${ver_build2}")
		endif ()
		if ("${ver_build}" STREQUAL "")
			set (ver_build "${ver_build3}")
		endif ()
		if ("${ver_build}" STREQUAL "")
			set (ver_build "${ver_build4}")
		endif ()
		if ("${ver_build}" STREQUAL "")
			set (ver_build "${${fallback}_VERSION_BUILD}")
		endif ()
		
		set (${target}_VERSION_MAJOR "${ver_major}")
		set (${target}_VERSION_MINOR "${ver_minor}")
		set (${target}_VERSION_REVISION "${ver_revision}")
		set (${target}_VERSION_BUILD "${ver_build}")
		set (${target}_VERSION_MAJOR "${ver_major}" CACHE STRING "" FORCE)
		set (${target}_VERSION_MINOR "${ver_minor}" CACHE STRING "" FORCE)
		set (${target}_VERSION_REVISION "${ver_revision}" CACHE STRING "" FORCE)
		set (${target}_VERSION_BUILD "${ver_build}" CACHE STRING "" FORCE)
		
		set (${target}_SHORT_VERSION "${ver_shortversion}")
		if ("${${target}_SHORT_VERSION}" STREQUAL "")
			set (${target}_SHORT_VERSION "${ver_major}.${ver_minor}.${ver_revision}")
		endif ()
		set (${target}_SHORT_VERSION "${${target}_SHORT_VERSION}" CACHE STRING "" FORCE)
		
		set (${target}_VERSION "${ver_version}")
		if ("${${target}_VERSION}" STREQUAL "")
			set (${target}_VERSION "${ver_versionstring}")
		endif ()
		if ("${${target}_VERSION}" STREQUAL "")
			set (${target}_VERSION "${ver_major}.${ver_minor}.${ver_revision}")
		endif ()
		set (${target}_VERSION "${${target}_VERSION}" CACHE STRING "" FORCE)

		set (${target}_NAME "${ver_name}")
		if("${${target}_NAME}" STREQUAL "")
			set (${target}_NAME "${ver_productname}")
		endif ()
		if("${${target}_NAME}" STREQUAL "")
			set (${target}_NAME "${${prefix}_NAME}")
		endif ()
		if("${${target}_NAME}" STREQUAL "")
			set (${target}_NAME "${${fallback}_NAME}")
		endif ()
		set (${target}_NAME "${${target}_NAME}" CACHE STRING "" FORCE)

		set (${target}_COMPANY_NAME "${ver_company}")
		if("${${target}_COMPANY_NAME}" STREQUAL "")
			set (${target}_COMPANY_NAME "${ver_companyname}")
		endif ()
		if("${${target}_COMPANY_NAME}" STREQUAL "")
			set (${target}_COMPANY_NAME "${ver_authorname}")
		endif ()
		if("${${target}_COMPANY_NAME}" STREQUAL "")
			set (${target}_COMPANY_NAME "${ver_vendor}")
		endif ()
		if("${${target}_COMPANY_NAME}" STREQUAL "")
			set (${target}_COMPANY_NAME "${${fallback}_COMPANY_NAME}")
		endif ()
		set (${target}_COMPANY_NAME "${${target}_COMPANY_NAME}" CACHE STRING "" FORCE)

		set (${target}_COPYRIGHT "${ver_copyright}")
		if("${${target}_COPYRIGHT}" STREQUAL "")
			set (${target}_COPYRIGHT "${ver_authorcopyright}")
		endif ()
		if("${${target}_COPYRIGHT}" STREQUAL "")
			set (${target}_COPYRIGHT "${${fallback}_COPYRIGHT}")
		endif ()
		set (${target}_COPYRIGHT "${${target}_COPYRIGHT}" CACHE STRING "" FORCE)

		set (${target}_WEBSITE "${ver_website}")
		if("${${target}_WEBSITE}" STREQUAL "")
			set (${target}_WEBSITE "${ver_vendorurl}")
		endif ()
		set (${target}_WEBSITE "${${target}_WEBSITE}" CACHE STRING "" FORCE)

		if ("${ver_packagedomain}" STREQUAL "")
			set (${target}_PACKAGE_DOMAIN "${VENDOR_PACKAGE_DOMAIN}.")
		else ()
			set (${target}_PACKAGE_DOMAIN "${ver_packagedomain}.")
		endif ()
		set (${target}_PACKAGE_DOMAIN "${${target}_PACKAGE_DOMAIN}" CACHE STRING "" FORCE)

		if ("${ver_packageid}" STREQUAL "")
			set (${target}_PACKAGE_ID "${${target}_PACKAGE_DOMAIN}${target}")
		else ()
			set (${target}_PACKAGE_ID "${ver_packageid}")
		endif ()
		set (${target}_PACKAGE_ID "${${target}_PACKAGE_ID}" CACHE STRING "" FORCE)
		
		if ("${ver_id}" STREQUAL "")
			set (${target}_ID "${target}")
		else ()
			set (${target}_ID "${ver_id}")
		endif ()
		set (${target}_ID "${${target}_ID}" CACHE STRING "" FORCE)

		if ("${ver_filedescription}" STREQUAL "")
			set (${target}_FILE_DESCRIPTION "${${target}_NAME}")
		else ()
			set (${target}_FILE_DESCRIPTION "${ver_filedescription}")
		endif ()
		set (${target}_FILE_DESCRIPTION "${${target}_FILE_DESCRIPTION}" CACHE STRING "" FORCE)

		set (${target}_MIME_TYPE "${ver_mimetype}")
		set (${target}_MIME_TYPE "${${target}_MIME_TYPE}" CACHE STRING "" FORCE)
		
		mark_as_advanced (
			${target}_VERSION ${target}_VERSION_MAJOR ${target}_VERSION_MINOR ${target}_VERSION_REVISION ${target}_VERSION_BUILD ${target}_SHORT_VERSION
			${target}_NAME ${target}_COMPANY_NAME ${target}_COPYRIGHT ${target}_WEBSITE ${target}_PACKAGE_DOMAIN ${target}_PACKAGE_ID ${target}_ID
			${target}_FILE_DESCRIPTION ${target}_MIME_TYPE
		)

	endif ()
endmacro ()

# Link a library as whole archive, i.e. don't remove any symbols.
# @param {STRING} target  Name of the target to link libraries to.
# @param args  [variadic] Libraries to link to <target>.
macro (ccl_link_whole_archive target)
	set (archives ${ARGN})

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND APPLE)
		foreach (archive ${archives})
			target_link_options (${target} PRIVATE -Wl,-force_load,$<TARGET_FILE:${archive}>)
		endforeach ()
	elseif (MSVC)
		foreach (archive ${archives})
			target_link_options (${target} PRIVATE -WHOLEARCHIVE:$<TARGET_FILE:${archive}>)
		endforeach ()
	else ()
		# Assume everything else is like gcc
		target_link_options (${target} PRIVATE -Wl,--whole-archive)
		foreach (archive ${archives})
			target_link_options (${target} PRIVATE $<TARGET_FILE:${archive}>)
		endforeach ()
		target_link_options (${target} PRIVATE -Wl,--no-whole-archive)
	endif ()
endmacro ()

# Set the default target, i.e. the startup project for the current solution / workspace.
# @param {STRING} project  Name of the target that should be the startup project.
macro (ccl_set_startup_project project)
	get_property (startup_project_set DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SET)
	if (NOT startup_project_set)
		set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${project})
	endif ()
endmacro ()

# Convert a file to a byte array and write it to a cpp file.
# Defines two variables <PREFIX>_Ptr and <PREFIX>_Size in the resulting cpp file. 
# @param {FILEPATH} INPUT  Path to the input file.
# @param {FILEPATH} OUTPUT  [optional] Path to the resulting cpp file.
# @param {FILEPATH} SCRIPT_FILE  [optional] Path to a temporary script file.
# @param {STRING} PREFIX  Variable name prefix used in the generated cpp file.
# @param {BOOL} BYTE_ARRAY  Use a byte array instead of a string literal. Using a string literal can speed up linking, but some compilers might have a size limit for string literals.
macro (ccl_makebin)
	cmake_parse_arguments (makebin_params "BYTE_ARRAY" "INPUT;OUTPUT;SCRIPT_FILE;PREFIX" "" ${ARGN})

	if (NOT makebin_params_OUTPUT)
		set (makebin_params_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bincpp/${makebin_params_PREFIX}.cpp)
	endif ()
	
	if (NOT makebin_params_SCRIPT_FILE)
		set (makebin_params_SCRIPT_FILE ${makebin_params_OUTPUT}.cmake)
	endif ()
	
	if (makebin_params_BYTE_ARRAY)
		set (use_byte_array ON)
	else ()
		set (use_byte_array OFF)
	endif ()

	file (WRITE "${makebin_params_SCRIPT_FILE}"
		"file (READ \"${makebin_params_INPUT}\" filedata HEX)
		file (SIZE \"${makebin_params_INPUT}\" filesize)
		# Use a byte array if a string would exceed MSVCs limits, see https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026
		if (${use_byte_array} OR (WIN32 AND filesize GREATER 16380))
			string (REGEX REPLACE \"([0-9a-f][0-9a-f])\" \"0x\\\\1,\" filedata \${filedata})
			file (WRITE \"${makebin_params_OUTPUT}\" \"
				static const unsigned char ${makebin_params_PREFIX}_Data[] = {\${filedata}};
				unsigned int ${makebin_params_PREFIX}_Size = sizeof(${makebin_params_PREFIX}_Data);
				void* ${makebin_params_PREFIX}_Ptr = (void*)${makebin_params_PREFIX}_Data;
			\")
		else ()
			string (REGEX REPLACE \"([0-9a-f][0-9a-f])\" \"\\\\\\\\x\\\\1\" filedata \${filedata})
			file (WRITE \"${makebin_params_OUTPUT}\" \"
				static const unsigned char ${makebin_params_PREFIX}_Data[] = \\\"\${filedata}\\\";
				unsigned int ${makebin_params_PREFIX}_Size = sizeof(${makebin_params_PREFIX}_Data) - 1;
				void* ${makebin_params_PREFIX}_Ptr = (void*)${makebin_params_PREFIX}_Data;
			\")
		endif ()"
	)
	add_custom_command (
		COMMAND ${CMAKE_COMMAND} -P "${makebin_params_SCRIPT_FILE}"
		OUTPUT "${makebin_params_OUTPUT}"
		DEPENDS "${makebin_params_INPUT}"
	)
endmacro ()

# Convert a file to a byte array and write it to a cpp file.
# Defines two exported functions <PREFIX>Data and <PREFIX>Size in the resulting cpp file. 
# @param {FILEPATH} INPUT  Path to the input file.
# @param {FILEPATH} OUTPUT  [optional] Path to the resulting cpp file.
# @param {FILEPATH} SCRIPT_FILE  [optional] Path to a temporary script file.
# @param {STRING} PREFIX  Function name prefix used in the generated cpp file.
# @param {STRING} VARIABLE_NAME  Variable name used in the generated cpp file.
macro (ccl_makebin_export)
	cmake_parse_arguments (makebin_params "" "INPUT;OUTPUT;SCRIPT_FILE;PREFIX;VARIABLE_NAME" "" ${ARGN})

	if (NOT makebin_params_OUTPUT)
		set (makebin_params_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bincpp/${makebin_params_PREFIX}.cpp)
	endif ()
	
	if (NOT makebin_params_SCRIPT_FILE)
		set (makebin_params_SCRIPT_FILE ${makebin_params_OUTPUT}.cmake)
	endif ()
	
	file (WRITE "${makebin_params_SCRIPT_FILE}"
		"file (READ \"${makebin_params_INPUT}\" filedata HEX)
		string (REGEX REPLACE \"([0-9a-f][0-9a-f])\" \"\\\\\\\\x\\\\1\" filedata \${filedata})
		file (WRITE \"${makebin_params_OUTPUT}\" \"
			#include \\\"core/public/coreplatform.h\\\"
			static unsigned char ${makebin_params_VARIABLE_NAME}[] = \\\"\${filedata}\\\";
			CORE_EXPORT unsigned char* ${makebin_params_PREFIX}Data () { return ${makebin_params_VARIABLE_NAME}; }
			CORE_EXPORT unsigned int ${makebin_params_PREFIX}Size () { return sizeof(${makebin_params_VARIABLE_NAME}) - 1; }
		\")"
	)
	add_custom_command (
		COMMAND ${CMAKE_COMMAND} -P "${makebin_params_SCRIPT_FILE}"
		OUTPUT "${makebin_params_OUTPUT}"
		DEPENDS "${makebin_params_INPUT}"
	)
endmacro ()

# Set default debug arguments.
# @param {STRING} target  Name of the target to set debug arguments for.
# @param args  [variadic] Debug arguments.
macro (ccl_set_debug_arguments target)
	string (REPLACE ";" " " arg_string "${ARGN}")
	set_target_properties (${target} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "${arg_string}")
endmacro ()

# Print all properties cmake supports.
function (ccl_print_properties)
	# Get all propreties that cmake supports
	execute_process (COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

	# Convert command output into a CMake list
	string (REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
	string (REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

	message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction ()

# Print all properties that are defined for a target.
# @param {STRING} target  Name of the target to print the properties for.
function (ccl_print_target_properties target)
	if(NOT TARGET ${target})
		message (WARNING "There is no target named '${target}'")
		return()
	endif()

	foreach (prop ${CMAKE_PROPERTY_LIST})
		string (REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
		# Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
		if (prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
			continue ()
		endif ()
		# message ("Checking ${prop}")
		get_property (propval TARGET ${target} PROPERTY ${prop} SET)
		if (propval)
			get_target_property (propval ${target} ${prop})
			message (STATUS "${target} ${prop} = ${propval}")
		endif ()
	endforeach ()
endfunction ()

# Print all variables.
function (ccl_print_variables)
	get_cmake_property (_variableNames VARIABLES)
	list (SORT _variableNames)
	foreach (_variableName ${_variableNames})
		message (STATUS "${_variableName}=${${_variableName}}")
	endforeach ()
endfunction ()
