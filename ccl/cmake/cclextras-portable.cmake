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
# Filename    : cclextras-portable.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_portable_headers
	${CCL_DIR}/extras/portable/gameenvironment.h
	${CCL_DIR}/extras/portable/resourcepackage.h
)

ccl_list_append_once (cclextras_portable_sources
	${CCL_DIR}/extras/portable/gameenvironment.cpp
)

set (cclextras-portable "cclextras-portable")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-portable ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-portable})
	ccl_add_library (cclextras-portable STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	source_group ("source" FILES ${cclextras_portable_sources} ${cclextras_portable_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-portable} PRIVATE ${cclextras_portable_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-portable} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_portable_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-portable} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
											ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:${cclextras-portable}>/${cclextras-portable}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
