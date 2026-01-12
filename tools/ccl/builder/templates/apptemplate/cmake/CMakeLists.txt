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
find_package (ccl(FrameworkVersion) REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui)
find_package (cclspy)
find_package (skins REQUIRED)

# Add targets
ccl_add_app (apptemplate GUI
	VENDOR appvendor
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (apptemplate ${apptemplate_NAME})
ccl_set_startup_project (apptemplate)

ccl_use_skin_packages (apptemplate neutralbase neutraldesign)
ccl_add_skin_package (apptemplate ${CMAKE_CURRENT_LIST_DIR}/../skin)

list (APPEND apptemplate_resource_files
	${CMAKE_CURRENT_LIST_DIR}/../resource/cclgui.config
	${CMAKE_CURRENT_LIST_DIR}/../resource/menubar.xml
)
list (APPEND apptemplate_resources
    ${apptemplate_resource_files}
)

list (APPEND apptemplate_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/apptemplate.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/apptemplate.h
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/main/cclmain.cpp
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
	cclapp cclbase ccltext cclsystem cclgui
)

ccl_add_dependencies (cclspy)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set (REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/(RelPathToRoot)")
	ccl_generate_repo_info ("${REPOSITORY_ROOT}/repo.json")
endif ()