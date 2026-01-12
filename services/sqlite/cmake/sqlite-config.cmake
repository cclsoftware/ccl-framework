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
# Filename    : sqlite-config.cmake
# Description : CMake target for SQLite service
#
#************************************************************************************************

find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)
find_package (sqlitelib)

# Find source directories
ccl_find_path (sqlite_sourcedir NAMES "plugversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/../source" DOC "SQLite service source directory")
mark_as_advanced (sqlite_sourcedir)

ccl_check_target_exists (sqlite target_exists)
if (target_exists)
	return ()
endif ()

# Add target
ccl_add_plugin_library (sqlite
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
ccl_set_product_name (${sqlite_target} ${sqlite_ID})
target_link_ccl_framework (${sqlite_target} PRIVATE cclbase cclsystem ccltext)
ccl_add_test (${sqlite_target})

set_target_properties (${sqlite_target} PROPERTIES USE_FOLDERS ON FOLDER "services/ccl/${CCL_ISOLATION_POSTFIX}")

list (APPEND sqlite_resources
)

list (APPEND sqlite_source_files
    ${sqlite_sourcedir}/plugmain.cpp
    ${sqlite_sourcedir}/plugversion.h
    ${sqlite_sourcedir}/sqliteconnection.cpp
    ${sqlite_sourcedir}/sqliteconnection.h
    ${sqlite_sourcedir}/sqliteengine.cpp
    ${sqlite_sourcedir}/sqliteengine.h
    ${sqlite_sourcedir}/sqliteerror.cpp
    ${sqlite_sourcedir}/sqliteerror.h
    ${sqlite_sourcedir}/sqlitestatement.cpp
    ${sqlite_sourcedir}/sqlitestatement.h
)

list (APPEND sqlite_test_sources
    ${sqlite_sourcedir}/sqlitetest.cpp
    ${sqlite_sourcedir}/sqlitetest.h
)

list (APPEND sqlite_ccl_sources
    ${CCL_DIR}/main/cclmodmain.cpp
    ${CCL_DIR}/main/cclmodmain.empty.cpp
    ${CCL_DIR}/public/plugins/classfactory.cpp
    ${CCL_DIR}/main/cclmodmain.h
)

source_group ("source" FILES ${sqlite_source_files})
source_group ("source\\test" FILES ${sqlite_test_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND sqlite_sources ${sqlite_source_files} ${sqlite_test_sources} ${sqlite_lib_sources} ${sqlite_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (${sqlite_target} ${sqlite_resources})
target_sources (${sqlite_target} PRIVATE ${sqlite_sources})
target_link_libraries (${sqlite_target} PRIVATE ${SQLITE_LIBRARY})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS ${sqlite_target} 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:${sqlite_target}>/${sqlite_target}.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS ${sqlite_target} LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
                                      FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
