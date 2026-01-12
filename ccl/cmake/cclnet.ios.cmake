ccl_list_append_once (cclnet_cocoa_sources
	${CCL_DIR}/platform/cocoa/net/transfersession.cocoa.h
	${CCL_DIR}/platform/cocoa/net/transfersession.cocoa.mm
)

ccl_list_append_once (cclnet_sources
	${cclnet_cocoa_sources}
)

source_group (TREE ${CCL_DIR}/platform/cocoa PREFIX "source" FILES ${cclnet_cocoa_sources})

find_library (SECURITY_LIBRARY Security REQUIRED)

ccl_list_append_once (cclnet_apple_frameworks
	${SECURITY_LIBRARY}
)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclnet} PUBLIC ${cclnet_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${cclnet_apple_frameworks})
