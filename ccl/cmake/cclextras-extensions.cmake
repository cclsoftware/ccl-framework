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
# Filename    : cclextras.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_extensions_public_headers
	${CCL_DIR}/public/extras/ibackupitem.h
	${CCL_DIR}/public/extras/icontentinstaller.h
	${CCL_DIR}/public/extras/iextensionhandler.h
)

ccl_list_append_once (cclextras_extensions_headers
	${CCL_DIR}/extras/extensions/appupdater.h
	${CCL_DIR}/extras/extensions/backupmanager.h
	${CCL_DIR}/extras/extensions/contentinstallengine.h
	${CCL_DIR}/extras/extensions/extensiondescription.h
	${CCL_DIR}/extras/extensions/extensiondraghandler.h
	${CCL_DIR}/extras/extensions/extensionhandler.h
	${CCL_DIR}/extras/extensions/extensionmanagement.h
	${CCL_DIR}/extras/extensions/extensionmanager.h
	${CCL_DIR}/extras/extensions/extensionpropertiesui.h
	${CCL_DIR}/extras/extensions/icontentserver.h
	${CCL_DIR}/extras/extensions/installdata.h
	${CCL_DIR}/extras/extensions/pluginversionprovider.h
	${CCL_DIR}/extras/extensions/zipinstallhandler.h
)

ccl_list_append_once (cclextras_extensions_sources
	${CCL_DIR}/extras/extensions/appupdater.cpp
	${CCL_DIR}/extras/extensions/backupmanager.cpp
	${CCL_DIR}/extras/extensions/contentinstallengine.cpp
	${CCL_DIR}/extras/extensions/extensiondescription.cpp
	${CCL_DIR}/extras/extensions/extensionhandler.cpp
	${CCL_DIR}/extras/extensions/extensionmanagement.cpp
	${CCL_DIR}/extras/extensions/extensionmanager.cpp
	${CCL_DIR}/extras/extensions/extensionpropertiesui.cpp
	${CCL_DIR}/extras/extensions/icontentserver.cpp
	${CCL_DIR}/extras/extensions/installdata.cpp
	${CCL_DIR}/extras/extensions/pluginversionprovider.cpp
	${CCL_DIR}/extras/extensions/zipinstallhandler.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

set (cclextras-extensions "cclextras-extensions")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-extensions ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-extensions})
	ccl_add_library (cclextras-extensions STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	source_group ("source" FILES ${cclextras_extensions_sources} ${cclextras_extensions_headers})
	source_group ("public" FILES ${cclextras_public_sources} ${cclextras_extensions_public_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-extensions} PRIVATE ${cclextras_extensions_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-extensions} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_extensions_headers} ${cclextras_extensions_public_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-extensions} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										   ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclextras-extensions}>/${cclextras-extensions}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
