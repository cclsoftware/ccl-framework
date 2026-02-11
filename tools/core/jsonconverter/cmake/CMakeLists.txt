#************************************************************************************************
#
# JSON Converter
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
# Description : JSON Converter
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (JsonConverter)

include (vendor)
find_package (corelib)

ccl_add_core_app (jsonconverter)

if (CCL_INSTALL_TOOLS)
	install (TARGETS jsonconverter RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
								   BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

# collect source files
list (APPEND jsonconverter_source_files
	${CMAKE_CURRENT_SOURCE_DIR}/../source/main.cpp
)

# set up project folder structure
source_group ("source" FILES ${jsonconverter_source_files})

# collect source files
list (APPEND jsonconverter_sources
	${jsonconverter_source_files}
)

target_sources (jsonconverter PRIVATE ${jsonconverter_sources})
