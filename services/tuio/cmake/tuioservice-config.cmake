#************************************************************************************************
#
# TUIO Service CMake Configuration
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
# Filename    : tuioservice-config.cmake
# Description : CMake target for TUIO service
#
#************************************************************************************************

include_guard (DIRECTORY)

if (TARGET tuioservice)
	return ()
endif ()

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (TUIO)
find_package (skins)

ccl_find_path (tuioservice_DIR NAMES "source/tuioservice.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "TUIO service directory")

# Add target
ccl_add_plugin_library (tuioservice
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
set_target_properties (tuioservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

ccl_add_skin_package (tuioservice ${CMAKE_CURRENT_LIST_DIR}/../skin)

list (APPEND tuioservice_source_files
    ${tuioservice_DIR}/source/plugmain.cpp
	${tuioservice_DIR}/source/plugversion.h
	${tuioservice_DIR}/source/tuioservice.cpp
	${tuioservice_DIR}/source/tuioservice.h
	${tuioservice_DIR}/source/tuiouseroption.cpp
	${tuioservice_DIR}/source/tuiouseroption.h
)

list (APPEND tuioservice_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/gui/touch/touchcollection.cpp
    ${CCL_DIR}/gui/touch/touchcollection.h
)

source_group (TREE ${tuioservice_DIR}/source PREFIX "source" FILES ${tuioservice_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND tuioservice_sources ${tuioservice_source_files} ${tuioservice_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (tuioservice ${tuioservice_resources})
target_sources (tuioservice PRIVATE ${tuioservice_sources})
target_link_libraries (tuioservice PRIVATE cclapp cclbase ccltext cclsystem cclgui ${TUIO_LIBRARIES})
