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
# Filename    : CMakeLists.txt
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (corelib)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include (vendor)

find_package (corelib)

option (corelib_BUILD_TESTAPP "Build unit test application" OFF)
if (corelib_BUILD_TESTAPP)
	set (corelib_ENABLE_CORETEST ON CACHE BOOL "Mandatory for this application" FORCE)
endif ()
if (corelib_BUILD_TESTAPP)
	project (CoreTest)
	
	find_package (corelib REQUIRED COMPONENTS coretest)

	ccl_add_core_app (coretestapp VENDOR ccl)
	ccl_set_startup_project (coretestapp)
	ccl_set_debug_arguments (coretestapp run)

	list (APPEND coretestapp_sources "${corelib_DIR}/test/coretestmain.cpp")
	source_group ("source" FILES "${corelib_DIR}/test/coretestmain.cpp")

	target_sources (coretestapp PRIVATE ${coretestapp_sources})
	target_include_directories (coretestapp PRIVATE "${corelib_BASEDIR}")
	target_link_libraries (coretestapp PRIVATE corelib coretest coretestbase)
endif ()
