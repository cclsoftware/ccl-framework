#************************************************************************************************
#
# CCL Builder Tool
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
# Description : CCL Builder Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (cclbuilder)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)

# Add targets
ccl_add_app (cclbuilder
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (cclbuilder)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS cclbuilder RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
								BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
	set (cclbuilder_templates_directory "${CMAKE_INSTALL_PREFIX}/${CCL_SUPPORT_DESTINATION}/templates")
	install (DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../templates" DESTINATION "${CCL_SUPPORT_DESTINATION}")
	
	cmake_path (IS_PREFIX REPOSITORY_ROOT "${cclbuilder_templates_directory}" NORMALIZE is_prefix)
	if (NOT is_prefix)
		cmake_path (NATIVE_PATH cclbuilder_templates_directory NORMALIZE cclbuilder_templates_directory)
		string (REPLACE "\\" "\\\\" cclbuilder_templates_directory "${cclbuilder_templates_directory}")
		target_compile_definitions (cclbuilder PRIVATE TEMPLATES_DIRECTORY=\"${cclbuilder_templates_directory}\")
	endif ()

	set (cclbuilder_identities_directory "${CMAKE_INSTALL_PREFIX}/${CCL_CMAKE_EXPORT_DESTINATION}/identities")
	cmake_path (IS_PREFIX REPOSITORY_ROOT "${cclbuilder_identities_directory}" NORMALIZE is_prefix)
	if (NOT is_prefix)
		cmake_path (NATIVE_PATH cclbuilder_identities_directory NORMALIZE cclbuilder_identities_directory)
		string (REPLACE "\\" "\\\\" cclbuilder_identities_directory "${cclbuilder_identities_directory}")
		target_compile_definitions (cclbuilder PRIVATE IDENTITIES_DIRECTORY=\"${cclbuilder_identities_directory}\")
	endif ()
	
	target_compile_definitions (cclbuilder PRIVATE PREFERRED_CCL_VERSION=\"${CCL_VERSION}\")
elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
	install (TARGETS cclbuilder RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
								BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")	
endif ()

list (APPEND cclbuilder_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclbuilder.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/cclbuilder.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclbuildermain.cpp
)

list (APPEND cclbuilder_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/argumentparser.h
	${CCL_DIR}/extras/tools/repositoryinfo.cpp
	${CCL_DIR}/extras/tools/repositoryinfo.h
)

cmake_path (RELATIVE_PATH CCL_VERSION_FILE BASE_DIRECTORY "${CCL_REPOSITORY_ROOT}" OUTPUT_VARIABLE search_file_path)		
target_compile_definitions (cclbuilder PRIVATE "CCL_SEARCH_FILE_PATH=\"${search_file_path}\"")

cmake_path (RELATIVE_PATH CCL_TEMPLATES_DIR BASE_DIRECTORY "${CCL_REPOSITORY_ROOT}" OUTPUT_VARIABLE relative_templates_directory)
target_compile_definitions (cclbuilder PRIVATE "RELATIVE_TEMPLATES_DIRECTORY=\"${relative_templates_directory}\"")

cmake_path (RELATIVE_PATH CCL_IDENTITIES_DIR BASE_DIRECTORY "${CCL_REPOSITORY_ROOT}" OUTPUT_VARIABLE relative_identities_directory)
target_compile_definitions (cclbuilder PRIVATE "RELATIVE_IDENTITIES_DIRECTORY=\"${relative_identities_directory}\"")

source_group ("source" FILES ${cclbuilder_source_files})
source_group ("source/ccl" FILES ${cclbuilder_ccl_sources})

list (APPEND cclbuilder_sources ${cclbuilder_source_files} ${cclbuilder_ccl_sources})

ccl_add_resources (cclbuilder ${cclbuilder_resources})
target_sources (cclbuilder PRIVATE ${cclbuilder_sources})
target_link_libraries (cclbuilder PRIVATE ${CCL_LIBRARIES})
