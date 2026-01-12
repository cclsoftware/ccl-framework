#************************************************************************************************
#
# 3D Model Importer Library
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
# Filename    : modelimporter3d-config.cmake
# Description : CMake Configuration
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (assimp)

ccl_find_path (model3d_DIR NAMES "source/plugversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "Model3D directory")

ccl_add_plugin_library (modelimporter3d
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
set_target_properties (modelimporter3d PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND modelimporter3d_source_files
    ${model3d_DIR}/source/assimpiosystem.h
    ${model3d_DIR}/source/assimpiosystem.cpp
    ${model3d_DIR}/source/modelimporter.h
    ${model3d_DIR}/source/modelimporter.cpp
    ${model3d_DIR}/source/plugmain.cpp
	${model3d_DIR}/source/plugversion.h
)

list (APPEND modelimporter3d_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/public/plugins/classfactory.cpp
)

source_group (TREE ${model3d_DIR}/source PREFIX "source" FILES ${modelimporter3d_source_files})
source_group ("source/ccl" FILES ${modelimporter3d_ccl_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND modelimporter3d_sources
    ${modelimporter3d_source_files}
    ${modelimporter3d_ccl_sources}
    ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (modelimporter3d ${modelimporter3d_resources})
target_sources (modelimporter3d PRIVATE ${modelimporter3d_sources})
target_link_libraries (modelimporter3d PRIVATE cclapp cclbase ccltext cclsystem cclgui ${assimp_LIBRARIES})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS modelimporter3d 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:modelimporter3d>/modelimporter3d.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS modelimporter3d LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
                                     FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
