ccl_list_append_once (cclsecurity_source_files
	${CCL_DIR}/platform/wasm/security/credentialmanager.wasm.cpp
)

source_group ("source" FILES ${cclsecurity_source_files})

set_source_files_properties (
	#${CCL_DIR}/security/cryptoservice.cpp
	${CCL_DIR}/security/cryptoppglue.cpp
	PROPERTIES COMPILE_FLAGS "-frtti"
)
