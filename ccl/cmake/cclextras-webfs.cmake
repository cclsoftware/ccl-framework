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
# Filename    : cclextras-webfs.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_webfs_public_headers
	${CCL_DIR}/public/extras/iwebfilebrowser.h
)

ccl_list_append_once (cclextras_webfs_headers
	${CCL_DIR}/extras/webfs/downloaddraghandler.h
	${CCL_DIR}/extras/webfs/webfileaction.h
	${CCL_DIR}/extras/webfs/webfiledraghandler.h
	${CCL_DIR}/extras/webfs/webfileinfo.h
	${CCL_DIR}/extras/webfs/webfilemethods.h
	${CCL_DIR}/extras/webfs/webfilenodes.h
	${CCL_DIR}/extras/webfs/webfsbrowsercomponent.h
	${CCL_DIR}/extras/webfs/webfsbrowserextension.h
)

ccl_list_append_once (cclextras_webfs_sources
	${CCL_DIR}/extras/webfs/downloaddraghandler.cpp
	${CCL_DIR}/extras/webfs/webfileaction.cpp
	${CCL_DIR}/extras/webfs/webfiledraghandler.cpp
	${CCL_DIR}/extras/webfs/webfileinfo.cpp
	${CCL_DIR}/extras/webfs/webfilemethods.cpp
	${CCL_DIR}/extras/webfs/webfilenodes.cpp
	${CCL_DIR}/extras/webfs/webfsbrowsercomponent.cpp
	${CCL_DIR}/extras/webfs/webfsbrowserextension.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

set (cclextras-webfs "cclextras-webfs")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-webfs ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-webfs})
	ccl_add_library (cclextras-webfs STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")

	source_group ("source" FILES ${cclextras_webfs_sources} ${cclextras_webfs_headers})
	source_group ("public" FILES ${cclextras_public_sources} ${cclextras_webfs_public_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-webfs} PRIVATE ${cclextras_webfs_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-webfs} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_webfs_headers} ${cclextras_webfs_public_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-webfs} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										 ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclextras-webfs}>/${cclextras-webfs}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
