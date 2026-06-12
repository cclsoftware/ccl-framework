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
# Filename    : coremacros.ios.cmake
# Description : iOS CMake Macros
#
#************************************************************************************************

find_file (corelib_MAC_MACROS_FILE NAMES "coremacros.mac.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}")

if (EXISTS "${corelib_MAC_MACROS_FILE}")
	include ("${corelib_MAC_MACROS_FILE}")
endif ()

# Configure the project vendor.
# @group ios
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Code Signing
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/signing.cmake)
		include (${CMAKE_CURRENT_SOURCE_DIR}/signing.cmake)
	endif ()

	if (DEFINED FORCE_SIGNING_TEAMID)
		set (APP_SIGNING_TEAMID "${FORCE_SIGNING_TEAMID}")
	else ()
		set (APP_SIGNING_TEAMID "${SIGNING_TEAMID_IOS}")
	endif()
	set (CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APP_SIGNING_TEAMID}")

	# Installation directories (if using CPack), for cross compiling on a macOS host system
	set (VENDOR_LIBRARY_DESTINATION "Frameworks/$<PLATFORM_ID>")
	set (VENDOR_STATIC_LIBRARY_DESTINATION "lib//$<PLATFORM_ID>")
	set (VENDOR_CMAKE_FILES_DESTINATION "Frameworks/cmake/ccl")
	set (VENDOR_BINARY_DESTINATION "bin")
	set (VENDOR_PUBLIC_HEADERS_DESTINATION "include")
	set (VENDOR_APPSUPPORT_DESTINATION "share")
endmacro ()

# Configure the target vendor.
# @group ios
# @param {STRING} vendor Vendor name.
macro (ccl_configure_vendor target vendor)
	if (NOT APP_SIGNING_TEAMID)
		set (APP_SIGNING_TEAMID "${SIGNING_TEAMID_IOS}")
	endif ()

	if (DEFINED FORCE_SIGNING_TEAMID)
		set_target_properties (${target} PROPERTIES
			XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${FORCE_SIGNING_TEAMID}")
	else ()
		set_target_properties (${target} PROPERTIES
			XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APP_SIGNING_TEAMID}")
	endif()

	ccl_configure_entitlements (${target})
endmacro ()