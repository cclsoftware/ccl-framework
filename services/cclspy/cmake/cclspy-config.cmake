#************************************************************************************************
#
# CCL Spy CMake Configuration
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
# Filename    : cclspy-config.cmake
# Description : CMake target for CCL Spy
#
#************************************************************************************************

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (skins)

# Find CCL Spy sources
ccl_find_path (spy_sources NAMES "spyservice.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/../source" DOC "CCL Spy source directory")

ccl_check_target_exists (cclspy target_exists)
if (${target_exists})
	return ()
endif ()

# Add target
ccl_add_plugin_library (cclspy
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
ccl_set_product_name (${cclspy_target} ${cclspy_ID})
target_link_ccl_framework (${cclspy_target} PRIVATE cclapp cclbase ccltext cclsystem cclgui)
set_target_properties (${cclspy_target} PROPERTIES USE_FOLDERS ON FOLDER "services/ccl/${CCL_ISOLATION_POSTFIX}")

ccl_use_skin_packages (${cclspy_target} devtools)
ccl_add_skin_package (${cclspy_target} ${CMAKE_CURRENT_LIST_DIR}/../skin)

configure_file ("${CCL_CLASSMODELS_DIR}/Skin Elements.classModel" "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/rc/cclspy/models/skinelements.classModel" COPYONLY)
configure_file ("${CCL_CLASSMODELS_DIR}/Visual Styles.classModel" "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/rc/cclspy/models/visualstyles.classModel" COPYONLY)

list (APPEND cclspy_resources
	${CMAKE_CURRENT_LIST_DIR}/../resource/commands.xml
	${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/rc/cclspy/models
)

list (APPEND cclspy_source_files
    ${spy_sources}/docbrowser.cpp
    ${spy_sources}/docbrowser.h
    ${spy_sources}/doceditor.cpp
    ${spy_sources}/doceditor.h
    ${spy_sources}/objectinfo.cpp
    ${spy_sources}/objectinfo.h
    ${spy_sources}/objecttablebrowser.cpp
    ${spy_sources}/objecttablebrowser.h
    ${spy_sources}/plugmain.cpp
    ${spy_sources}/plugversion.h
    ${spy_sources}/scene3dproperties.h
    ${spy_sources}/shadowview.cpp
    ${spy_sources}/shadowview.h
    ${spy_sources}/spycomponent.cpp
    ${spy_sources}/spycomponent.h
    ${spy_sources}/spymanager.cpp
    ${spy_sources}/spymanager.h
    ${spy_sources}/spyservice.cpp
    ${spy_sources}/spyservice.h
    ${spy_sources}/styleproperties.h
    ${spy_sources}/threadmonitor.cpp
    ${spy_sources}/threadmonitor.h
    ${spy_sources}/viewclass.cpp
    ${spy_sources}/viewclass.h
    ${spy_sources}/viewclasses.cpp
    ${spy_sources}/viewproperty.cpp
    ${spy_sources}/viewproperty.h
    ${spy_sources}/viewsprite.cpp
    ${spy_sources}/viewsprite.h
    ${spy_sources}/viewtree.cpp
    ${spy_sources}/viewtree.h
)

list (APPEND cclspy_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
    ${CCL_DIR}/public/plugins/classfactory.cpp
    ${CCL_DIR}/public/plugins/serviceplugin.cpp
    ${CCL_DIR}/public/app/documentlistener.cpp
    ${CCL_DIR}/public/gui/framework/designsize.cpp
)

list (APPEND cclspy_modeling_sources
    ${CCL_DIR}/extras/modeling/classmodel.cpp
    ${CCL_DIR}/extras/modeling/classmodel.h
    ${CCL_DIR}/extras/modeling/classrepository.cpp
    ${CCL_DIR}/extras/modeling/classrepository.h
    ${CCL_DIR}/extras/modeling/docscanner.cpp
    ${CCL_DIR}/extras/modeling/docscanner.h
    ${CCL_DIR}/extras/modeling/modelbrowser.cpp
    ${CCL_DIR}/extras/modeling/modelbrowser.h
    ${CCL_DIR}/extras/modeling/modelinspector.cpp
    ${CCL_DIR}/extras/modeling/modelinspector.h
)

source_group ("source" FILES ${cclspy_source_files})
source_group ("source\\modeling" FILES ${cclspy_modeling_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND cclspy_sources ${cclspy_source_files} ${cclspy_modeling_sources} ${cclspy_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (${cclspy_target} ${cclspy_resources})
target_sources (${cclspy_target} PRIVATE ${cclspy_sources})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS ${cclspy_target} 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:${cclspy_target}>/${cclspy_target}.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
endif ()
