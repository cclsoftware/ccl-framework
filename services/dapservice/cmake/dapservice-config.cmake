#************************************************************************************************
#
# DAP Service CMake Configuration
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
# Filename    : dapservice-config.cmake
# Description : CMake target for DAP service
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui cclnet)

ccl_find_path (dapservice_DIR NAMES "source/dapservice.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "DAP service directory")

if (TARGET dapservice)
	return ()
endif ()

# Add target
ccl_add_plugin_library (dapservice
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)

set_target_properties (dapservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND dapservice_source_files
	${dapservice_DIR}/source/dapservice.cpp
	${dapservice_DIR}/source/dapservice.h
	${dapservice_DIR}/source/plugmain.cpp
	${dapservice_DIR}/source/plugversion.h
)

list (APPEND dapservice_ccl_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/public/plugins/classfactory.cpp
	${CCL_DIR}/public/plugins/serviceplugin.cpp
)

source_group (TREE ${dapservice_DIR}/source PREFIX "source" FILES ${dapservice_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND dapservice_sources ${dapservice_source_files} ${dapservice_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (dapservice ${dapservice_resources})
target_sources (dapservice PRIVATE ${dapservice_sources})
target_link_libraries (dapservice PRIVATE cclapp cclbase ccltext cclsystem cclgui cclnet)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS dapservice 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:dapservice>/dapservice.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS dapservice LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
								FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
