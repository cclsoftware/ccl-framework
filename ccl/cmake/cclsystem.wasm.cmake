
ccl_list_append_once (cclsystem_wasm_sources
	${CCL_DIR}/platform/wasm/system/debug.wasm.cpp
	${CCL_DIR}/platform/wasm/system/filemanager.wasm.cpp
	${CCL_DIR}/platform/wasm/system/filemanager.wasm.h
	${CCL_DIR}/platform/wasm/system/localemanager.wasm.cpp
	${CCL_DIR}/platform/wasm/system/mediathreadservice.wasm.cpp
	${CCL_DIR}/platform/wasm/system/nativefilesystem.wasm.cpp
	${CCL_DIR}/platform/wasm/system/nativefilesystem.wasm.h
	${CCL_DIR}/platform/wasm/system/resourceloader.wasm.cpp
	${CCL_DIR}/platform/wasm/system/safetymanager.wasm.cpp
	${CCL_DIR}/platform/wasm/system/system.wasm.cpp
)

ccl_list_append_once (cclsystem_platform_sources
	${CCL_DIR}/platform/shared/host/frameworkresourceloader.cpp
	${CCL_DIR}/platform/shared/host/frameworkresourceloader.h
	
	${CCL_DIR}/platform/shared/posix/system/localemanager.posix.h
	${CCL_DIR}/platform/shared/posix/system/localemanager.posix.cpp
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.cpp
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.h
)

ccl_list_append_once (cclsystem_sources
	${cclsystem_wasm_sources}
	${cclsystem_platform_sources}
)

source_group (TREE ${CCL_DIR}/platform/wasm PREFIX "source" FILES ${cclsystem_wasm_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclsystem_platform_sources})

ccl_check_imported (${cclsystem} imported)
if (NOT imported)
	if (NOT ${CCL_STATIC_ONLY})
		target_link_libraries (${cclsystem} PUBLIC ${CMAKE_DL_LIBS} PRIVATE -luuid)
	endif ()
	if (CMAKE_INSTALL_PREFIX)
		target_compile_definitions (${cclsystem} PRIVATE "CCL_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}/\"")
	endif ()
endif ()

string (APPEND VENDOR_EMSCRIPTEN_LINKER_FLAGS " -lidbfs.js")
string (APPEND VENDOR_EMSCRIPTEN_LINKER_FLAGS " --pre-js=${CCL_DIR}/platform/wasm/js/cclsystem.js")
ccl_list_append_once (VENDOR_EMSCRIPTEN_EXPORTED_FUNCTIONS _emscripten_dlopen_promise _emscripten_promise_destroy)
	
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES -luuid)

