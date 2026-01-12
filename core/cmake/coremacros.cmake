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
set (CCL_CMAKE_REVISION_NUMBER 28 CACHE INTERNAL "" FORCE)

set (corelib_MACROS_FILE ${CMAKE_CURRENT_LIST_FILE})

# Find a directory containing a named file.
# This command is equivalent to the built-in find_path CMake provides.
# The only exception is that it checks for a revision number to make sure that CMake 
# updates paths after a developer moved or renamed a previously cached directoy.
# Whenever you move or rename a directory which might be used in any calls to ccl_find_path, 
# update the revision number CCL_CMAKE_REVISION_NUMBER at the top of this file
macro (ccl_find_path var)
	if (NOT ${var}_CMAKE_REVISION_NUMBER OR CCL_CMAKE_REVISION_NUMBER GREATER ${var}_CMAKE_REVISION_NUMBER)
		unset (${var} CACHE)
		unset (${var})
	endif ()
	find_path (${var} ${ARGN})
	set (${var}_CMAKE_REVISION_NUMBER ${CCL_CMAKE_REVISION_NUMBER} CACHE INTERNAL "" FORCE)
endmacro ()

# Find a file.
# This command is equivalent to the built-in find_file CMake provides.
# The only exception is that it checks for a revision number to make sure that CMake 
# updates paths after a developer moved or renamed a previously cached directoy.
# Whenever you move or rename a file which might be used in any calls to ccl_find_file, 
# update the revision number CCL_CMAKE_REVISION_NUMBER at the top of this file
macro (ccl_find_file var)
	if (NOT ${var}_FILE_CMAKE_REVISION_NUMBER OR CCL_CMAKE_REVISION_NUMBER GREATER ${var}_FILE_CMAKE_REVISION_NUMBER)
		unset (${var} CACHE)
		unset (${var})
	endif ()
	find_file (${var} ${ARGN})
	set (${var}_FILE_CMAKE_REVISION_NUMBER ${CCL_CMAKE_REVISION_NUMBER} CACHE INTERNAL "" FORCE)
endmacro ()

# Find a program.
# This command is equivalent to the built-in find_program CMake provides.
# The only exception is that it checks for a revision number to make sure that CMake 
# updates paths after a developer moved or renamed a previously cached directoy.
# Whenever you move or rename a program which might be used in any calls to ccl_find_program, 
# update the revision number CCL_CMAKE_REVISION_NUMBER at the top of this file
macro (ccl_find_program var)
	if (NOT ${var}_PROGRAM_CMAKE_REVISION_NUMBER OR CCL_CMAKE_REVISION_NUMBER GREATER ${var}_PROGRAM_CMAKE_REVISION_NUMBER)
		unset (${var} CACHE)
		unset (${var})
	endif ()
	find_program (${var} ${ARGN})
	set (${var}_PROGRAM_CMAKE_REVISION_NUMBER ${CCL_CMAKE_REVISION_NUMBER} CACHE INTERNAL "" FORCE)
endmacro ()

set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
get_filename_component (platformdir ${platformdir} ABSOLUTE)
ccl_find_file (corelib_PLATFORMMACROS_FILE NAMES "coremacros.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings.")
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
			message (WARNING "VENDOR_PLATFORM not set.")
			set (VENDOR_PLATFORM "unknown")
		endif ()
	endif ()
	set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
	set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
	get_filename_component (platformdir ${platformdir} ABSOLUTE)
	message (DEBUG "Searching for ${target}.${VENDOR_PLATFORM}.cmake in ${platformdir};${platformdir2};${CMAKE_CURRENT_LIST_DIR}")
	ccl_find_file (${target}_PLATFORM_FILE NAMES "${target}.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings used when compiling ${target}.")
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
		set (subdir "${ARGV1}")
	else ()
		set (subdir "")
	endif ()
	
	if (NOT VENDOR_OUTPUT_DIRECTORY)
		set (VENDOR_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	endif ()

	set (subdirectory "${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/$<CONFIG>/${CCL_ISOLATION_POSTFIX}/${subdir}/")
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
		
		if (CCL_ISOLATION_POSTFIX AND ${ARGC} GREATER 2 AND NOT "${ARGV2}" STREQUAL "${target}")
			target_compile_definitions (${target} PUBLIC "CCL_ISOLATION_POSTFIX=\".${CCL_ISOLATION_POSTFIX}\"" "CCL_EXPORT_POSTFIX=${CCL_EXPORT_POSTFIX}")
		endif ()

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

			# Configure code analysis
			if (ENABLE_CODE_ANALYSIS)
				set_target_properties (${target} PROPERTIES VS_GLOBAL_CodeAnalysisRuleSet "${CCL_REPOSITORY_ROOT}/build/win/codeanalysis.ruleset")
				set_target_properties (${target} PROPERTIES VS_GLOBAL_RunCodeAnalysis "true")
			endif ()
		endif ()

		if (XCODE)
			if (IOS)
				# iOS does not support dylibs or .bundle packages for plug-ins, only frameworks
				if (target_type STREQUAL "SHARED_LIBRARY")
					set_target_properties (${target} PROPERTIES
						FRAMEWORK TRUE
					)
					target_compile_options (${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX,C>:-fno-constant-cfstrings>")
				endif ()
			else ()
				if(target_type STREQUAL "MODULE_LIBRARY")
			 		set_target_properties (${target} PROPERTIES 
						BUNDLE TRUE
					)
					target_compile_options (${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX,C>:-fno-constant-cfstrings>")
				endif()
			endif ()

			if ("${${target}_VERSION_BUILD}" STREQUAL "")
				set_target_properties (${target} PROPERTIES
					MACOSX_BUNDLE_BUNDLE_VERSION "1"
					MACOSX_FRAMEWORK_BUNDLE_VERSION "1"
				)
			else ()
				set_target_properties (${target} PROPERTIES
					MACOSX_BUNDLE_BUNDLE_VERSION "${${target}_VERSION_BUILD}"
					MACOSX_FRAMEWORK_BUNDLE_VERSION "${${target}_VERSION_BUILD}"
				)
			endif ()

			if ("${${target}_SHORT_VERSION}" STREQUAL "")
				set_target_properties (${target} PROPERTIES
					MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
					MACOSX_FRAMEWORK_SHORT_VERSION_STRING "1.0.0"
				)
			else ()
				set_target_properties (${target} PROPERTIES
					MACOSX_BUNDLE_SHORT_VERSION_STRING "${${target}_SHORT_VERSION}"
					MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${${target}_SHORT_VERSION}"
				)
			endif ()

			set_target_properties (${target} PROPERTIES
				MACOSX_BUNDLE_COPYRIGHT "${${target}_COPYRIGHT}"
				MACOSX_BUNDLE_BUNDLE_NAME "${${target}_NAME}"
				XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "${CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH}"
				XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "" # prevent cmake adding bogus entries to frameworks searchs paths (breaks iOS simulator builds)
				XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "${CMAKE_XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES}"
				XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "${CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS}"
				XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "${CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL}"
				XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "${CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN}"
				XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "${CMAKE_XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN}"
			)
			if (IOS)
				set_target_properties (${target} PROPERTIES
					XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks @loader_path/Frameworks"
				)
			else ()
				set_target_properties (${target} PROPERTIES
					XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @loader_path/../Frameworks"
				)
			endif ()

			get_target_property (is_framework ${target} FRAMEWORK) # there is no MACOSX_FRAMEWORK_COPYRIGHT as of cmake 3.26.3 : use workaround with a user defined Xcode variable
			if (is_framework)
				set_target_properties (${target} PROPERTIES
					XCODE_ATTRIBUTE_FRAMEWORK_COPYRIGHT "${${target}_COPYRIGHT}"
				)
			endif ()
			# if (NOT IOS AND VENDOR_USE_PUBLISHER_CERTIFICATE)
			# 	set_target_properties (${target} PROPERTIES
			# 		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
			# 		XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${SIGNING_TEAMID_MAC}"
			# 		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
			# 	)
			# endif()
		endif ()
		
		if (PROJECT_VENDOR)
			ccl_set_project_vendor (${PROJECT_VENDOR})
		endif ()

		if (PROJECT_VENDOR AND NOT ${target}_VENDOR)
			ccl_set_vendor (${target} ${PROJECT_VENDOR})
		endif ()
	endif ()
	
	# use custom output directory
	if (${ARGC} GREATER 1)
		ccl_get_build_output_directory (CORE_BUILDOUTPUT_DIRECTORY ${ARGV1})
	else ()
		ccl_get_build_output_directory (CORE_BUILDOUTPUT_DIRECTORY)
	endif ()

	if (VENDOR_BUILDOUTPUT_DIRECTORY)
		set (target_buildoutput_directory "${VENDOR_BUILDOUTPUT_DIRECTORY}")
	else ()
		set (target_buildoutput_directory "${CORE_BUILDOUTPUT_DIRECTORY}")
	endif ()

	if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
		set_target_properties (${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${target_buildoutput_directory}")
		set_target_properties (${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${target_buildoutput_directory}")
		set_target_properties (${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${target_buildoutput_directory}")
		set_target_properties (${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
		
		cmake_path (RELATIVE_PATH target_buildoutput_directory BASE_DIRECTORY "${REPOSITORY_ROOT}/build" OUTPUT_VARIABLE relative_build_directory)
		target_compile_definitions (${target} PRIVATE "CORE_BUILD_PATH=\"${relative_build_directory}\"")
	endif ()
	
	# add platform implementation files and other platform-specific definitions
	if (${ARGC} GREATER 2)
		ccl_include_platform_specifics (${ARGV2})
	else ()
		ccl_include_platform_specifics (${target})
	endif ()

	# allow platform implementation file to overwrite the PACKAGE_ID
	if (XCODE)
		if ("${CCL_ISOLATION_POSTFIX}" STREQUAL "")
			set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${${target}_PACKAGE_ID}"
														MACOSX_BUNDLE_GUI_IDENTIFIER "${${target}_PACKAGE_ID}")
		else ()
			set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${${target}_PACKAGE_ID}.${CCL_ISOLATION_POSTFIX}"
														MACOSX_BUNDLE_GUI_IDENTIFIER "${${target}_PACKAGE_ID}.${CCL_ISOLATION_POSTFIX}")
		endif ()
	endif ()

	if (COMMAND ccl_process_plugins)
		ccl_process_plugins (${target})
	endif ()
endmacro ()

# Check if a target already exists
# @param {STRING} target  Name of the target.
# @param {STRING} result  Name of the result variable.
macro (ccl_check_target_exists target result)
	set (${result} OFF)
	if (TARGET ${target} AND "${CCL_ISOLATION_POSTFIX}" STREQUAL "")
		set (${result} ON)
		set (${target}_target "${target}")
	elseif (TARGET ${target}.${CCL_ISOLATION_POSTFIX})
		set (${result} ON)
		set (${target}_target "${target}.${CCL_ISOLATION_POSTFIX}")
	endif()
endmacro ()

# Add a library, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
# @param {STRING} type  Target type, e.g. STATIC, MODULE, INTERFACE.
# @param {STRING} SUBDIR  [optional] Subdirectory to place the build artifact in, e.g. "Plugins".
# @param {STRING} VENDOR  [optional] Vendor ID, used to determine version information. See ccl_set_vendor.
# @param {FILEPATH} VERSION_FILE  [optional] Path to a version header file.
# @param {STRING} VERSION_PREFIX  [optional] Prefix used in the version header file, e.g. APP or PLUG.
# @param {STRING} POSTFIX  [optional] Postfix to append to the target name and binary output name.
macro (ccl_add_library target type)
	cmake_parse_arguments (target_params "" "SUBDIR;VENDOR;VERSION_FILE;VERSION_PREFIX;POSTFIX" "" ${ARGN})

	set (target_name "${target}")
	if (target_params_POSTFIX)
		string (APPEND target_name ".${target_params_POSTFIX}")
	endif ()
	
	if (IOS AND ${type} STREQUAL "MODULE")
		add_library (${target_name} SHARED ${target_params_UNPARSED_ARGUMENTS})
	else ()
		add_library (${target_name} ${type} ${target_params_UNPARSED_ARGUMENTS})
	endif ()
	
	if (target_params_VENDOR)
		ccl_set_vendor (${target_name} ${target_params_VENDOR})
	endif ()
	
	if (target_params_VERSION_FILE)
		ccl_read_version (${target} "${target_params_VERSION_FILE}" ${target_params_VERSION_PREFIX})
		if (target_params_POSTFIX)
			ccl_read_version (${target_name} "${target_params_VERSION_FILE}" ${target_params_VERSION_PREFIX})
		endif ()
	endif ()

	get_target_property (imported ${target_name} IMPORTED)
	if (NOT imported AND NOT ${type} STREQUAL "ALIAS")
		ccl_configure_target (${target_name} "${target_params_SUBDIR}" "${target}")
	endif ()
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
	target_sources (${target_name} PRIVATE ${corelib_malloc_sources})
	source_group ("source\\libs" FILES ${corelib_malloc_sources})
	target_link_libraries (${target_name} PUBLIC corelib)
	target_compile_definitions (${target_name} PRIVATE __DSP_NO_CCL_FRAMEWORK__)
endmacro ()

# Add a plug-in library, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
macro (ccl_add_core_plugin_library target type)
	ccl_add_core_library (${target} ${type} ${ARGN})
	if(XCODE)
		set_target_properties (${target} PROPERTIES
			BUNDLE NO
		)
	endif()
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

		if ("${VENDOR_PLATFORM}" STREQUAL "android")
			ccl_generate_version_properties (${target})
		endif ()
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
	target_sources (${target} PRIVATE ${corelib_malloc_sources})
	source_group ("source\\libs" FILES ${corelib_malloc_sources})
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

	set (VENDOR_NAME "")
	set (VENDOR_COPYRIGHT_YEAR "")
	set (VENDOR_COPYRIGHT "")
	set (VENDOR_WEBSITE "")
	set (VENDOR_MAIL "")
	set (VENDOR_PACKAGE_DOMAIN "")
	set (VENDOR_MIME_TYPE "")
	set (VENDOR_INSTALL_SUBDIR "")
	set (VENDOR_PUBLISHER "")
	
	set (vendor_id ${vendor})
	
	ccl_find_path (${vendor}_IDENTITY_DIR NAMES "${vendor}/identity.cmake" HINTS ${VENDOR_IDENTITY_DIRS})
	ccl_find_path (${vendor}_SIGNING_DIR NAMES "${vendor}/signing.cmake" HINTS ${VENDOR_SIGNING_DIRS})

	include (${${vendor}_IDENTITY_DIR}/${vendor}/identity.cmake)
	if (EXISTS ${${vendor}_SIGNING_DIR}/${vendor}/signing.cmake)
		include (${${vendor}_SIGNING_DIR}/${vendor}/signing.cmake)
	endif ()

	# set target-specific vendor information
	foreach (var VENDOR_NAME VENDOR_COPYRIGHT_YEAR VENDOR_COPYRIGHT VENDOR_WEBSITE VENDOR_MAIL VENDOR_PACKAGE_DOMAIN VENDOR_MIME_TYPE VENDOR_INSTALL_SUBDIR VENDOR_PUBLISHER)
		set (${target}_${var} "${${var}}")
	endforeach ()

	# generate a vendor.h header file
	set (vendor_include_directory "${CMAKE_BINARY_DIR}/vendor/${vendor}")
	file (MAKE_DIRECTORY "${vendor_include_directory}")
	configure_file ("${CCL_IDENTITIES_DIR}/shared/vendor.h.in" "${vendor_include_directory}/vendor.h.new")
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
	if (NOT "${CURRENT_PROJECT_VENDOR}" STREQUAL "${vendor}" OR NOT "${CURRENT_ISOLATION_POSTFIX}" STREQUAL "${CCL_ISOLATION_POSTFIX}")

		set (CURRENT_PROJECT_VENDOR "${vendor}")
		set (CURRENT_ISOLATION_POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set (PROJECT_VENDOR "${vendor}")

		set (VENDOR_NAME "")
		set (VENDOR_COPYRIGHT_YEAR "")
		set (VENDOR_COPYRIGHT "")
		set (VENDOR_WEBSITE "")
		set (VENDOR_MAIL "")
		set (VENDOR_PACKAGE_DOMAIN "")
		set (VENDOR_MIME_TYPE "")
		set (VENDOR_INSTALL_SUBDIR "")
		set (VENDOR_PUBLISHER "")
	
		# reset platform-specific variables
		if (COMMAND ccl_reset_project_vendor_variables)
			ccl_reset_project_vendor_variables ()
		endif ()
	
		set (vendor_id ${vendor})

		ccl_find_path (${vendor}_IDENTITY_DIR NAMES "${vendor}/identity.cmake" HINTS ${VENDOR_IDENTITY_DIRS})
		ccl_find_path (${vendor}_SIGNING_DIR NAMES "${vendor}/signing.cmake" HINTS ${VENDOR_SIGNING_DIRS})

		include (${${vendor}_IDENTITY_DIR}/${vendor}/identity.cmake)
		if (EXISTS ${${vendor}_SIGNING_DIR}/${vendor}/signing.cmake)
			include (${${vendor}_SIGNING_DIR}/${vendor}/signing.cmake)
		endif ()
	
		foreach (var VENDOR_NAME VENDOR_COPYRIGHT_YEAR VENDOR_COPYRIGHT VENDOR_WEBSITE VENDOR_MAIL VENDOR_PACKAGE_DOMAIN VENDOR_MIME_TYPE VENDOR_INSTALL_SUBDIR VENDOR_PUBLISHER)
			set (PROJECT_${var} "${${var}}")
		endforeach ()
	
		# generate a vendor.h header file
		set (vendor_include_directory "${CMAKE_BINARY_DIR}/vendor/${vendor}")
		file (MAKE_DIRECTORY "${vendor_include_directory}")
		configure_file ("${CCL_IDENTITIES_DIR}/shared/vendor.h.in" "${vendor_include_directory}/vendor.h.new")
		file (COPY_FILE "${vendor_include_directory}/vendor.h.new" "${vendor_include_directory}/vendor.h" ONLY_IF_DIFFERENT)
		list (PREPEND VENDOR_INCLUDE_DIRS "${vendor_include_directory}")

		# apply platform-specific configuration
		if (COMMAND ccl_configure_project_vendor)
			ccl_configure_project_vendor (${vendor})
		endif ()
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

# Add asset files to a target.
# All asset files are placed in the application directory.
# You may place all asset files in a subdirectory using an optional `PATH <subdirectory>` argument.
# @param {STRING} target  Name of the target to add asset files to.
# @param {PATH} PATH  [optional] Destination subdirectory.
# @param {FILEPATH} FILES  [variadic] Asset files.
# @param {PATH} DIRECTORY  [variadic] Asset directories.
macro (ccl_add_assets target)
	cmake_parse_arguments (asset_params "" "PATH" "DIRECTORY;FILES" ${ARGN})
	if(NOT asset_params_PATH)
		set (asset_params_PATH "")
	endif ()

	if (COMMAND ccl_process_assets)
		if (asset_params_DIRECTORY)
			ccl_process_assets ("${target}" DIRECTORY ${asset_params_DIRECTORY} PATH "${asset_params_PATH}")
		elseif (asset_params_FILES)
			ccl_process_assets ("${target}" FILES ${asset_params_FILES} PATH "${asset_params_PATH}")
		else ()
			message (WARNING "ccl_add_assets called without FILES or DIRECTORY arguments")
		endif ()
	endif ()
endmacro ()

# Add resource files to a target.
# All resource files are placed in a flat resource directory.
# You may place all resource files in a subdirectory using an optional `PATH <subdirectory>` argument.
# @param {STRING} target  Name of the target to add resource files to.
# @param args  [variadic] Resource files.
macro (ccl_add_resources target)
	cmake_parse_arguments (resource_params "" "CONFIG;PATH" "" ${ARGN})
	if(NOT resource_params_CONFIG OR CCL_SYSTEM_INSTALL)
		set (resource_params_CONFIG "")
	endif ()
	if(NOT resource_params_PATH)
		set (resource_params_PATH "")
	endif ()
	set (resources ${resource_params_UNPARSED_ARGUMENTS})
	if ("${resource_params_CONFIG}" STREQUAL "" OR APPLE OR ANDROID)
		target_sources (${target} PRIVATE ${resources})
	else ()
		target_sources (${target} PRIVATE $<$<CONFIG:${resource_params_CONFIG}>:${resources}>)	
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

	if (COMMAND ccl_process_dependencies)
		ccl_process_dependencies (${target} ITEMS ${items})
	endif ()
endmacro ()

# Preprocess a header file and read the values of peprocessor definitions.
# @param {FILEPATH} file  Source file with preprocessor definitions.
# @param args  [variadic] List of pairs <var> <macro>, where <var> is the name of an output variable and <macro> is a macro to evaluate and store into <var>.
macro (ccl_evaluate_macros file)
	cmake_parse_arguments (evaluation_params "" "" "INCLUDE_DIRECTORIES;MACROS" ${ARGN})
	
	list (APPEND evaluation_params_INCLUDE_DIRECTORIES ${VENDOR_INCLUDE_DIRS})
	list (APPEND evaluation_params_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/vendor/ccl")
	
	ccl_find_file (buildnumber_file NAMES "buildnumber.h" HINTS ${evaluation_params_INCLUDE_DIRECTORIES})
	if (NOT buildnumber_file)
		message (WARNING "Couldn't find a buldnumber header file. Generating a new one...")
		ccl_find_program (SHELL sh REQUIRED NAMES sh bash PATHS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer/Git/usr/bin" REQUIRED)
		ccl_find_file (buildnumber_script NAMES "update_buildnumber.sh" HINTS ${CCL_REPOSITORY_ROOT}/build/shared REQUIRED)
		set (buildnumber_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include/buildnumber")	
		file (MAKE_DIRECTORY "${buildnumber_include_dir}")
		execute_process (COMMAND "${SHELL}" "${buildnumber_script}" "${buildnumber_include_dir}/buildnumber.h" 1)
		list (APPEND evaluation_params_INCLUDE_DIRECTORIES "${buildnumber_include_dir}")
		list (APPEND VENDOR_INCLUDE_DIRS "${buildnumber_include_dir}")
	endif ()
	
	ccl_find_file (buildtime_file NAMES "buildtime.h" HINTS ${evaluation_params_INCLUDE_DIRECTORIES})
	if (NOT buildtime_file)
		message (WARNING "Couldn't find a buildtime header file. Generating a new one...")
		ccl_find_program (SHELL sh REQUIRED NAMES sh bash PATHS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer/Git/usr/bin" REQUIRED)
		ccl_find_file (buildtime_script NAMES "update_buildtime.sh" HINTS ${CCL_REPOSITORY_ROOT}/build/shared REQUIRED)
		set (buildtime_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include/buildtime")	
		file (MAKE_DIRECTORY "${buildtime_include_dir}")
		execute_process (COMMAND "${SHELL}" "${buildtime_script}" "${buildtime_include_dir}/buildtime.h")
		list (APPEND evaluation_params_INCLUDE_DIRECTORIES "${buildtime_include_dir}")
		list (APPEND VENDOR_INCLUDE_DIRS "${buildtime_include_dir}")
	endif ()
	
	if (TARGET corelib)
		get_property (corelib_has_include_dirs TARGET corelib PROPERTY INTERFACE_INCLUDE_DIRECTORIES SET)
		if (corelib_has_include_dirs)
			get_target_property (corelib_INCLUDE_DIRS corelib INTERFACE_INCLUDE_DIRECTORIES)
			list (APPEND evaluation_params_INCLUDE_DIRECTORIES ${corelib_INCLUDE_DIRS})
		endif ()
	endif ()
	
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
			get_property (target_has_include_dirs TARGET ${target} PROPERTY INCLUDE_DIRECTORIES SET)
			if (target_has_include_dirs)
				get_target_property (target_INCLUDE_DIRS ${target} INCLUDE_DIRECTORIES)
			endif ()
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

# Get all properties cmake supports.
macro (get_properties var)
	# Get all propreties that cmake supports
	execute_process (COMMAND cmake --help-property-list OUTPUT_VARIABLE ${var})

	# Convert command output into a CMake list
	string (REGEX REPLACE ";" "\\\\;" ${var} "${${var}}")
	string (REGEX REPLACE "\n" ";" ${var} "${${var}}")
endmacro ()

# Print all properties that are defined for a target.
# @param {STRING} target  Name of the target to print the properties for.
function (ccl_print_target_properties target)
	if(NOT TARGET ${target})
		message (WARNING "There is no target named '${target}'")
		return()
	endif()
	
	get_properties (CMAKE_PROPERTY_LIST)
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
			message (NOTICE "${target} ${prop} = ${propval}")
		endif ()
	endforeach ()
endfunction ()

# Print all properties that are defined for the current directory.
function (ccl_print_directory_properties)
	get_properties (CMAKE_PROPERTY_LIST)
	foreach (prop ${CMAKE_PROPERTY_LIST})
		string (REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
		# Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
		if (prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
			continue ()
		endif ()
		# message ("Checking ${prop}")
		get_directory_property (propval ${prop})
		if (NOT "${propval}" STREQUAL "")
			message (NOTICE "${prop} = ${propval}")
		endif ()
	endforeach ()
endfunction ()

# Print all properties that are defined for a source file.
# @param {FILEPATH} file  Path to a file to print the properties for.
function (ccl_print_source_file_properties file)
	get_properties (CMAKE_PROPERTY_LIST)
	foreach (prop ${CMAKE_PROPERTY_LIST})
		string (REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
		# Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
		if (prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
			continue ()
		endif ()
		# message ("Checking ${prop}")
		get_source_file_property (propval ${file} ${prop})
		if (NOT "${propval}" STREQUAL "NOTFOUND")
			message (NOTICE "${file} ${prop} = ${propval}")
		endif ()
	endforeach ()
endfunction ()

# Print all variables.
function (ccl_print_variables)
	get_cmake_property (_variableNames VARIABLES)
	list (SORT _variableNames)
	foreach (_variableName ${_variableNames})
		message (NOTICE "${_variableName}=${${_variableName}}")
	endforeach ()
endfunction ()

# Setup options for an external project.
# @param {STRING} target  Name of the external project target to process.
# @param {LIST} options  Options to add to the external project's CMake invocation.
macro (ccl_setup_external_project_options target options)
	list (APPEND _${target}_options
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		${options}
	)

	if (${target}_DEPLOYMENT_DIR)
		list (APPEND _${target}_options "-DCMAKE_INSTALL_PREFIX=${${target}_DEPLOYMENT_DIR}")
	endif ()

	ccl_include_platform_specifics (${target})

	if (COMMAND ccl_setup_external_project_platform_options)
		ccl_setup_external_project_platform_options (${target})
	endif ()

	list (APPEND ${target}_platform_options
		"-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} ${${target}_c_flags}"
		"-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} ${${target}_cxx_flags}"
	)
endmacro ()

# Remove external project cache file when running without cache.
# @param {STRING} target  Name of the external project target to process.
macro (ccl_prepare_external_project_cache target)
    if (NOT DEFINED CACHE{_${target}_cached})
		file (REMOVE "${${target}_CONFIG_DIR}/CMakeCache.txt")
		set (_${target}_cached CACHE INTERNAL "Used for testing whether cache is used when configuring ${target}")
    endif()
endmacro ()

# Add an external project.
# @param {STRING} target  Name of the target to add for the external project.
# @param {PATH} sourcedir  Folder containing the external project's CMakeLists.txt.
# @param {BOOL} CONFIGURE_ONLY  [optional] Skip build and install steps.
# @param {STRING} BUILD_COMMAND  [optional] Custom build command.
# @param {STRING} INSTALL_COMMAND  [optional] Custom install command.
# @param {LIST} OPTIONS  [optional] Options to add to the external project's CMake invocation.
macro (ccl_add_external_project target sourcedir)
	cmake_parse_arguments (target_params "CONFIGURE_ONLY" "BUILD_COMMAND;INSTALL_COMMAND" "OPTIONS" ${ARGN})

	string (REPLACE ";" "-" _${target}_prefix "${CMAKE_BINARY_DIR}/ext/${target}/${VENDOR_TARGET_ARCHITECTURE}") # VENDOR_TARGET_ARCHITECTURE can be a list e.g. for Apple platforms 
	set (${target}_CONFIG_DIR "${_${target}_prefix}/src/${target}-build")

	if (NOT ${target_params_CONFIGURE_ONLY})
		set (${target}_DEPLOYMENT_DIR "${_${target}_prefix}/deployment")
		set (${target}_INCLUDE_DIR "${${target}_DEPLOYMENT_DIR}/include")
		file (MAKE_DIRECTORY "${${target}_INCLUDE_DIR}")
	endif ()

	ccl_setup_external_project_options (${target} "${target_params_OPTIONS}")
	ccl_prepare_external_project_cache (${target})

	include (ExternalProject)

	if (${target_params_CONFIGURE_ONLY} OR (target_params_BUILD_COMMAND AND target_params_INSTALL_COMMAND))
		ExternalProject_Add (${target}
			PREFIX "${_${target}_prefix}"
			SOURCE_DIR  "${sourcedir}"
			LIST_SEPARATOR "|"
			CMAKE_ARGS --no-warn-unused-cli -Wno-dev ${_${target}_options} ${${target}_platform_options}
			BUILD_BYPRODUCTS "${${target}_BUILD_BYPRODUCTS}"
			BUILD_COMMAND "${target_params_BUILD_COMMAND}"
			INSTALL_COMMAND "${target_params_INSTALL_COMMAND}"
			LOG_CONFIGURE ON
			LOG_INSTALL ON
			LOG_BUILD ON
			LOG_OUTPUT_ON_FAILURE ON
		)
	elseif (target_params_INSTALL_COMMAND)
		ExternalProject_Add (${target}
			PREFIX "${_${target}_prefix}"
			SOURCE_DIR  "${sourcedir}"
			LIST_SEPARATOR "|"
			CMAKE_ARGS --no-warn-unused-cli -Wno-dev ${_${target}_options} ${${target}_platform_options}
			BUILD_BYPRODUCTS "${${target}_BUILD_BYPRODUCTS}"
			INSTALL_COMMAND "${target_params_INSTALL_COMMAND}"
			LOG_CONFIGURE ON
			LOG_INSTALL ON
			LOG_BUILD ON
			LOG_OUTPUT_ON_FAILURE ON
		)
	else ()
		ExternalProject_Add (${target}
			PREFIX "${_${target}_prefix}"
			SOURCE_DIR  "${sourcedir}"
			LIST_SEPARATOR "|"
			CMAKE_ARGS --no-warn-unused-cli -Wno-dev ${_${target}_options} ${${target}_platform_options}
			BUILD_BYPRODUCTS "${${target}_BUILD_BYPRODUCTS}"
			LOG_CONFIGURE ON
			LOG_INSTALL ON
			LOG_BUILD ON
			LOG_OUTPUT_ON_FAILURE ON
		)
	endif ()
endmacro ()

# Copy an imported target to the build output directory.
# @param {STRING} target  Name of the imported target.
# @param {STRING} SUBDIR  [optional] Subdirectory to copy the imported target to, e.g. "Plugins".
# @param {STRING} FOLDER  [optional] Project "folder" as displayed in the IDE.
# @param {STRING} STRING  [optional] Target name of the new copy target.
macro (ccl_copy_imported_target target)
	cmake_parse_arguments (copy_params "" "SUBDIR;FOLDER;TARGET_NAME" "" ${ARGN})

	if (copy_params_TARGET_NAME)
		set (copyTarget "${copy_params_TARGET_NAME}")
	else ()
		set (copyTarget copy_imported_${target})
	endif ()
	
	if (NOT TARGET ${copyTarget})
		add_custom_target (${copyTarget})
		set_target_properties (${copyTarget} PROPERTIES 
			PROJECT_LABEL "${target}"
			FOLDER "${copy_params_FOLDER}"
		)
		
		ccl_get_build_output_directory (outputDir "${copy_params_SUBDIR}")

		get_target_property (imported_location_debug ${target} IMPORTED_LOCATION_DEBUG)
		get_target_property (imported_location_release ${target} IMPORTED_LOCATION_RELEASE)
		get_target_property (imported_location ${target} IMPORTED_LOCATION)

		if (NOT imported_location_debug)
			set (imported_location_debug "${imported_location}")
		endif ()
		if (NOT imported_location_release)
			set (imported_location_release "${imported_location}")
		endif ()

		unset (bundle_location_debug)
		unset (bundle_location_release)
		if (APPLE)
			ccl_get_bundle_dir (bundle_location_debug "${imported_location_debug}")
			ccl_get_bundle_dir (bundle_location_release "${imported_location_release}")
		endif ()

		if (bundle_location_debug AND bundle_location_release)
			add_custom_command (TARGET ${copyTarget} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${outputDir}/"
				COMMAND rsync -rl
				"$<IF:$<CONFIG:Release,RelWithDebInfo>,${bundle_location_release},${bundle_location_debug}>" "${outputDir}/"
			)
		else ()
			add_custom_command (TARGET ${copyTarget} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${outputDir}/"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"$<IF:$<CONFIG:Release,RelWithDebInfo>,${imported_location_release},${imported_location_debug}>" "${outputDir}/"
			)
		endif ()

		list (APPEND ${target}_cmake_sources "${CMAKE_CURRENT_LIST_FILE}")
		source_group ("cmake" FILES ${target_cmake_sources})

		add_dependencies (${target} ${copyTarget})
	endif ()
endmacro ()

# Restore a file from cache.
# @param {STRING} cache_entry  Unique cache entry identifier.
# @param {STRING} FILE_NAME  Name of the file to restore from cache.
# @param {FILEPATH} DESTINATION  Directory to copy the file to.
macro (ccl_restore_from_cache cache_entry)
	cmake_parse_arguments (cache_params "" "FILE_NAME;DESTINATION;" "" ${ARGN})
	if (VENDOR_CACHE_DIRECTORY AND cache_params_FILE_NAME AND cache_params_DESTINATION)
		set (entry_cache_path "${VENDOR_CACHE_DIRECTORY}/${cache_entry}/${cache_params_FILE_NAME}")
		if (EXISTS "${entry_cache_path}")
			file (MAKE_DIRECTORY "${cache_params_DESTINATION}")
			file (COPY "${entry_cache_path}" DESTINATION "${cache_params_DESTINATION}/")
			file (TOUCH_NOCREATE "${cache_params_DESTINATION}/${cache_params_FILE_NAME}")
			message (STATUS "Restored ${cache_entry} from cache")
		else ()
			message (STATUS "Cache miss for ${cache_entry}. File does not exist: ${entry_cache_path}")
		endif ()
	endif ()
endmacro ()

# Add a file to cache.
# @param {STRING} cache_entry  Unique cache entry identifier.
# @param {FILEPATH} file_path  Path to the file.
macro (ccl_add_to_cache cache_entry file_path)
	if (VENDOR_CACHE_DIRECTORY)
		set (entry_cache_path "${VENDOR_CACHE_DIRECTORY}/${cache_entry}")
		file (MAKE_DIRECTORY "${entry_cache_path}")
		file (COPY "${file_path}" DESTINATION "${entry_cache_path}/")
		message (STATUS "Caching ${cache_entry} as ${file_path} at ${entry_cache_path}")
	endif ()
endmacro ()

# Create ZIP archive. Note that paths in the ZIP file will be relative to the INPUT argument.
# @param {STRING} INPUT  Name of directory containing folders and files to include in the ZIP archive.
# @param {STRING} OUTPUT  Name of the file to create.
macro (ccl_makezip)
	cmake_parse_arguments (makezip_params "" "INPUT;OUTPUT;" "" ${ARGN})

	add_custom_command (
		OUTPUT "${makezip_params_OUTPUT}"
		COMMAND "${CMAKE_COMMAND}" -E tar -cf "${makezip_params_OUTPUT}" --format=zip .
		WORKING_DIRECTORY "${makezip_params_INPUT}"
		COMMENT "Generating ZIP archive ${makezip_params_OUTPUT}..."
		VERBATIM
	)
endmacro ()

# Add headers files to a target. For Xcode, emulate the FILE_SET feature to install the files
# @param {STRING} target Name of the target to add the headers to.
# @param {BOOL} INSTALL [optional] Also install the files.
# @param {STRING} DESTINATION  Name of directory to write the files when installing.
# @param {LIST} BASE_DIRS List of base directories which are stripped from the header file paths.
# @param {LIST} FILES List of paths to header files.
macro (ccl_target_headers target)
	cmake_parse_arguments (headers_params "" "INSTALL;DESTINATION" "BASE_DIRS;FILES;" ${ARGN})
	if (XCODE)
		target_sources (${target} PRIVATE ${headers_params_FILES})
		if (headers_params_INSTALL)
			foreach (f IN LISTS headers_params_FILES)
				cmake_path (GET f PARENT_PATH filedir)
				foreach (b IN LISTS headers_params_BASE_DIRS)
					cmake_path (IS_PREFIX b "${f}" NORMALIZE basedir_matches)
					if (basedir_matches)
						cmake_path (RELATIVE_PATH filedir BASE_DIRECTORY "${b}")
						install (FILES "${f}" DESTINATION "${headers_params_DESTINATION}/${filedir}" COMPONENT public_headers)
						break ()
					endif ()
				endforeach ()
			endforeach ()
		endif ()
	else ()
		target_sources (${target} PUBLIC FILE_SET HEADERS BASE_DIRS ${headers_params_BASE_DIRS} FILES ${headers_params_FILES})
	endif ()
endmacro ()

# Ensure a file is created and initialized exactly once.
# @param {STRING} file  File name.
# @param {STRING} CONTENT  [variadic,optional] Initial file content.
macro (ccl_create_file_once file)
	cmake_parse_arguments (file_params "" "" "CONTENT" ${ARGN})

	ccl_check_file_created ("${file}" ccl_create_file_once_file_exists)

	if (NOT ccl_create_file_once_file_exists)
		file (WRITE "${file_normalized}" "")

		foreach (line ${file_params_CONTENT})
			file (APPEND "${file_normalized}" "${line}")
		endforeach ()

		list (APPEND generated_files "${file_normalized}")
		set_property (GLOBAL PROPERTY generated_files "${generated_files}")
	endif ()
endmacro ()

# Append items to a list and filter duplicate items
# @param {STRING} variable  Name of the list variable.
# @param {STRING} CONTENT  [variadic,optional] Content to append to the list.
macro (ccl_list_append_once variable)
	list (APPEND ${variable} ${ARGN})
	list (REMOVE_DUPLICATES ${variable})
endmacro ()

# Check if a file has been created using \a ccl_create_file_once
# @param {STRING} file  File name.
# @param {STRING} result  Result variable. Set to ON if the file has already been created. Set to OFF otherwise.
macro (ccl_check_file_created file result)
	cmake_path (SET file_normalized NORMALIZE "${file}")
	get_filename_component (base_dir "${file_normalized}" DIRECTORY)
	file (MAKE_DIRECTORY "${base_dir}")

	get_property (generated_files GLOBAL PROPERTY generated_files)

	if ("${file_normalized}" IN_LIST generated_files)
		set (${result} ON)
	else ()
		set (${result} OFF)
	endif ()
endmacro ()

# Get the name of an isolated target, if it exists.
# Returns the plain target name if an isolated variant does not exist
# @param {STRING} target  Name of the target without isolation postfix.
# @param {STRING} result  Name of the result variable.
macro (ccl_get_isolated_target plain_target result)
	set (${result} "${plain_target}")
	if (NOT "${CCL_ISOLATION_POSTFIX}" STREQUAL "")
		if (TARGET "${plain_target}.${CCL_ISOLATION_POSTFIX}")
			string (APPEND ${result} ".${CCL_ISOLATION_POSTFIX}")
		endif ()
	endif ()
endmacro ()

# Create a .plugin file for native module loading.
# @param {STRING} module  Module name.
# @param {STRING} LOCATION  Location to add the .plugin file to.
macro (ccl_create_plugin_file module)
	cmake_parse_arguments (params "" "LOCATION" "" ${ARGN})

	if (NOT params_LOCATION)
		set (params_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/modules")
	endif ()

	if (IOS)
		set (extension "framework")
	elseif (ANDROID)
		set (extension "so")
	else ()
		message (WARNING "ccl_create_plugin_file: Plaform not supported")
	endif ()

	file (WRITE "${params_LOCATION}/${module}.plugin" "Lib=lib${module}.${extension}\n")
endmacro ()

# Get the build output path of an extension library.
# @param {PATH} result  Output argument, vendor and platform specific library build directory.
# @param {STRING} subdir  Subdirectory of the extension library, e.g. plugins or dsp.
# @param {STRING} extension  Name of the extension.
macro (ccl_get_extension_library_path result subdir extension)
	if (NOT EXTENSIONS_BINARY_SUBDIRECTORY)
		set (EXTENSIONS_BINARY_SUBDIRECTORY "${VENDOR_PLATFORM}")
	endif ()
	if (NOT REPOSITORY_EXTENSIONS_DIR)
		set (REPOSITORY_EXTENSIONS_DIR "${REPOSITORY_ROOT}/extensions")
	endif ()
	set (${result} "${REPOSITORY_EXTENSIONS_DIR}/deployment/${extension}/${subdir}/${EXTENSIONS_BINARY_SUBDIRECTORY}")
endmacro ()

# Preconfigure an extension library to match vendor guidelines.
# @group extensions
# @param {STRING} target  Name of a target to configure.
# @param {STRING} subdir  Subdirectory of the extension library, e.g. plugins or dsp.
# @param {STRING} extension  Name of the extension.
macro (ccl_extensions_configure_target target subdir extension)
	ccl_get_extension_library_path (extension_dir "${subdir}" "${extension}")
	if (MSVC)
		# Disable Arm64X for extensions as they use per architecture subfolders
		ccl_disable_arm64x (${target})
	endif ()

	set_target_properties (${target} PROPERTIES 
		LIBRARY_OUTPUT_DIRECTORY "${extension_dir}"
		RUNTIME_OUTPUT_DIRECTORY "${extension_dir}"
		COMPILE_PDB_OUTPUT_DIRECTORY "${extension_dir}"
	)
	foreach (buildconfig ${CMAKE_CONFIGURATION_TYPES})
		string (TOUPPER ${buildconfig} buildconfig)
		set_target_properties (${target} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY_${buildconfig} "${extension_dir}"
			RUNTIME_OUTPUT_DIRECTORY_${buildconfig} "${extension_dir}"
			COMPILE_PDB_OUTPUT_DIRECTORY_${buildconfig} "${extension_dir}"
		)
	endforeach ()
	
	target_include_directories (${target} PRIVATE ${VENDOR_INCLUDE_DIRS})
endmacro ()

# Add an extension library using core, preconfigured to match vendor guidelines.
# @group extensions
# @param {STRING} target  Name of the target to add.
# @param {STRING} subdir  Subdirectory of the extension library, e.g. plugins or dsp.
# @param {STRING} extension  [optional] Name of the extension. Defaults to <target>.
macro (ccl_extensions_add_core_library target subdir)
	if (${ARGC} GREATER 3)
		set (extension "${ARGV3}")
	else ()
		set (extension "${target}")
	endif ()

	ccl_add_core_plugin_library (${target} MODULE)
	ccl_extensions_configure_target (${target} ${subdir} ${extension})
endmacro ()

# Add an extension library, preconfigured to match vendor guidelines.
# @group extensions
# @param {STRING} target  Name of the target to add.
# @param {STRING} subdir  Subdirectory of the extension library, e.g. plugins or dsp.
# @param {STRING} extension  [optional] Name of the extension. Defaults to <target>.
macro (ccl_extensions_add_library target subdir)
	if (${ARGC} GREATER 3)
		set (extension "${ARGV3}")
	else ()
		set (extension "${target}")
	endif ()

	ccl_add_plugin_library (${target})
	ccl_extensions_configure_target (${target} ${subdir} ${extension})
endmacro ()

# Add external files to an extension.
# @group extensions
# @param {STRING} target  Name of the target to add files to.
# @param {PATH} directory  Path to the copy the files to.
macro (ccl_import_extension_files target directory)
	foreach (file ${ARGN})
		get_filename_component (filename ${file} NAME)
		add_custom_command (OUTPUT "${directory}/${filename}"
			COMMAND ${CMAKE_COMMAND} -E copy "${file}" "${directory}/${filename}"
			DEPENDS "${file}"
		)
		target_sources (${target} PRIVATE "${directory}/${filename}")
		source_group ("imported" FILES "${directory}/${filename}")
	endforeach ()
endmacro ()
