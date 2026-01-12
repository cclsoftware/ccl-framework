
set_target_properties (jsrust PROPERTIES
	IMPORTED_LOCATION_DEBUG "${spidermonkey_DIR}/obj-linux-${VENDOR_TARGET_ARCHITECTURE}-release/libjsrust.a"
	IMPORTED_LOCATION_RELEASE "${spidermonkey_DIR}/obj-linux-${VENDOR_TARGET_ARCHITECTURE}-release/libjsrust.a"
)

list (APPEND spidermonkey_LIBRARIES jsrust)