#************************************************************************************************
#
# Firebase Service CMake Configuration
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
# Filename    : firebaseservice-config.cmake
# Description : CMake target for firebase service
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui cclsecurity cclnet cclextras-firebase)

ccl_find_path (firebase_DIR NAMES "source/plugversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "Firebase service directory")

if (TARGET firebaseservice)
	return ()
endif ()

# Add target
ccl_add_plugin_library (firebaseservice
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
set_target_properties (firebaseservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND firebaseservice_source_files
    ${firebase_DIR}/source/plugmain.cpp
	${firebase_DIR}/source/plugversion.h
	${firebase_DIR}/source/restapi/restapp.cpp
	${firebase_DIR}/source/restapi/restapp.h
	${firebase_DIR}/source/restapi/restauth.cpp
	${firebase_DIR}/source/restapi/restauth.h
	${firebase_DIR}/source/restapi/restfirebase.cpp
	${firebase_DIR}/source/restapi/restfirebase.h
	${firebase_DIR}/source/restapi/restfirestore.cpp
	${firebase_DIR}/source/restapi/restfirestore.h
)

list (APPEND firebaseservice_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/public/plugins/classfactory.cpp
)

source_group (TREE ${firebase_DIR}/source PREFIX "source" FILES ${firebaseservice_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND firebaseservice_sources ${firebaseservice_source_files} ${firebaseservice_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (firebaseservice ${firebaseservice_resources})
target_sources (firebaseservice PRIVATE ${firebaseservice_sources})
target_link_libraries (firebaseservice PRIVATE cclapp cclbase ccltext cclsystem cclgui cclsecurity cclnet cclextras-firebase)
