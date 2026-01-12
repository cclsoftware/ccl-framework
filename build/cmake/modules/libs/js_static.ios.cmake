if(${VENDOR_SIMULATOR})
	set_target_properties (js_static PROPERTIES
		IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-iossim-universal-release/libjs_static.a"
		IMPORTED_LOCATION "${spidermonkey_DIR}/obj-iossim-universal-release/libjs_static.a"
	)
else()
	set_target_properties (js_static PROPERTIES
		IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-ios-aarch64-release/libjs_static.a"
		IMPORTED_LOCATION "${spidermonkey_DIR}/obj-ios-aarch64-release/libjs_static.a"
	)
endif()

list (APPEND spidermonkey_LIBRARIES js_static)
