if (VENDOR_ENABLE_DEBUG_SPIDERMONKEY)
	set (spidermonkey_postfix "-debug")
else ()
	set (spidermonkey_postfix "-release")
endif ()

set_target_properties (js_static PROPERTIES
	IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-mac-universal${spidermonkey_postfix}/libjs_static.a"
	IMPORTED_LOCATION "${spidermonkey_DIR}/obj-mac-universal${spidermonkey_postfix}/libjs_static.a"
)

list (APPEND spidermonkey_LIBRARIES js_static)
