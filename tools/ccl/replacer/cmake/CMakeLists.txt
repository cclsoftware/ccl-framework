#************************************************************************************************
#
# CCL Replacer
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
# Description : CCL Replacer
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (cclreplacer)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)

# Add targets
ccl_add_app (cclreplacer
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (cclreplacer)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS cclreplacer RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
								 BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS cclreplacer RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
								 BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND cclreplacer_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclreplacer.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/cclreplacer.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclreplacermain.cpp
)

list (APPEND cclreplacer_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/argumentparser.h
)

source_group ("source" FILES ${cclreplacer_source_files})
source_group ("source/ccl" FILES ${cclreplacer_ccl_sources})

list (APPEND cclreplacer_sources ${cclreplacer_source_files} ${cclreplacer_ccl_sources})

ccl_add_resources (cclreplacer ${cclreplacer_resources})
target_sources (cclreplacer PRIVATE ${cclreplacer_sources})
target_link_libraries (cclreplacer PRIVATE ${CCL_LIBRARIES})
