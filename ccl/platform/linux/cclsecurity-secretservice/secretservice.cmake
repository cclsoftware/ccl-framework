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
# Filename    : secretservice.cmake
# Description : CCL Security Secret Service Integration using D-Bus
#
#************************************************************************************************

set (cclsecurity-secretservice "cclsecurity-secretservice")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclsecurity-secretservice ".${CCL_ISOLATION_POSTFIX}")
endif ()

option (CCL_BUILD_SECRETSERVICE_INTEGRATION "secretservice integration for cclsecurity" ON)
if (${CCL_BUILD_SECRETSERVICE_INTEGRATION} AND NOT TARGET ${cclsecurity-secretservice})
	find_package (ccl REQUIRED COMPONENTS cclbase)

	ccl_add_library (cclsecurity-secretservice SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}"
		SUBDIR "PlatformIntegration/${CCL_PACKAGE_DOMAIN}cclsecurity"
		VERSION_FILE ${CMAKE_CURRENT_LIST_DIR}/version.h
		VERSION_PREFIX PLUG
	)
	set_target_properties (${cclsecurity-secretservice} PROPERTIES FOLDER "ccl")
    
	ccl_list_append_once (secretservice_ccl_sources
		${CCL_DIR}/main/cclmodmain.cpp
		${CCL_DIR}/main/cclmodmain.empty.cpp
		${CCL_DIR}/platform/linux/interfaces/linuxiids.cpp
	)

	ccl_list_append_once (secretservice_source_files
		${CMAKE_CURRENT_LIST_DIR}/secretservice.cpp
		${CMAKE_CURRENT_LIST_DIR}/version.h
		${CMAKE_CURRENT_LIST_FILE}
	)

	ccl_list_append_once (secretservice_sources
		${secretservice_ccl_sources}
		${secretservice_source_files}	
	)

	source_group ("source" FILES ${secretservice_source_files})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	target_sources (${cclsecurity-secretservice} PRIVATE ${secretservice_sources})
    
	target_link_libraries (${cclsecurity-secretservice} PRIVATE ${cclbase})
	target_include_directories (${cclsecurity-secretservice} PRIVATE "${CCL_INCLUDE_DIRS}")
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclsecurity-secretservice} LIBRARY DESTINATION "${CCL_BINARY_DESTINATION}/PlatformIntegration/${CCL_PACKAGE_DOMAIN}cclsecurity/")
	else ()
		install (TARGETS ${cclsecurity-secretservice} LIBRARY DESTINATION "${VENDOR_PLATFORMINTEGRATION_DIRECTORY}/${CCL_PACKAGE_DOMAIN}cclsecurity/")
	endif ()

	ccl_use_dbus_interface (${cclsecurity-secretservice} "${CMAKE_CURRENT_LIST_DIR}/dbus/org.freedesktop.Secret.Service.xml")
	ccl_use_dbus_interface (${cclsecurity-secretservice} "${CMAKE_CURRENT_LIST_DIR}/dbus/org.freedesktop.Secret.Session.xml")
	ccl_use_dbus_interface (${cclsecurity-secretservice} "${CMAKE_CURRENT_LIST_DIR}/dbus/org.freedesktop.Secret.Prompt.xml")
	ccl_use_dbus_interface (${cclsecurity-secretservice} "${CMAKE_CURRENT_LIST_DIR}/dbus/org.freedesktop.Secret.Collection.xml")
	ccl_use_dbus_interface (${cclsecurity-secretservice} "${CMAKE_CURRENT_LIST_DIR}/dbus/org.freedesktop.Secret.Item.xml")
endif ()
