#************************************************************************************************
#
# CCL Modeller Tool
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
# Filename    : CMakeLists.txt
# Description : CCL Modeller Tool
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (modeller)

include_guard (GLOBAL)

# Find CCL
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem cclgui)

# Add targets
ccl_add_app (modeller BUNDLE
	VENDOR ccl
	VERSION_FILE "${CCL_VERSION_FILE}"
	VERSION_PREFIX CCL
)
ccl_set_startup_project (modeller)
ccl_set_product_name (modeller "cclmodeller")
target_link_ccl_framework (modeller PRIVATE 
	cclbase ccltext cclsystem cclgui
)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS modeller RUNTIME DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools
							  BUNDLE DESTINATION ${CCL_BINARY_DESTINATION} COMPONENT tools)
elseif (CCL_INSTALL_TOOLS)
	install (TARGETS modeller RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
						      BUNDLE DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()

list (APPEND modeller_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/modellermain.cpp
)

list (APPEND modeller_modelling_sources
	${CCL_DIR}/extras/modeling/classmodel.cpp
	${CCL_DIR}/extras/modeling/classmodel.h
	${CCL_DIR}/extras/modeling/classrepository.cpp
	${CCL_DIR}/extras/modeling/classrepository.h
	${CCL_DIR}/extras/modeling/docscanner.cpp
	${CCL_DIR}/extras/modeling/docscanner.h
	${CCL_DIR}/extras/modeling/modeltool.cpp
	${CCL_DIR}/extras/modeling/modeltool.h
)

list (APPEND modeller_ccl_sources
	${CCL_DIR}/main/cclconsolemain.cpp
	${CCL_DIR}/extras/tools/argumentparser.cpp
	${CCL_DIR}/extras/tools/argumentparser.h
	${CCL_DIR}/extras/tools/toolhelp.cpp
	${CCL_DIR}/extras/tools/toolhelp.h
)

source_group ("source" FILES ${modeller_source_files})
source_group ("source/modelling" FILES ${modeller_modelling_sources})
source_group ("source/ccl" FILES ${modeller_ccl_sources})

list (APPEND modeller_sources
	${modeller_source_files}
	${modeller_modelling_sources} 
	${modeller_ccl_sources}
)

ccl_add_resources (modeller ${modeller_resources})

target_sources (modeller PRIVATE ${modeller_sources})

# Custom target to export & update the framework classmodels
set (skin_elements_path "${CCL_REPOSITORY_ROOT}/classmodels/Skin Elements.classModel")
set (visual_styles_path "${CCL_REPOSITORY_ROOT}/classmodels/Visual Styles.classModel")
cmake_path (NATIVE_PATH skin_elements_path NORMALIZE skin_elements_path)
cmake_path (NATIVE_PATH visual_styles_path NORMALIZE visual_styles_path)
cmake_path (NATIVE_PATH CCL_REPOSITORY_ROOT NORMALIZE native_repository_root)

add_custom_target (classmodelexport
    COMMAND $<TARGET_FILE:modeller> -export "Skin Elements" "${skin_elements_path}"
	COMMAND $<TARGET_FILE:modeller> -export "Visual Styles" "${visual_styles_path}"
	COMMAND $<TARGET_FILE:modeller> -scan "${native_repository_root}" "${skin_elements_path}"
	COMMAND $<TARGET_FILE:modeller> -scan "${native_repository_root}" "${visual_styles_path}"	
    DEPENDS modeller
)
