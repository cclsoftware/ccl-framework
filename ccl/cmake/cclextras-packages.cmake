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
# Filename    : cclextras-packages.cmake
# Description : CCL extras libraries
#
#************************************************************************************************

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include (cclextras-extensions)
include (cclextras-web)

# collect source files
ccl_list_append_once (cclextras_packages_public_headers
	${CCL_DIR}/public/extras/icontentpackagemanager.h
)

ccl_list_append_once (cclextras_packages_headers
	${CCL_DIR}/extras/packages/contentpackagemanager.h
	${CCL_DIR}/extras/packages/extensionmanagerpackages.h
	${CCL_DIR}/extras/packages/factorycontentpackages.h
	${CCL_DIR}/extras/packages/installdatapackages.h
	${CCL_DIR}/extras/packages/packagefilter.h
	${CCL_DIR}/extras/packages/packagehandlerregistry.h
	${CCL_DIR}/extras/packages/packageicons.h
	${CCL_DIR}/extras/packages/packagemanager.h
	${CCL_DIR}/extras/packages/packagesorter.h
	${CCL_DIR}/extras/packages/unifiedpackage.h
	${CCL_DIR}/extras/packages/unifiedpackageaction.h
	${CCL_DIR}/extras/packages/unifiedpackageinstaller.h
	${CCL_DIR}/extras/packages/unifiedpackagesource.h
)

ccl_list_append_once (cclextras_packages_sources
	${CCL_DIR}/extras/packages/contentpackagemanager.cpp
	${CCL_DIR}/extras/packages/extensionmanagerpackages.cpp
	${CCL_DIR}/extras/packages/factorycontentpackages.cpp
	${CCL_DIR}/extras/packages/installdatapackages.cpp
	${CCL_DIR}/extras/packages/packagefilter.cpp
	${CCL_DIR}/extras/packages/packagehandlerregistry.cpp
	${CCL_DIR}/extras/packages/packageicons.cpp
	${CCL_DIR}/extras/packages/packagemanager.cpp
	${CCL_DIR}/extras/packages/packagesorter.cpp
	${CCL_DIR}/extras/packages/unifiedpackage.cpp
	${CCL_DIR}/extras/packages/unifiedpackageaction.cpp
	${CCL_DIR}/extras/packages/unifiedpackageinstaller.cpp
	${CCL_DIR}/extras/packages/unifiedpackagesource.cpp
	${CCL_DIR}/extras/packages/usercontentpackages.cpp
)

ccl_list_append_once (cclextras_public_sources
	${CCL_DIR}/public/cclextraiids.cpp
)

set (cclextras-packages "cclextras-packages")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclextras-packages ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclextras-packages})
	ccl_add_library (cclextras-packages STATIC POSTFIX "${CCL_ISOLATION_POSTFIX}")
		
	target_link_libraries (${cclextras-packages} PUBLIC ${cclextras-web} ${cclextras-extensions})

	source_group ("source" FILES ${cclextras_packages_sources} ${cclextras_packages_headers})
	source_group ("public" FILES ${cclextras_public_sources} ${cclextras_packages_public_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (${cclextras-packages} PRIVATE ${cclextras_packages_sources} ${cclextras_public_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclextras-packages} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_packages_headers} ${cclextras_packages_public_headers})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclextras-packages} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										   ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclextras-packages}>/${cclextras-packages}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
