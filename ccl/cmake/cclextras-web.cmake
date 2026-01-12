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
# Filename    : cclextras-web.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_web_headers
	${CCL_DIR}/extras/web/oauth.h
	${CCL_DIR}/extras/web/oauth2.h
	${CCL_DIR}/extras/web/cognito.h
	${CCL_DIR}/extras/web/webutils.h
	${CCL_DIR}/extras/web/webelements.h
	${CCL_DIR}/extras/web/webformdata.h
	${CCL_DIR}/extras/web/webpreset.h
	${CCL_DIR}/extras/web/webprotocol.h
	${CCL_DIR}/extras/web/webxhrclient.h
	${CCL_DIR}/extras/web/webxhroperation.h
	${CCL_DIR}/extras/web/transfermanagerui.h
)

ccl_list_append_once (cclextras_web_sources
	${CCL_DIR}/extras/web/oauth.cpp
	${CCL_DIR}/extras/web/oauth2.cpp
	${CCL_DIR}/extras/web/cognito.cpp
	${CCL_DIR}/extras/web/webutils.cpp
	${CCL_DIR}/extras/web/webelements.cpp
	${CCL_DIR}/extras/web/webformdata.cpp
	${CCL_DIR}/extras/web/webpreset.cpp
	${CCL_DIR}/extras/web/webprotocol.cpp
	${CCL_DIR}/extras/web/webxhrclient.cpp
	${CCL_DIR}/extras/web/transfermanagerui.cpp
)
	
set (cclextras-web "cclextras-web")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-web ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-web})
	ccl_add_library (cclextras-web STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")

	source_group ("source" FILES ${cclextras_web_sources} ${cclextras_web_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-web} PRIVATE ${cclextras_web_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-web} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_web_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-web} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
									   ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:${cclextras-web}>/${cclextras-web}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
