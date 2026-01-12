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
# Filename    : cclextras-gadgets.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_gadgets_public_headers
	${CCL_DIR}/public/extras/gadgets.h
)

ccl_list_append_once (cclextras_gadgets_headers
	${CCL_DIR}/extras/gadgets/gadgetdashboard.h
	${CCL_DIR}/extras/gadgets/gadgetmanager.h
)

ccl_list_append_once (cclextras_gadgets_sources
	${CCL_DIR}/extras/gadgets/gadgetdashboard.cpp
	${CCL_DIR}/extras/gadgets/gadgetmanager.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

# Add targets
if (NOT TARGET cclextras-gadgets)
	ccl_add_library (cclextras-gadgets STATIC)
		
	source_group ("source" FILES ${cclextras_gadgets_sources} ${cclextras_gadgets_headers})
	source_group ("public" FILES ${cclextras_public_sources} ${cclextras_gadgets_public_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (cclextras-gadgets PRIVATE ${cclextras_gadgets_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (cclextras-gadgets INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_gadgets_headers} ${cclextras_gadgets_public_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS cclextras-gadgets EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										   ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:cclextras-gadgets>/cclextras-gadgets$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
