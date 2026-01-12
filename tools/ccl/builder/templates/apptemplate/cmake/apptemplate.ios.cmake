include_guard (DIRECTORY)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoamain.mm
)

list (APPEND apptemplate_bundle_resources
	${CMAKE_CURRENT_LIST_DIR}/../packaging/ios/resource/launchscreen.storyboard
	${CMAKE_CURRENT_LIST_DIR}/../packaging/ios/resource/apptemplate.xcassets
)

list (APPEND apptemplate_resources
	${apptemplate_bundle_resources}
)

list (APPEND apptemplate_sources
	${apptemplate_resources}
)
source_group ("Resources" FILES ${apptemplate_resources})

set (apptemplate_FRAMEWORKS
	ccltext
	cclsystem
	cclgui
)

set_target_properties (apptemplate PROPERTIES
	XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
	RESOURCE "${apptemplate_bundle_resources}"
	XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
)
