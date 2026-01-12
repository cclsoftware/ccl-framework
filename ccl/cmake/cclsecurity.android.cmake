include_guard (DIRECTORY)

ccl_list_append_once (cclsecurity_android_sources
	${CCL_DIR}/platform/android/cclandroidjni.h

	${CCL_DIR}/platform/android/interfaces/androidnativeiids.cpp

	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.cpp
	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.h

	${CCL_DIR}/platform/android/security/credentialmanager.android.cpp
)

ccl_list_append_once (cclsecurity_sources
	${cclsecurity_android_sources}

	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source" FILES ${cclsecurity_android_sources})

set_source_files_properties (
	${CCL_DIR}/security/cryptoppglue.cpp
	PROPERTIES COMPILE_OPTIONS "-frtti;-fexceptions"
)

set_source_files_properties (
	${CCL_DIR}/security/cryptoservice.cpp
	PROPERTIES COMPILE_OPTIONS "-fexceptions"
)
