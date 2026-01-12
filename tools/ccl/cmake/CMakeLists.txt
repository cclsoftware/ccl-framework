#************************************************************************************************
#
# CCL Tools
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
# Filename    : CMakeLists.txt
# Description : CCL Tools
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (ccltools)
set (PROJECT_VENDOR ccl)

if (NOT CCL_SYSTEM_INSTALL)
	set (PROJECT_APPLICATION_RUNTIME_DIRECTORY ".")
	set (CCL_ENABLE_LIBUNISTRING OFF CACHE BOOL "" FORCE)
	set (CCL_INSTALL_TOOLS ON)
endif ()

include (vendor)

find_package ("corelib" REQUIRED)

add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../builder/cmake builder)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../crypt/cmake crypt)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../generator/cmake generator)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../replacer/cmake replacer)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../makebin/cmake makebin)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../makeguid/cmake makeguid)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../modeller/cmake modeller)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../package/cmake packagetool)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../script/cmake script)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../testrunner/cmake testrunner)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../xstring/cmake xstring)

if (NOT CCL_DISABLE_CPACK)
	# Package target
	set (CPACK_GENERATOR "ZIP")
	set (CPACK_PACKAGE_NAME "CCL Tools")
	set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	set (CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/ArchiveConfig.cmake") # rename output config file to prevent generation of a "package" target, which clashes with our packagetool
	include (CPack)
	add_custom_target (archive COMMAND "${CMAKE_CPACK_COMMAND}" "-C" "$<CONFIGURATION>" "--config" "${CMAKE_BINARY_DIR}/ArchiveConfig.cmake")
endif ()
