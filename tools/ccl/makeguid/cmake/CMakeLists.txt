#************************************************************************************************
#
# CCL GUID Generator
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
# Description : CCL GUID Generator
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (makeguid)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase cclgui ccltext cclsystem)

# Add targets
ccl_add_app (makeguid
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (makeguid)
ccl_set_product_name (makeguid "cclmakeguid")

if (CCL_SYSTEM_INSTALL)
	install (TARGETS makeguid RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							  BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS makeguid RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
							  BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND makeguid_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/makeguid.cpp
)

list (APPEND makeguid_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
)

source_group ("source" FILES ${makeguid_source_files})
source_group ("source/ccl" FILES ${makeguid_ccl_sources})

list (APPEND makeguid_sources ${makeguid_source_files} ${makeguid_ccl_sources})

ccl_add_resources (makeguid ${makeguid_resources})
target_sources (makeguid PRIVATE ${makeguid_sources})
target_link_libraries (makeguid PRIVATE ${CCL_LIBRARIES})
