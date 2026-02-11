#************************************************************************************************
#
# CCL Script Tool
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
# Description : Script Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (script)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase cclgui ccltext cclsystem)

# CCL services
find_package (jsengine REQUIRED)

# Add targets
if(APPLE)
	ccl_add_app (script GUI 
		VENDOR ccl
		VERSION_FILE "${CCL_VERSION_FILE}"
		VERSION_PREFIX CCL
	)
else ()
	ccl_add_app (script 
		VENDOR ccl
		VERSION_FILE "${CCL_VERSION_FILE}"
		VERSION_PREFIX CCL
	)
endif ()
ccl_set_product_name (script "cclscript")

ccl_set_startup_project (script)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS script RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS script RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
							BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")	
endif ()

list (APPEND script_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/scriptmain.cpp
)

list (APPEND script_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
)

source_group ("source" FILES ${script_source_files})
source_group ("source/ccl" FILES ${script_ccl_sources})

list (APPEND script_sources ${script_source_files} ${script_ccl_sources})

ccl_add_resources (script ${script_resources})
target_sources (script PRIVATE ${script_sources})
target_link_libraries (script PRIVATE ${CCL_LIBRARIES} cclgui)

ccl_add_dependencies (script
	jsengine
)
