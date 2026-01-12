#************************************************************************************************
#
# Application Template
# (Application Copyright)
#
# Filename    : CMakeLists.txt
# Description : Application Template
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("Application Template")
set (PROJECT_VENDOR appvendor)

# Find dependencies
include (vendor OPTIONAL)
find_package (ccl(FrameworkVersion) REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclextras-tools)

# Add targets
ccl_add_app (apptemplate
	VENDOR appvendor
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (apptemplate ${apptemplate_NAME})
ccl_set_startup_project (apptemplate)

list (APPEND apptemplate_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/apptemplate.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../resource PREFIX "resource" FILES ${apptemplate_resource_files})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../source PREFIX "source" FILES ${apptemplate_source_files})
source_group ("source\\ccl" FILES ${apptemplate_ccl_sources})

list (APPEND apptemplate_sources 
	${apptemplate_source_files} 
	${apptemplate_ccl_sources}
)

ccl_add_resources (apptemplate ${apptemplate_resources})

target_sources (apptemplate PRIVATE ${apptemplate_sources} ${CMAKE_CURRENT_LIST_FILE})

target_link_libraries (apptemplate PRIVATE 
	cclapp cclbase ccltext cclsystem cclextras-tools
)
