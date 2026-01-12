include_guard (DIRECTORY)

ccl_find_path (webview2_DIR NAMES "build/native/include/WebView2.h" HINTS ${CCL_SUBMODULES_DIR}/webview2)
mark_as_advanced (webview2_DIR)

if (NOT TARGET webview2)
	ccl_add_library (webview2 INTERFACE)

	target_include_directories (webview2 INTERFACE "${webview2_DIR}/build/native/include")

	if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64ec")
		target_link_libraries (webview2 INTERFACE "${webview2_DIR}/build/native/x64/WebView2LoaderStatic.lib")
	else ()
		target_link_libraries (webview2 INTERFACE "${webview2_DIR}/build/native/${VENDOR_PLATFORM_SUBDIR}/WebView2LoaderStatic.lib")
	endif ()
endif ()
