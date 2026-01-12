if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64ec)")
	set (spidermonkey_subdir "obj-win-x86_64")
else ()
	set (spidermonkey_subdir "obj-win-${VENDOR_TARGET_ARCHITECTURE}")
endif ()

set_target_properties (jsrust PROPERTIES
	IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/${spidermonkey_subdir}-debug/jsrust.lib"
	IMPORTED_LOCATION_RELEASE "${spidermonkey_DIR}/${spidermonkey_subdir}-release/jsrust.lib"
)

target_link_libraries (jsrust INTERFACE ntdll)

list (APPEND spidermonkey_LIBRARIES jsrust)
