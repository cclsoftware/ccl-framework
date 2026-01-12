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
# Filename    : xdgportal.cmake
# Description : CCL GUI Integration using XDG Portal
#
#************************************************************************************************

set (cclgui-xdgportal "cclgui-xdgportal")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclgui-xdgportal ".${CCL_ISOLATION_POSTFIX}")
endif ()

option (CCL_BUILD_XDGPORTAL_INTEGRATION "xdg portal integration for cclgui" ON)
if (${CCL_BUILD_XDGPORTAL_INTEGRATION} AND NOT TARGET ${cclgui-xdgportal})
	find_package (ccl REQUIRED COMPONENTS cclbase)

	ccl_add_library (cclgui-xdgportal SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}"
		SUBDIR "PlatformIntegration/${CCL_PACKAGE_DOMAIN}cclgui"
		VERSION_FILE ${CMAKE_CURRENT_LIST_DIR}/version.h
		VERSION_PREFIX PLUG
	)
	set_target_properties (${cclgui-xdgportal} PROPERTIES FOLDER "ccl")
    
	ccl_list_append_once (xdgportal_ccl_sources
		${CCL_DIR}/main/cclmodmain.cpp
		${CCL_DIR}/main/cclmodmain.empty.cpp
		${CCL_DIR}/platform/linux/interfaces/linuxiids.cpp
	)

	ccl_list_append_once (xdgportal_source_files
		${CMAKE_CURRENT_LIST_DIR}/xdgportal.cpp
		${CMAKE_CURRENT_LIST_DIR}/version.h
		${CMAKE_CURRENT_LIST_FILE}
	)

	ccl_list_append_once (xdgportal_sources
		${xdgportal_ccl_sources}
		${xdgportal_source_files}	
	)

	source_group ("source" FILES ${xdgportal_source_files})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	target_sources (${cclgui-xdgportal} PRIVATE ${xdgportal_sources})
    
	target_link_libraries (${cclgui-xdgportal} PRIVATE ${cclbase})
	target_include_directories (${cclgui-xdgportal} PRIVATE "${CCL_INCLUDE_DIRS}")
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclgui-xdgportal} LIBRARY DESTINATION "${CCL_BINARY_DESTINATION}/PlatformIntegration/${CCL_PACKAGE_DOMAIN}cclgui/")
	else ()
		install (TARGETS ${cclgui-xdgportal} LIBRARY DESTINATION "${VENDOR_PLATFORMINTEGRATION_DIRECTORY}/${CCL_PACKAGE_DOMAIN}cclgui/")
	endif ()

	ccl_use_dbus_interface (${cclgui-xdgportal} org.freedesktop.portal.Request)
	ccl_use_dbus_interface (${cclgui-xdgportal} org.freedesktop.portal.Notification)
	ccl_use_dbus_interface (${cclgui-xdgportal} org.freedesktop.portal.FileChooser)
endif ()
