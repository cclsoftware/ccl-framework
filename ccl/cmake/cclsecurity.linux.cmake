
ccl_list_append_once (cclsecurity_source_files
	${CCL_DIR}/platform/shared/host/platformcredentialmanager.cpp
	${CCL_DIR}/platform/shared/host/platformintegration.cpp
)

set_source_files_properties (
	#${CCL_DIR}/security/cryptoservice.cpp
	${CCL_DIR}/security/cryptoppglue.cpp
	PROPERTIES COMPILE_FLAGS "-frtti"
)

include ("${CMAKE_CURRENT_LIST_DIR}/../platform/linux/cclsecurity-secretservice/secretservice.cmake")
