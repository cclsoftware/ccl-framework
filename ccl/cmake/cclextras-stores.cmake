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
# Filename    : cclextras-stores.cmake
# Description : CCL extras stores
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclsecurity)

# collect source files
ccl_list_append_once (cclextras_stores_headers
	${CCL_DIR}/extras/stores/platformstoremanager.h
	${CCL_DIR}/extras/stores/purchasemodel.h
	${CCL_DIR}/extras/stores/storepurchasehandler.h
)

ccl_list_append_once (cclextras_stores_source_files
	${CCL_DIR}/extras/stores/demostoremanager.cpp
	${CCL_DIR}/extras/stores/platformstoremanager.cpp
	${CCL_DIR}/extras/stores/purchasemodel.cpp
	${CCL_DIR}/extras/stores/storepurchasehandler.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

# Add targets
if (NOT TARGET cclextras-stores)
	ccl_add_library (cclextras-stores STATIC)

	# collect source files for target
	ccl_list_append_once (cclextras_stores_sources
		${cclextras_stores_source_files}
		${cclextras_public_sources}
	)

	source_group ("source" FILES ${cclextras_stores_source_files} ${cclextras_stores_headers})
	source_group ("public" FILES ${cclextras_public_sources})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (cclextras-stores PRIVATE ${cclextras_stores_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (cclextras-stores INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_stores_headers})
	
	target_link_libraries (cclextras-stores PRIVATE cclsecurity)

	if (CCL_SYSTEM_INSTALL)
		install (TARGETS cclextras-stores EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										  ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:cclextras-stores>/cclextras-stores$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
elseif (NOT XCODE)
	ccl_include_platform_specifics (cclextras-stores)
endif ()
