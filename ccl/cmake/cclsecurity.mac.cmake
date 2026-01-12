ccl_list_append_once (cclsecurity_source_files
	${CCL_DIR}/platform/cocoa/security/credentialmanager.cocoa.mm
)

ccl_list_append_once (cclsecurity_sources
	${CCL_DIR}/platform/cocoa/security/cryptorcommon.cpp
		${CCL_DIR}/platform/cocoa/security/cryptorcommon.h
)

source_group ("source\\cocoa" FILES
	${CCL_DIR}/platform/cocoa/security/cryptorcommon.cpp
	${CCL_DIR}/platform/cocoa/security/cryptorcommon.h
)

set_source_files_properties (
	${CCL_DIR}/security/cryptoppglue.cpp
	PROPERTIES COMPILE_OPTIONS "-frtti"
)

find_library (IOKIT_LIBRARY IOKit REQUIRED)
find_library (COCOA_LIBRARY Cocoa REQUIRED)
find_library (SECURITY_LIBRARY Security REQUIRED)

ccl_list_append_once (cclsecurity_apple_frameworks
	${IOKIT_LIBRARY}
	${COCOA_LIBRARY}
	${SECURITY_LIBRARY}
)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclsecurity} PRIVATE ${cclsecurity_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${cclsecurity_apple_frameworks})

set_target_properties (${cclsecurity} PROPERTIES
	FRAMEWORK TRUE
)


if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclsecurity} PUBLIC ${cclsecurity_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${cclsecurity_apple_frameworks})
