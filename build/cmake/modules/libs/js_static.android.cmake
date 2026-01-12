set_target_properties (js_static PROPERTIES
	IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-android-${VENDOR_TARGET_ARCHITECTURE}-release/libjs_static.a"
	IMPORTED_LOCATION "${spidermonkey_DIR}/obj-android-${VENDOR_TARGET_ARCHITECTURE}-release/libjs_static.a"
)

list (APPEND spidermonkey_LIBRARIES js_static)
