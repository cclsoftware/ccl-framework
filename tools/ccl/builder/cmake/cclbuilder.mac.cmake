include_guard (DIRECTORY)

list (APPEND cclbuilder_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoaconsolemain.mm
)

if (CCL_SYSTEM_INSTALL)
	set_target_properties (cclbuilder PROPERTIES
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks/$<PLATFORM_ID>"
	)
else()
	set_target_properties (cclbuilder PROPERTIES
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/Frameworks"
	)
endif()

target_compile_definitions (cclbuilder PRIVATE "TEMPLATES_SUBDIRECTORY=\"share\"")

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (cclbuilder PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()