#************************************************************************************************
#
# USB Service CMake Configuration
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
# Filename    : usbservice-config.cmake
# Description : CMake target for USB service
#
#************************************************************************************************

include_guard (DIRECTORY)

if (IOS OR ANDROID)
    return ()
endif ()

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (hidapi)
if (NOT hidapi_FOUND)
	message (NOTICE "Skipping USB service. Missing dependency: hidapi")
	return ()
endif ()

ccl_find_path (usb_DIR NAMES "source/plugversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "USB service directory")

if (NOT TARGET usbservice)
    ccl_add_plugin_library (usbservice
        VENDOR ccl
        VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
        VERSION_PREFIX PLUG
    )
    set_target_properties (usbservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")
endif ()

list (APPEND usbservice_source_files
    ${usb_DIR}/source/plugmain.cpp
	${usb_DIR}/source/plugversion.h
	${usb_DIR}/source/usbhidstatics.cpp
	${usb_DIR}/source/usbhidstatics.h
	${usb_DIR}/source/shared/hidintegration.cpp
	${usb_DIR}/source/shared/hidintegration.h
)

list (APPEND usbservice_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/public/plugins/classfactory.cpp
    ${CCL_DIR}/public/plugins/serviceplugin.cpp
    ${CCL_DIR}/public/devices/iusbhidstatics.h
)

source_group (TREE ${usb_DIR}/source PREFIX "source" FILES ${usbservice_source_files})
source_group ("source/ccl" FILES ${usbservice_ccl_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND usbservice_sources
    ${usbservice_source_files}
    ${usbservice_ccl_sources}
    ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (usbservice ${usbservice_resources})
target_sources (usbservice PRIVATE ${usbservice_sources})
target_link_libraries (usbservice PRIVATE cclapp cclbase ccltext cclsystem cclgui ${hidapi_LIBRARIES})
target_include_directories (usbservice PUBLIC  ${hidapi_INCLUDE_DIRS})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS usbservice
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:usbservice>/usbservice.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS usbservice LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
                                FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
