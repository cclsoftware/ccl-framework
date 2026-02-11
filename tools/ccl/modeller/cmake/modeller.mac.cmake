include_guard (DIRECTORY)

list (APPEND modeller_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoaconsolemain.mm
)

set (modeller_FRAMEWORKS
	cclgui
	ccltext
	cclsystem
)

if (CCL_SYSTEM_INSTALL)
	set (LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks/$<PLATFORM_ID>")
else()
	set (LD_RUNPATH_SEARCH_PATHS "@loader_path/Frameworks")
endif()

set_target_properties (modeller PROPERTIES
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS ${LD_RUNPATH_SEARCH_PATHS}
)

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (modeller PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()
