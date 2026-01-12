#************************************************************************************************
#
# CCL Package Tool
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
# Description : Package Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (cclpackage)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsecurity cclsystem)

# Add targets
ccl_add_app (cclpackage
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (cclpackage)
ccl_set_product_name (cclpackage "cclpackage")

if (CCL_SYSTEM_INSTALL)
	install (TARGETS cclpackage RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
								 BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS cclpackage RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
								 BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND cclpackage_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/package.cpp
)

list (APPEND cclpackage_ccl_sources
	${CCL_DIR}/extras/tools/publisher.cpp
	${CCL_DIR}/extras/tools/publisher.h
	${CCL_DIR}/extras/tools/repositoryinfo.cpp
	${CCL_DIR}/extras/tools/repositoryinfo.h
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/argumentparser.h
	${CCL_DIR}/main/cclconsolemain.cpp
)

source_group ("source" FILES ${cclpackage_source_files})
source_group ("source\\ccl" FILES ${cclpackage_ccl_sources})

list (APPEND cclpackage_sources ${cclpackage_source_files} ${cclpackage_test_sources} ${cclpackage_ccl_sources})

ccl_add_resources (cclpackage ${cclpackage_resources})
target_sources (cclpackage PRIVATE ${cclpackage_sources})
target_link_libraries (cclpackage PRIVATE ${CCL_LIBRARIES})
