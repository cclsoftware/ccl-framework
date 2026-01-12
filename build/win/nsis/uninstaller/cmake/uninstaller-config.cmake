#************************************************************************************************
#
# Uninstall Wrapper
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
# Filename    : uninstaller-config.cmake
# Description : Uninstall Wrapper
#
#************************************************************************************************

include_guard (GLOBAL)

find_package (ccl REQUIRED COMPONENTS cclbase)

if (TARGET uninstaller)
	return ()
endif ()

ccl_add_app (uninstaller GUI
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (uninstaller ${uninstaller_NAME})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS uninstaller EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
								 RUNTIME DESTINATION "." COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
endif ()

list (APPEND uninstaller_icons
	1	${CMAKE_CURRENT_LIST_DIR}/../packaging/resource/uninstaller.ico
)

list (APPEND uninstaller_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/wrapper.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
)

source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group ("resource" FILES ${uninstaller_resources})
source_group ("source" FILES ${uninstaller_source_files})

list (APPEND uninstaller_sources ${uninstaller_source_files} ${uninstaller_resources})

ccl_add_resources (uninstaller ${uninstaller_resources})
target_sources (uninstaller PRIVATE ${uninstaller_sources} ${CMAKE_CURRENT_LIST_FILE})
target_include_directories (uninstaller PRIVATE ${CCL_INCLUDE_DIRS})

ccl_embed_manifest (uninstaller "${CCL_DIR}/packaging/win/application.manifest")
