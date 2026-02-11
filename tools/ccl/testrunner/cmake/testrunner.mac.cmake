include_guard (DIRECTORY)

list (APPEND testrunner_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoaconsolemain.mm
)

set (testrunner_FRAMEWORKS
	cclgui
	cclnet
	ccltext
	cclsystem
	cclsecurity
)

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (testrunner PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()