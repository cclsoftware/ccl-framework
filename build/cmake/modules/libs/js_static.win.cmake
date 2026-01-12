if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64ec)")
	set (spidermonkey_subdir "obj-win-x86_64")
else ()
	set (spidermonkey_subdir "obj-win-${VENDOR_TARGET_ARCHITECTURE}")
endif ()

set_target_properties (js_static PROPERTIES
	IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/${spidermonkey_subdir}-debug/js_static.lib"
	IMPORTED_LOCATION_RELEASE "${spidermonkey_DIR}/${spidermonkey_subdir}-release/js_static.lib"
)

target_link_libraries (js_static INTERFACE Bcrypt Dbghelp Userenv Ws2_32 Winmm)

list (APPEND spidermonkey_LIBRARIES js_static)
