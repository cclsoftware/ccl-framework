#************************************************************************************************
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
# Filename    : ccltest.cmake
# Description : CCL unit tests
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclapp)
include (cclgui)
include (cclsecurity)
include (cclnet)

find_package (corelib REQUIRED COMPONENTS coretest)

# collect resource files
ccl_list_append_once (ccltest_resources
	${CCL_DIR}/test/resource/atomtest.xml
)

# collect source files
ccl_list_append_once (ccltest_source_files
	${CCL_DIR}/test/inittest.cpp
	${CCL_DIR}/test/plugversion.h
)

ccl_list_append_once (ccltest_test_sources
	${CCL_DIR}/test/argumentparsertest.cpp
	${CCL_DIR}/test/basetest.cpp
	${CCL_DIR}/test/bitsettest.cpp
	${CCL_DIR}/test/bufferedstreamtest.cpp
	${CCL_DIR}/test/collectionstest.cpp
	${CCL_DIR}/test/coretestsuite.cpp
	${CCL_DIR}/test/coretestsuite.h
	${CCL_DIR}/test/cpptest.cpp
	${CCL_DIR}/test/cryptotest.cpp
	${CCL_DIR}/test/cstringtest.cpp
	${CCL_DIR}/test/graphics3dtest.cpp
	${CCL_DIR}/test/graphicstest.cpp
	${CCL_DIR}/test/guitest.cpp
	${CCL_DIR}/test/layouttest.cpp
	${CCL_DIR}/test/localetest.cpp
	${CCL_DIR}/test/nativefilesystemtest.cpp
	${CCL_DIR}/test/networktest.cpp
	${CCL_DIR}/test/packagefiletest.cpp
	${CCL_DIR}/test/smartptrtest.cpp
	${CCL_DIR}/test/stringtest.cpp
	${CCL_DIR}/test/systemtest.cpp
	${CCL_DIR}/test/textconverttest.cpp
	${CCL_DIR}/test/textlayouttest.cpp
	${CCL_DIR}/test/threadtest.cpp
	${CCL_DIR}/test/zipfiletest.cpp
)

ccl_list_append_once (ccltest_ccl_sources
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
)

source_group ("source" FILES ${ccltest_source_files})
source_group ("source\\test" FILES ${ccltest_test_sources})
source_group ("source\\ccl" FILES ${ccltest_ccl_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

ccl_list_append_once (ccltest_sources ${ccltest_source_files} ${ccltest_test_sources} ${ccltest_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

# Add target
if (NOT ${CCL_STATIC_ONLY} AND NOT TARGET ccltest)
	ccl_add_plugin_library (ccltest 
		VENDOR ccl
		VERSION_FILE "${CCL_DIR}/public/cclversion.h"
		VERSION_PREFIX CCL
	)
	ccl_add_test (ccltest)
		
	if (NOT ${CCL_STATIC_ONLY})
		ccl_add_resources (ccltest ${ccltest_resources})
		target_sources (ccltest PRIVATE ${ccltest_sources})
		target_link_libraries (ccltest PRIVATE cclapp cclbase cclsystem cclsecurity ccltext cclnet cclgui coretest)
	endif ()
else ()
	ccl_include_platform_specifics (ccltest)
endif ()
