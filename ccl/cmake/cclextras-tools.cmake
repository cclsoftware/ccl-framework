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
# Filename    : cclextras-tools.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_tools_headers
	${CCL_DIR}/extras/tools/argumentparser.h
	${CCL_DIR}/extras/tools/publisher.h
	${CCL_DIR}/extras/tools/repositoryinfo.h
	${CCL_DIR}/extras/tools/testcollectionregistry.h
	${CCL_DIR}/extras/tools/testresult.h
	${CCL_DIR}/extras/tools/testrunner.h
	${CCL_DIR}/extras/tools/toolhelp.h
)

ccl_list_append_once (cclextras_tools_sources
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/publisher.cpp
	${CCL_DIR}/extras/tools/repositoryinfo.cpp
	${CCL_DIR}/extras/tools/testcollectionregistry.cpp
	${CCL_DIR}/extras/tools/testresult.cpp
	${CCL_DIR}/extras/tools/testrunner.cpp
	${CCL_DIR}/extras/tools/toolhelp.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

# Add targets
if (NOT TARGET cclextras-tools)
	ccl_add_library (cclextras-tools STATIC)

	source_group ("source" FILES ${cclextras_tools_sources} ${cclextras_tools_headers})
	source_group ("public" FILES ${cclextras_public_sources})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (cclextras-tools PRIVATE ${cclextras_tools_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (cclextras-tools INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_tools_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS cclextras-tools EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										 ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:cclextras-tools>/cclextras-tools$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
