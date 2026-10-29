#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2026 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
#
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : cclnet.wasm.cmake
# Description : CCL network library - WebAssembly platform
#
#************************************************************************************************

set (CCLNET_ENABLE_INTERNAL_WEBENGINE OFF CACHE BOOL "" Force)
set (CCLNET_ENABLE_INTERNAL_WEBENGINE OFF)
set (CCLNET_ENABLE_ASYNCDNSRESOLVER OFF CACHE BOOL "" Force)
set (CCLNET_ENABLE_ASYNCDNSRESOLVER OFF)

ccl_list_append_once (cclnet_wasm_sources
	${CCL_DIR}/platform/wasm/network/webengine.wasm.cpp
	${CCL_DIR}/platform/wasm/network/webengine.wasm.h
	${CCL_DIR}/platform/wasm/network/httpclient.wasm.cpp
	${CCL_DIR}/platform/wasm/network/httpclient.wasm.h
	${CCL_DIR}/platform/wasm/network/wsclient.wasm.cpp
	${CCL_DIR}/platform/wasm/network/wsclient.wasm.h
)

ccl_list_append_once (cclnet_sources
	${cclnet_wasm_sources}
)

source_group (TREE ${CCL_DIR}/platform/wasm PREFIX "source" FILES ${cclnet_wasm_sources})

# Enable fetch API for HTTP support and WebSocket library
string (APPEND VENDOR_EMSCRIPTEN_LINKER_FLAGS " -lwebsocket.js")
