#************************************************************************************************
#
# CCL Test Runner
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
# Description : CCL Test Runner
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

# The testrunner can be added using find_package or add_subdirectory
if (TARGET testrunner)
	return ()
endif ()

project (testrunner)
set (PROJECT_VENDOR ccl)

# Find dependencies
include (vendor)

set (corelib_ENABLE_CORETEST ON CACHE BOOL "" FORCE)

find_package (ccl REQUIRED COMPONENTS 
	cclapp
	cclbase
	cclextras-tools
	cclgui
	cclnet
	cclsecurity
	cclsystem
	ccltext
)

# Add application
ccl_add_app (testrunner BUNDLE
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (testrunner "ccltestrunner")
target_link_ccl_framework (testrunner PRIVATE 
	cclapp cclbase ccltext cclgui cclsystem cclsecurity cclnet cclextras-tools
)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS testrunner RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							  	BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS testrunner RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
						      	BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND testrunner_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/junitreporter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/junitreporter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/testreporter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/testreporter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/testresultprinter.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/testresultprinter.h
	${CMAKE_CURRENT_LIST_DIR}/../source/testrunnermain.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/testtool.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/testtool.h
	
	${CMAKE_CURRENT_LIST_DIR}/../test/junitreportertest.cpp
)

list (APPEND testrunner_ccl_sources
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/argumentparser.h
	${CCL_DIR}/main/cclconsolemain.cpp
)

source_group ("source" FILES ${testrunner_source_files})
source_group ("source\\ccl" FILES ${testrunner_ccl_sources})

list (APPEND testrunner_sources ${testrunner_source_files} ${testrunner_ccl_sources})

ccl_add_resources (testrunner ${testrunner_resources})
target_sources (testrunner PRIVATE ${testrunner_sources})
