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

ccl_find_path (jsengine_DIR NAMES "source/jsengine.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "JavaScript service directory")

if (TARGET jsengine)
	if (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
		ccl_install_imported (jsengine LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY} FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
	endif ()
	return ()
endif ()

# Add target
ccl_add_plugin_library (jsengine
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)
ccl_add_test (jsengine)

set (JSENGINE_BACKEND_DEFAULT "spidermonkey")
set (JSENGINE_BACKEND "${JSENGINE_BACKEND_DEFAULT}" CACHE STRING "jsengine backend implementation")
set_property (CACHE JSENGINE_BACKEND PROPERTY STRINGS spidermonkey quickjs)

if (JSENGINE_BACKEND STREQUAL "spidermonkey")
	find_package (spidermonkey)
	if (spidermonkey_FOUND)
		set (jsengine_backend_libs ${spidermonkey_LIBRARIES})
	else ()
		message (NOTICE "Missing dependency: spidermonkey. Trying quickjs instead.")
		set (JSENGINE_BACKEND "quickjs")
		set (JSENGINE_BACKEND "quickjs" CACHE STRING "jsengine backend implementation" FORCE)
	endif ()
endif ()

if (JSENGINE_BACKEND STREQUAL "quickjs")
	find_package (quickjs)
	if (NOT quickjs_FOUND)
		message (NOTICE "Skipping jsengine service. Missing dependency: quickjs")
		return ()
	endif ()
	set (jsengine_backend_libs ${QUICKJS_LIBRARY})
endif ()

set_target_properties (jsengine PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

# No jsdebugcontext.quickjs.* here: quickjs-ng ships no built-in debugger/inspector to build one on.
list (APPEND jsengine_quickjs_source_files
	${jsengine_DIR}/source/quickjs/jscontext.quickjs.cpp
	${jsengine_DIR}/source/quickjs/jscontext.quickjs.h
	${jsengine_DIR}/source/quickjs/jscrossthread.quickjs.cpp
	${jsengine_DIR}/source/quickjs/jscrossthread.quickjs.h
	${jsengine_DIR}/source/quickjs/jsengine.quickjs.cpp
	${jsengine_DIR}/source/quickjs/jsengine.quickjs.h
)

list (APPEND jsengine_spidermonkey_source_files
	${jsengine_DIR}/source/spidermonkey/jscontext.spidermonkey.cpp
	${jsengine_DIR}/source/spidermonkey/jscontext.spidermonkey.h
	${jsengine_DIR}/source/spidermonkey/jscrossthread.spidermonkey.cpp
	${jsengine_DIR}/source/spidermonkey/jscrossthread.spidermonkey.h
	${jsengine_DIR}/source/spidermonkey/jsdebugcontext.spidermonkey.cpp
	${jsengine_DIR}/source/spidermonkey/jsdebugcontext.spidermonkey.h
	${jsengine_DIR}/source/spidermonkey/jsengine.spidermonkey.cpp
	${jsengine_DIR}/source/spidermonkey/jsengine.spidermonkey.h
	${jsengine_DIR}/source/spidermonkey/jsinclude.spidermonkey.h
)

list (APPEND jsengine_source_files
	${jsengine_DIR}/source/jsclassregistry.h
	${jsengine_DIR}/source/jscontextbase.cpp
	${jsengine_DIR}/source/jscontextbase.h
	${jsengine_DIR}/source/jsengine.h
	${jsengine_DIR}/source/jsenginebase.cpp
	${jsengine_DIR}/source/jsenginebase.h
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

if (JSENGINE_BACKEND STREQUAL "spidermonkey")
	list (APPEND jsengine_resource_files
		${jsengine_DIR}/debug/daphandler.js
	)
endif ()

source_group (TREE ${jsengine_DIR}/source PREFIX "source" FILES ${jsengine_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
source_group (TREE ${jsengine_DIR}/debug PREFIX "resource" FILES ${jsengine_resource_files})

list (APPEND jsengine_sources ${jsengine_source_files} ${jsengine_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})
if (JSENGINE_BACKEND STREQUAL "quickjs")
	list (APPEND jsengine_sources ${jsengine_quickjs_source_files})
else ()
	list (APPEND jsengine_sources ${jsengine_spidermonkey_source_files})
endif ()

ccl_add_resources (jsengine ${jsengine_resources})
ccl_add_resources (jsengine ${jsengine_resource_files})
target_sources (jsengine PRIVATE ${jsengine_sources})

if (JSENGINE_BACKEND STREQUAL "quickjs")
	target_compile_definitions (jsengine PRIVATE "JSENGINE_USE_QUICKJS=1")
else ()
	target_compile_definitions (jsengine PRIVATE "JSENGINE_USE_QUICKJS=0" "STATIC_JS_API" "NIGHTLY_BUILD")
	if (VENDOR_ENABLE_DEBUG_SPIDERMONKEY)
		add_compile_definitions (ENABLE_DEBUG_SPIDERMONKEY)
	endif ()
endif ()

target_link_libraries (jsengine PRIVATE cclbase ccltext cclsystem ${jsengine_backend_libs})

ccl_export_target (jsengine)
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
	install (TARGETS jsengine LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY} FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
							  FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
