#************************************************************************************************
#
# (Service Template)
# (Service Copyright)
#
# Filename    : CMakeLists.txt
# Description : (Service Template)
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("(Service Template)")
set (PROJECT_VENDOR servicevendor)

# Find dependencies
include (vendor OPTIONAL)

find_package (ccl(FrameworkVersion) REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclsecurity cclnet cclgui)

if (TARGET servicetemplate)
	return ()
endif ()

# Add targets
ccl_extensions_add_library (servicetemplate extensiondir)
ccl_set_vendor (servicetemplate servicevendor)
ccl_read_version (servicetemplate "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h" "PLUG")

ccl_set_product_name (servicetemplate ${servicetemplate_NAME})
ccl_set_startup_project (servicetemplate)

list (APPEND servicetemplate_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/servicetemplate.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/servicetemplate.h
	${CMAKE_CURRENT_LIST_DIR}/../source/plugmain.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h
)

list (APPEND servicetemplate_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group ("source" FILES ${servicetemplate_source_files})

list (APPEND servicetemplate_sources ${servicetemplate_source_files} ${servicetemplate_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (servicetemplate ${servicetemplate_resources})
target_sources (servicetemplate PRIVATE ${servicetemplate_sources} ${CMAKE_CURRENT_LIST_FILE})
target_link_libraries (servicetemplate PRIVATE cclbase ccltext cclsystem cclsecurity cclnet cclgui cclapp)
