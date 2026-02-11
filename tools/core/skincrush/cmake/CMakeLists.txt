#************************************************************************************************
#
# Skin Crush Tool
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
# Description : Skin Crush Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (skincrush)

include_guard (GLOBAL)

# Find Core
include (vendor)
find_package (corelib)

# Add targets
ccl_add_core_app (skincrush
	VENDOR ccl
	VERSION_FILE "${corelib_VERSION_FILE}"
	VERSION_PREFIX CORE
)
ccl_set_startup_project (skincrush)

if (CCL_INSTALL_TOOLS)
	install (TARGETS skincrush RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
							 BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND skincrush_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/skincrush.cpp
)

source_group ("source" FILES ${skincrush_source_files})

list (APPEND skincrush_sources ${skincrush_source_files})

target_sources (skincrush PRIVATE ${skincrush_sources})
target_link_libraries (skincrush PRIVATE corelib)
