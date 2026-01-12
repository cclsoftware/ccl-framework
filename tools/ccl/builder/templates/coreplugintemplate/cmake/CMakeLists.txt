#************************************************************************************************
#
# Plug-In Template
# (Plug-In Copyright)
#
# Filename    : CMakeLists.txt
# Description : Plug-In Template
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("Plug-In Template")
set (PROJECT_VENDOR plugvendor)

# Find dependencies
include (vendor OPTIONAL)
find_package (corelib(FrameworkVersion))

# Add targets
ccl_add_core_plugin_library (plugintemplate MODULE
	VENDOR plugvendor
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
ccl_set_product_name (plugintemplate ${plugintemplate_NAME})
ccl_set_startup_project (plugintemplate)

list (APPEND plugintemplate_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/plugintemplate.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../resource PREFIX "resource" FILES ${plugintemplate_resource_files})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../source PREFIX "source" FILES ${plugintemplate_source_files})

list (APPEND plugintemplate_sources 
	${plugintemplate_source_files}
)

ccl_add_resources (plugintemplate ${plugintemplate_resources})

target_sources (plugintemplate PRIVATE ${plugintemplate_sources} ${CMAKE_CURRENT_LIST_FILE})
