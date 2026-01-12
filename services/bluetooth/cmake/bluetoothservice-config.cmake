#************************************************************************************************
#
# Bluetooth Service CMake Configuration
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
# Filename    : bluetoothservice-config.cmake
# Description : CMake target for Bluetooth service
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui cclsecurity cclnet )

ccl_find_path (bluetooth_DIR NAMES "source/plugversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "Bluetooth service directory")

if (NOT TARGET bluetoothservice)
    ccl_add_plugin_library (bluetoothservice
        VENDOR ccl
        VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
        VERSION_PREFIX PLUG
    )
    set_target_properties (bluetoothservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")
elseif (NOT XCODE)
	ccl_include_platform_specifics (bluetoothservice)
endif ()

list (APPEND bluetoothservice_source_files
    ${bluetooth_DIR}/source/plugmain.cpp
	${bluetooth_DIR}/source/plugversion.h
	${bluetooth_DIR}/source/bluetoothstatics.cpp
	${bluetooth_DIR}/source/bluetoothstatics.h
)

list (APPEND bluetoothservice_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/public/plugins/classfactory.cpp
    ${CCL_DIR}/public/plugins/serviceplugin.cpp
    ${CCL_DIR}/public/devices/ibluetoothstatics.h
)

source_group (TREE ${bluetooth_DIR}/source PREFIX "source" FILES ${bluetoothservice_source_files})
source_group ("source/ccl" FILES ${bluetoothservice_ccl_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND bluetoothservice_sources
    ${bluetoothservice_source_files}
    ${bluetoothservice_ccl_sources}
    ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (bluetoothservice ${bluetoothservice_resources})
target_sources (bluetoothservice PRIVATE ${bluetoothservice_sources})
target_include_directories (bluetoothservice PRIVATE "${bluetooth_DIR}")
target_link_libraries (bluetoothservice PRIVATE cclapp cclbase ccltext cclsystem cclgui cclsecurity cclnet)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS bluetoothservice 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:bluetoothservice>/bluetoothservice.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS bluetoothservice LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
									  FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
