#************************************************************************************************
#
# CCL Binary to C++ Tool
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
# Description : CCL Binary to C++ Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (makebin)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)

# Add targets
ccl_add_app (makebin 
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (makebin)
ccl_set_product_name (makebin "cclmakebin")

if (CCL_SYSTEM_INSTALL)
	install (TARGETS makebin RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							 BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS makebin RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
						     BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")	
endif ()

list (APPEND makebin_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/makebin.cpp
)

list (APPEND makebin_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
)

source_group ("source" FILES ${makebin_source_files})
source_group ("source/ccl" FILES ${makebin_ccl_sources})

list (APPEND makebin_sources ${makebin_source_files} ${makebin_ccl_sources})

ccl_add_resources (makebin ${makebin_resources})
target_sources (makebin PRIVATE ${makebin_sources})
target_link_libraries (makebin PRIVATE ${CCL_LIBRARIES})
