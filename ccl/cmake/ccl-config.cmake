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
# Filename    : ccl-config.cmake
# Description : CCL CMake Configuration
#
#[*****************************************************************************************[.rst:
# ccl-config.cmake
# -------
#
# Configures CCL framework library targets.
# 
# Targets
# ^^^^^^^
#
# This module provides the following targets, if found:
#
# ``cclapp``
# ``cclbase``
# ``cclgui``
# ``cclnet``
# ``cclsecurity``
# ``cclsystem``
# ``ccltext``
# ``ccltest``
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This will define the following variables:
#
# ``CCL_FOUND``
#   True if the CCL libraries were found.
# ``CCL_VERSION``
#   The version of the CCL framework which was found.
# ``CCL_INCLUDE_DIRS``
#   Include directories needed to use CCL.
# ``CCL_LIBRARIES``
#   Libraries needed to link to CCL.
# ``CCL_STATIC_FOUND``
#   True if static CCL libraries are available.
# ``CCL_STATIC_LIBRARIES``
#   Static libraries which can be used as an alternative to CCL_LIBRARIES. 
#   Use only if CCL_STATIC_FOUND is True.
#]**********************************************************************************************]

set (CCL_LIBRARIES "")
set (CCL_INCLUDE_DIRS "")

set (ZLIB_USE_ZLIB_NG ON CACHE BOOL "Default to using zlib-ng instead of zlib for CCL")

find_package (corelib REQUIRED)

ccl_find_path (CCL_DIR NAMES "public/cclversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "CCL directory.")

option (CCL_STATIC_ONLY "Don't add CCL shared library targets." OFF)
option (CCL_SYSTEM_INSTALL "Install CCL to standard system directories. If OFF, libraries will be installed to the application directory." OFF)
option (CCL_ENABLE_TESTING "Adds the ccltestrunner as a dependency to targets calling ccl_add_test to enable debug executable tests." OFF)

ccl_find_file (CCL_VERSION_FILE NAMES "cclversion.h" HINTS "${CCL_DIR}/public" DOC "CCL version header file.")

# Read CCL version from header file
ccl_read_version (CCL "${CCL_VERSION_FILE}" "CCL")

if (NOT CCL_EXPORT_PREFIX)
	ccl_find_file (CCL_FRAMEWORK_EXPORTS_HEADER NAMES "cclexports.h" HINTS "${CCL_DIR}/public" DOC "CCL framework exports header file.")
	ccl_evaluate_macros (${CCL_FRAMEWORK_EXPORTS_HEADER} MACROS CCL_EXPORT_PREFIX "CCL_EXPORT_PREFIX")
endif ()

set (CCL_LIBRARY_DESTINATION "${VENDOR_LIBRARY_DESTINATION}")
if (VENDOR_STATIC_LIBRARY_DESTINATION)
	set (CCL_STATIC_LIBRARY_DESTINATION "${VENDOR_STATIC_LIBRARY_DESTINATION}")
else ()
	set (CCL_STATIC_LIBRARY_DESTINATION "${VENDOR_LIBRARY_DESTINATION}")
endif ()
if (VENDOR_IMPORT_LIBRARY_DESTINATION)
	set (CCL_IMPORT_LIBRARY_DESTINATION "${VENDOR_IMPORT_LIBRARY_DESTINATION}")
else ()
	set (CCL_IMPORT_LIBRARY_DESTINATION "${VENDOR_LIBRARY_DESTINATION}")
endif ()
set (CCL_BINARY_DESTINATION "${VENDOR_BINARY_DESTINATION}")
if (VENDOR_PLUGINS_DESTINATION)
	set (CCL_PLUGINS_DESTINATION "${VENDOR_PLUGINS_DESTINATION}")
elseif (VENDOR_BINARY_DESTINATION)
	set (CCL_PLUGINS_DESTINATION "${VENDOR_BINARY_DESTINATION}/Plugins")
else ()
	set (CCL_PLUGINS_DESTINATION "${VENDOR_LIBRARY_DESTINATION}/Plugins")	
endif ()
set (CCL_PUBLIC_HEADERS_DESTINATION "${VENDOR_PUBLIC_HEADERS_DESTINATION}/ccl")
if (VENDOR_CMAKE_FILES_DESTINATION)
	set (CCL_CMAKE_EXPORT_DESTINATION "${VENDOR_CMAKE_FILES_DESTINATION}")
elseif (VENDOR_LIBRARY_DESTINATION)
	set (CCL_CMAKE_EXPORT_DESTINATION "${VENDOR_LIBRARY_DESTINATION}/cmake/ccl")
else ()
	set (CCL_CMAKE_EXPORT_DESTINATION "cmake/ccl")
endif ()
if (VENDOR_LICENSE_DESTINATION)
	set (CCL_LICENSE_DESTINATION "${VENDOR_LICENSE_DESTINATION}")
else ()
	set (CCL_LICENSE_DESTINATION "license")
endif ()
if (VENDOR_APPSUPPORT_DESTINATION)
	set (CCL_SUPPORT_DESTINATION "${VENDOR_APPSUPPORT_DESTINATION}")
else ()
	set (CCL_SUPPORT_DESTINATION ".")
endif ()
cmake_path (NORMAL_PATH CCL_LIBRARY_DESTINATION)
cmake_path (NORMAL_PATH CCL_STATIC_LIBRARY_DESTINATION)
cmake_path (NORMAL_PATH CCL_BINARY_DESTINATION)
cmake_path (NORMAL_PATH CCL_PUBLIC_HEADERS_DESTINATION)
cmake_path (NORMAL_PATH CCL_CMAKE_EXPORT_DESTINATION)
cmake_path (NORMAL_PATH CCL_SUPPORT_DESTINATION)

list (APPEND CCL_INCLUDE_DIRS ${VENDOR_INCLUDE_DIRS} ${CCL_DIR}/.. ${corelib_INCLUDE_DIRS})

# Include CCL targets
list (APPEND CMAKE_MODULE_PATH "${CCL_DIR}/cmake")
include (cclmacros)

foreach (component IN ITEMS ${ccl_FIND_COMPONENTS})
	set (target ${component})
	set (listfile "${CCL_DIR}/cmake/${component}.cmake")
	if (NOT EXISTS ${listfile})
		if (${CCL_FIND_REQUIRED_${component}})
			message (FATAL_ERROR "CCL::${component} not found.")
		endif ()
		set (ccl_${component}_FOUND OFF)
	else ()
		include ("${component}")
		set (ccl_${component}_FOUND ON)
		if (NOT "${${component}}" STREQUAL "")
			set (target ${${component}})
		endif ()
		list (APPEND CCL_LIBRARIES "${target}")
		
		get_target_property (imported ${target} IMPORTED)
		get_target_property (target_type ${target} TYPE)
		if (NOT imported AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
			target_include_directories (${target} PUBLIC "$<BUILD_INTERFACE:${CCL_INCLUDE_DIRS}>")
			target_sources (${target} PRIVATE ${CMAKE_CURRENT_LIST_FILE} ${CCL_DIR}/cmake/cclmacros.cmake)
			source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE} ${CCL_DIR}/cmake/cclmacros.cmake)		

			if (CCL_ISOLATION_POSTFIX AND NOT "${${component}}" STREQUAL "")
				target_compile_definitions (${target} PUBLIC "CCL_ISOLATION_POSTFIX=\".${CCL_ISOLATION_POSTFIX}\"" "CCL_EXPORT_POSTFIX=${CCL_EXPORT_POSTFIX}")
			endif ()

			list (FIND ccl_FIND_COMPONENTS cclstatic want_static)
			list (FIND CCL_IS_STATIC ${target} is_static_library)
			if (NOT ${is_static_library} EQUAL -1)
				if (NOT ${want_static} EQUAL -1)
					list (APPEND CCL_STATIC_LIBRARIES "${target}")
				endif ()
			endif ()
		else ()
			target_include_directories (${target} INTERFACE "$<BUILD_INTERFACE:${CCL_INCLUDE_DIRS}>")
		endif ()
	endif ()
endforeach ()

# Set up project folder strucure
foreach (component IN ITEMS ${ccl_FIND_COMPONENTS})
	set (target ${component})
	if (NOT "${${component}}" STREQUAL "")
		set (target ${${component}})
	endif ()
	if (TARGET ${target})
		get_target_property (dependencies ${target} LINK_LIBRARIES)
		foreach (dependency IN ITEMS ${dependencies})
			if (NOT TARGET ${dependency})
				continue ()
			endif ()
			get_target_property (imported ${dependency} IMPORTED)
			if (NOT imported)
				if ("${dependency}" MATCHES "^cclextras.*$")
					set_target_properties (${dependency} PROPERTIES USE_FOLDERS ON FOLDER "ccl/extras")
				elseif ("${dependency}" MATCHES "^ccl.*$")
					set_target_properties (${dependency} PROPERTIES USE_FOLDERS ON FOLDER "ccl")
				endif ()
			endif ()
		endforeach ()
		
		get_target_property (imported ${target} IMPORTED)
		if (NOT imported)
			if (PROJECT_DEBUG_SUFFIX)
				get_target_property (target_type ${target} TYPE)
				if (target_type STREQUAL STATIC_LIBRARY)
					set_target_properties (${target} PROPERTIES DEBUG_POSTFIX ${PROJECT_DEBUG_SUFFIX})
				endif ()
			endif ()
		
			if ("${target}" MATCHES "^cclextras.*$")
				set_target_properties (${target} PROPERTIES USE_FOLDERS ON FOLDER "ccl/extras")
			else ()
				set_target_properties (${target} PROPERTIES USE_FOLDERS ON FOLDER "ccl")
			endif ()
		endif ()
	endif ()
endforeach ()

get_target_property (imported corelib IMPORTED)
if (NOT imported)
	set_target_properties (corelib PROPERTIES FOLDER "ccl")
endif ()

if (CCL_SYSTEM_INSTALL)
	include (${CMAKE_CURRENT_LIST_DIR}/ccl-install.cmake)
endif ()

# Set result variables
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (ccl
	FOUND_VAR CCL_FOUND
	REQUIRED_VARS CCL_LIBRARIES CCL_INCLUDE_DIRS
	VERSION_VAR CCL_VERSION
	HANDLE_COMPONENTS
)

# make subsequent find_package calls for ccl silent
set (ccl_FIND_QUIETLY ON)
