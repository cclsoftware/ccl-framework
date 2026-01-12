#************************************************************************************************
#
# JavaScript Service CMake Configuration
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
# Filename    : jsengine-config.cmake
# Description : CMake target for JavaScript service
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclbase ccltext cclsystem)
find_package (spidermonkey)

if (NOT spidermonkey_FOUND)
	message (NOTICE "Skipping jsengine service. Missing dependency: spidermonkey")
	return ()
endif ()

ccl_find_path (jsengine_DIR NAMES "source/jsengine.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "JavaScript service directory")

if (TARGET jsengine)
	return ()
endif ()

# Add target
ccl_add_plugin_library (jsengine
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
ccl_add_test (jsengine)

set_target_properties (jsengine PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND jsengine_source_files
	${jsengine_DIR}/source/jsclassregistry.cpp
	${jsengine_DIR}/source/jsclassregistry.h
	${jsengine_DIR}/source/jscontext.cpp
	${jsengine_DIR}/source/jscontext.h
	${jsengine_DIR}/source/jscrossthread.cpp
	${jsengine_DIR}/source/jscrossthread.h
	${jsengine_DIR}/source/jsdebugcontext.cpp
	${jsengine_DIR}/source/jsdebugcontext.h
	${jsengine_DIR}/source/jsengine.cpp
	${jsengine_DIR}/source/jsengine.h
	${jsengine_DIR}/source/jsinclude.h
	${jsengine_DIR}/source/jstest.cpp
	${jsengine_DIR}/source/jstest.h
	${jsengine_DIR}/source/plugmain.cpp
	${jsengine_DIR}/source/plugversion.h
)

list (APPEND jsengine_ccl_sources
	${CCL_DIR}/main/cclmodmain.empty.cpp
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/public/plugins/classfactory.cpp
	${CCL_DIR}/public/plugins/serviceplugin.cpp
	${CCL_DIR}/public/plugins/stubobject.cpp
)

list (APPEND jsengine_resource_files
	${jsengine_DIR}/debug/daphandler.js
)

source_group (TREE ${jsengine_DIR}/source PREFIX "source" FILES ${jsengine_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group (TREE ${jsengine_DIR}/debug PREFIX "resource" FILES ${jsengine_resource_files})

list (APPEND jsengine_sources ${jsengine_source_files} ${jsengine_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (jsengine ${jsengine_resources})
ccl_add_resources (jsengine ${jsengine_resource_files})
target_sources (jsengine PRIVATE ${jsengine_sources})
target_compile_definitions (jsengine PRIVATE "STATIC_JS_API" "NIGHTLY_BUILD")
target_link_libraries (jsengine PRIVATE cclbase ccltext cclsystem ${spidermonkey_LIBRARIES})

if (CCL_SYSTEM_INSTALL)
	install (TARGETS jsengine 
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
	if (WIN32)
		install (FILES $<TARGET_FILE_DIR:jsengine>/jsengine.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS jsengine LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
							  FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()

if (VENDOR_ENABLE_DEBUG_SPIDERMONKEY)
	add_compile_definitions (ENABLE_DEBUG_SPIDERMONKEY)
endif ()
