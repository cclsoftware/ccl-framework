#************************************************************************************************
#
# CCL Generator
# Copyright (c) 2024 CCL Software Licensing GmbH
#
# Filename    : CMakeLists.txt
# Description : CCL Generator
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("cclgenerator")
set (PROJECT_VENDOR ccl)

# Find dependencies
include (vendor OPTIONAL)
find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclextras-tools)

# Add targets
ccl_add_app (cclgenerator
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS cclgenerator RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
						   BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS cclgenerator RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
						   BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

# Create binary model
ccl_makebin (
	INPUT "${CMAKE_CURRENT_LIST_DIR}/../languages/builtins.json"
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bincpp/builtinlanguages.cpp"
	PREFIX BuiltInLanguages
)

ccl_set_startup_project (cclgenerator)

list (APPEND cclgenerator_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/cclgenerator.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclgenerator.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/cclgeneratormodel.h
	${CMAKE_CURRENT_LIST_DIR}/../source/cclgeneratormodel.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/cclgeneratormain.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
)

list (APPEND cclgenerator_modelling_sources
	${CCL_DIR}/extras/modeling/classmodel.cpp
	${CCL_DIR}/extras/modeling/classmodel.h
	${CCL_DIR}/extras/modeling/classrepository.cpp
	${CCL_DIR}/extras/modeling/classrepository.h
	${CCL_DIR}/extras/modeling/cplusplus.h
)

list (APPEND cclgenerator_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
)


list (APPEND cclgenerator_bincpp_files
	"${CMAKE_CURRENT_BINARY_DIR}/bincpp/builtinlanguages.cpp"
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../resource PREFIX "resource" FILES ${cclgenerator_resource_files})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../source PREFIX "source" FILES ${cclgenerator_source_files})
source_group ("source/modelling" FILES ${cclgenerator_modelling_sources})
source_group ("source/ccl" FILES ${cclgenerator_ccl_sources})
source_group ("bincpp" FILES ${cclgenerator_bincpp_files})

list (APPEND cclgenerator_sources 
	${cclgenerator_source_files} 
	${cclgenerator_modelling_sources}
	${cclgenerator_ccl_sources}
	${cclgenerator_bincpp_files}
)

ccl_add_resources (cclgenerator ${cclgenerator_resources})

target_sources (cclgenerator PRIVATE ${cclgenerator_sources} ${CMAKE_CURRENT_LIST_FILE})

target_link_libraries (cclgenerator PRIVATE 
	cclapp cclbase ccltext cclsystem cclextras-tools
)
