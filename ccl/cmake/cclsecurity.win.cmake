ccl_list_append_once (cclsecurity_source_files
	${CCL_DIR}/platform/win/security/credentialmanager.win.cpp
)

ccl_list_append_once (cclsecurity_sources
	${CCL_DIR}/platform/win/system/cclcom.cpp
	${CCL_DIR}/platform/win/system/cclcom.h
	${CCL_DIR}/platform/win/system/management.cpp
	${CCL_DIR}/platform/win/system/management.h
	${CCL_DIR}/platform/win/system/registry.cpp
	${CCL_DIR}/platform/win/system/registry.h
)

source_group ("source" FILES ${cclsecurity_source_files})

source_group ("source\\win32" FILES
	${CCL_DIR}/platform/win/system/cclcom.cpp
	${CCL_DIR}/platform/win/system/cclcom.h
	${CCL_DIR}/platform/win/system/management.cpp
	${CCL_DIR}/platform/win/system/management.h
	${CCL_DIR}/platform/win/system/registry.cpp
	${CCL_DIR}/platform/win/system/registry.h
)

set_source_files_properties (
    ${CCL_DIR}/security/cryptoservice.cpp
    ${CCL_DIR}/security/cryptoppglue.cpp
    PROPERTIES COMPILE_FLAGS "/GR"
)
