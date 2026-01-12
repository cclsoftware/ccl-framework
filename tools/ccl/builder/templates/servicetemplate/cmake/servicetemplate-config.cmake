#************************************************************************************************
#
# (Service Template) Service CMake Configuration
# (Service Copyright)
#
# Filename    : servicetemplate-config.cmake
# Description : CMake target for (Service Template) service
#
#************************************************************************************************

include_guard (GLOBAL)

find_package (ccl(FrameworkVersion) REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclsecurity cclnet cclgui)

# Find (Service Template) service sources
ccl_find_path (servicetemplate_source_dir NAMES "servicetemplate.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/../source" DOC "(Service Template) service source directory")

if (TARGET servicetemplate)
	return ()
endif ()

# Add target
ccl_add_plugin_library (servicetemplate
	VENDOR servicevendor
	VERSION_FILE "${servicetemplate_source_dir}/plugversion.h"
	VERSION_PREFIX PLUG
)
set_target_properties (servicetemplate PROPERTIES USE_FOLDERS ON FOLDER "services/devices")

list (APPEND servicetemplate_source_files
	${servicetemplate_source_dir}/servicetemplate.cpp
	${servicetemplate_source_dir}/servicetemplate.h
	${servicetemplate_source_dir}/plugmain.cpp
	${servicetemplate_source_dir}/plugversion.h
)

list (APPEND servicetemplate_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.h
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group ("source" FILES ${servicetemplate_source_files})

list (APPEND servicetemplate_sources ${servicetemplate_source_files} ${servicetemplate_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (servicetemplate ${servicetemplate_resources})
target_sources (servicetemplate PRIVATE ${servicetemplate_sources})
target_link_libraries (servicetemplate PRIVATE cclbase ccltext cclsystem cclsecurity cclnet cclgui cclapp)
