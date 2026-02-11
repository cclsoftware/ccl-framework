include_guard (DIRECTORY)

list (APPEND script_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoaconsolemain.mm
)

set (script_PLUGINS
	jsengine
)

if(CCL_SYSTEM_INSTALL)
	set (LD_RUNPATH_SEARCH_PATHS "@loader_path/../../../../Frameworks/$<PLATFORM_ID>")
else()
	set (LD_RUNPATH_SEARCH_PATHS "@loader_path/../../../Frameworks @loader_path/../../../")
endif()

set_target_properties (script PROPERTIES
	RESOURCE "${script_bundle_resources}"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/Info.plist"
	XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/script.entitlements"
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS ${LD_RUNPATH_SEARCH_PATHS}
)

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (script PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()
