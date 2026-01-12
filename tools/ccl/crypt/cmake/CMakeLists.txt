#************************************************************************************************
#
# CCL Crypt Tool
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
# Description : CCL Crypt Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (crypt)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase cclsecurity cclsystem ccltext)

# Add targets
ccl_add_app (crypt 
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (crypt)
ccl_set_product_name (crypt "cclcrypt")

if (CCL_SYSTEM_INSTALL)
	install (TARGETS crypt RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
						   BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS crypt RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
						   BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND crypt_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/crypttool.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/crypttool.h
	${CMAKE_CURRENT_LIST_DIR}/../source/crypttoolmain.cpp
)

list (APPEND crypt_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
)

source_group ("source" FILES ${crypt_source_files})
source_group ("source/ccl" FILES ${crypt_ccl_sources})

list (APPEND crypt_sources ${crypt_source_files} ${crypt_ccl_sources})

ccl_add_resources (crypt ${crypt_resources})
target_sources (crypt PRIVATE ${crypt_sources})
target_link_libraries (crypt PRIVATE ${CCL_LIBRARIES})
