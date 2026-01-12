
ccl_list_append_once (ccltext_cocoa_sources
	${CCL_DIR}/platform/cocoa/text/unicodestring.cocoa.h
	${CCL_DIR}/platform/cocoa/text/unicodestring.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/debug.cocoa.mm
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.h
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.cpp
)

ccl_list_append_once (ccltext_sources ${ccltext_cocoa_sources})

source_group ("source" FILES ${ccltext_cocoa_sources})

set_source_files_properties (${CCL_DIR}/text/xml/xmlparser.cpp PROPERTIES COMPILE_FLAGS "-Wno-shorten-64-to-32")

find_library (COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
find_library (CORESERVICES_LIBRARY CoreServices REQUIRED)
find_library (FOUNDATION_LIBRARY Foundation REQUIRED)

ccl_list_append_once (ccltext_apple_frameworks
	${COREFOUNDATION_LIBRARY}
	${CORESERVICES_LIBRARY}
	${FOUNDATION_LIBRARY}
)

set_target_properties (${ccltext} PROPERTIES
	FRAMEWORK TRUE
)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${ccltext} PUBLIC ${ccltext_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${ccltext_apple_frameworks})
