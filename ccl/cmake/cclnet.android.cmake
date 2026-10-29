include_guard (DIRECTORY)

if (CCLNET_ENABLE_ASYNCDNSRESOLVER)
	ccl_list_append_once (cclnet_android_sources
		${CCL_DIR}/platform/android/cclandroidjni.h
		${CCL_DIR}/platform/android/interfaces/androidnativeiids.cpp
		${CCL_DIR}/platform/android/interfaces/jni/androidcontent.cpp
		${CCL_DIR}/platform/android/interfaces/jni/androidcontent.h
		${CCL_DIR}/platform/android/network/asyncdnsresolver.android.cpp
	)
endif ()

ccl_list_append_once (cclnet_sources
	${cclnet_android_sources}
	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source" FILES ${cclnet_android_sources})
