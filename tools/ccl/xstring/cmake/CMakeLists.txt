#************************************************************************************************
#
# CCL String Extractor
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
# Description : CCL String Extractor
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (xstring)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)

# Add targets
ccl_add_app (xstring
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_startup_project (xstring)
ccl_set_product_name (xstring "cclxstring")

if (CCL_SYSTEM_INSTALL)
	install (TARGETS xstring RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							 BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS xstring RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
							 BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

# Create binary model
ccl_makebin (
	INPUT "${CMAKE_CURRENT_LIST_DIR}/../models/builtins.json"
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bincpp/builtinmodels.cpp"
	PREFIX BuiltInModels
)

# Add source files
list (APPEND xstring_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/pofilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/pofilter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xlffilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xlffilter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringextractor.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringextractor.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringfilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringfilter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringmain.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringmodehandler.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringmodehandler.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringmodel.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringmodel.h
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringparser.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/xstringparser.h
)

list (APPEND xstring_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
)

list (APPEND xstring_bincpp_files
	"${CMAKE_CURRENT_BINARY_DIR}/bincpp/builtinmodels.cpp"
)

source_group ("source" FILES ${xstring_source_files})
source_group ("source/ccl" FILES ${xstring_ccl_sources})
source_group ("bincpp" FILES ${xstring_bincpp_files})

list (APPEND xstring_sources
	${xstring_source_files}
	${xstring_ccl_sources}
	${xstring_bincpp_files}
)

# Required for version information.
ccl_add_resources (xstring ${xstring_resources})

ccl_add_resources (xstring ${xstring_resources})
target_sources (xstring PRIVATE ${xstring_sources})
target_link_libraries (xstring PRIVATE ${CCL_LIBRARIES})
