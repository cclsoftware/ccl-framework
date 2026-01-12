include_guard (DIRECTORY)

ccl_list_append_once (cclextras_stores_platform_sources
	${CCL_DIR}/extras/stores/platform/cocoa/applestorereceipt.cocoa.h
	${CCL_DIR}/extras/stores/platform/cocoa/applestorereceipt.cocoa.mm
	${CCL_DIR}/extras/stores/platform/cocoa/storekitmanager.cocoa.mm
)

source_group ("source/platform" FILES ${cclextras_stores_platform_sources})

ccl_list_append_once (cclextras_stores_sources
	${cclextras_stores_platform_sources}
)

find_library (STOREKIT_LIBRARY StoreKit)
find_library (SECURITY_LIBRARY Security)
find_library (IOKIT_LIBRARY IOKit)
find_library (FOUNDATION_LIBRARY Foundation)

ccl_list_append_once (cclextras_apple_frameworks
	${STOREKIT_LIBRARY}
	${SECURITY_LIBRARY}
	${IOKIT_LIBRARY}
	${FOUNDATION_LIBRARY}
)

target_link_libraries (cclextras-stores PRIVATE ${cclextras_apple_frameworks})
