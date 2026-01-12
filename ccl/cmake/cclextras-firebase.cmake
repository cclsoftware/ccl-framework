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
# Filename    : cclextras-firebase.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_firebase_headers
	${CCL_DIR}/extras/firebase/errorcodes.h
	${CCL_DIR}/extras/firebase/iapp.h
	${CCL_DIR}/extras/firebase/iauth.h
	${CCL_DIR}/extras/firebase/ifirebase.h
	${CCL_DIR}/extras/firebase/ifirestore.h
	${CCL_DIR}/extras/firebase/timestamp.h
)

ccl_list_append_once (cclextras_firebase_sources
	${CCL_DIR}/extras/firebase/errorcodes.cpp
	${CCL_DIR}/extras/firebase/firebaseiids.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

set (cclextras-firebase "cclextras-firebase")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-firebase ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-firebase})
	ccl_add_library (cclextras-firebase STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	source_group ("source" FILES ${cclextras_firebase_sources} ${cclextras_firebase_headers})
	source_group ("public" FILES ${cclextras_public_sources})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-firebase} PRIVATE ${cclextras_firebase_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-firebase} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_firebase_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-firebase} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
											ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclextras-firebase}>/${cclextras-firebase}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
