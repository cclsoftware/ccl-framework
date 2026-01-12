if(${VENDOR_SIMULATOR})
	set_target_properties (jsrust PROPERTIES
		IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-iossim-universal-release/libjsrust.a"
		IMPORTED_LOCATION "${spidermonkey_DIR}/obj-iossim-universal-release/libjsrust.a"
	)
else()
	set_target_properties (jsrust PROPERTIES
		IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-ios-aarch64-release/libjsrust.a"
		IMPORTED_LOCATION "${spidermonkey_DIR}/obj-ios-aarch64-release/libjsrust.a"
	)
endif()

list (APPEND spidermonkey_LIBRARIES jsrust)
