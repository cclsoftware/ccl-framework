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
# Filename    : cclextras-modeling.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_modeling_headers
	${CCL_DIR}/extras/modeling/classmodel.h
	${CCL_DIR}/extras/modeling/classrepository.h
	${CCL_DIR}/extras/modeling/cplusplus.h
	${CCL_DIR}/extras/modeling/docscanner.h
	${CCL_DIR}/extras/modeling/modelbrowser.h
	${CCL_DIR}/extras/modeling/modelinspector.h
	${CCL_DIR}/extras/modeling/modeltool.h
)

ccl_list_append_once (cclextras_modeling_sources
	${CCL_DIR}/extras/modeling/classmodel.cpp
	${CCL_DIR}/extras/modeling/classrepository.cpp
	${CCL_DIR}/extras/modeling/docscanner.cpp
	${CCL_DIR}/extras/modeling/modelbrowser.cpp
	${CCL_DIR}/extras/modeling/modelinspector.cpp
	${CCL_DIR}/extras/modeling/modeltool.cpp
)

set (cclextras-modeling "cclextras-modeling")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-modeling ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-modeling})
	ccl_add_library (cclextras-modeling STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	source_group ("source" FILES ${cclextras_modeling_sources} ${cclextras_modeling_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-modeling} PRIVATE ${cclextras_modeling_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-modeling} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_modeling_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-modeling} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
			ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											   FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:${cclextras-modeling}>/${cclextras-modeling}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
