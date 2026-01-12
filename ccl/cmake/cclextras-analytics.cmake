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
# Filename    : cclextras-analytics.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_analytics_headers
	${CCL_DIR}/extras/analytics/analyticsevent.h
	${CCL_DIR}/extras/analytics/apptrackinghandler.h
	${CCL_DIR}/extras/analytics/segmentanalyticsclient.h
)

ccl_list_append_once (cclextras_analytics_sources
	${CCL_DIR}/extras/analytics/analyticsevent.cpp
	${CCL_DIR}/extras/analytics/apptrackinghandler.cpp
	${CCL_DIR}/extras/analytics/segmentanalyticsclient.cpp
)

set (cclextras-analytics "cclextras-analytics")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-analytics ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-analytics})
	ccl_add_library (cclextras-analytics STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	source_group ("source" FILES ${cclextras_analytics_sources} ${cclextras_analytics_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-analytics} PRIVATE ${cclextras_analytics_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-analytics} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_analytics_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-analytics} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
											 ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											 FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											 FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:${cclextras-analytics}>/${cclextras-analytics}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
