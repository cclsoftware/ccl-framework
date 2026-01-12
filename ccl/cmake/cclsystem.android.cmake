include_guard (DIRECTORY)

set_source_files_properties (
	${CCL_DIR}/system/plugins/coderesource.cpp
	PROPERTIES COMPILE_OPTIONS "-fexceptions"
)

ccl_list_append_once (cclsystem_android_sources
	${CCL_DIR}/platform/android/cclandroidjni.h

	${CCL_DIR}/platform/android/interfaces/androidnativeiids.cpp
	${CCL_DIR}/platform/android/interfaces/iandroidsystem.h
	${CCL_DIR}/platform/android/interfaces/iframeworkactivity.h

	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.cpp
	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.h

	${CCL_DIR}/platform/android/system/assetfilesystem.cpp
	${CCL_DIR}/platform/android/system/assetfilesystem.h
	${CCL_DIR}/platform/android/system/contentprotocol.cpp
    ${CCL_DIR}/platform/android/system/debug.android.cpp
	${CCL_DIR}/platform/android/system/filemanager.android.cpp
	${CCL_DIR}/platform/android/system/filemanager.android.h
	${CCL_DIR}/platform/android/system/localemanager.android.cpp
    ${CCL_DIR}/platform/android/system/mediathreadservice.android.cpp
    ${CCL_DIR}/platform/android/system/nativefilesystem.android.cpp
    ${CCL_DIR}/platform/android/system/nativefilesystem.android.h
    ${CCL_DIR}/platform/android/system/resourceloader.android.cpp
    ${CCL_DIR}/platform/android/system/safetymanager.android.cpp
	${CCL_DIR}/platform/android/system/streamwrapper.cpp
    ${CCL_DIR}/platform/android/system/system.android.cpp
    ${CCL_DIR}/platform/android/system/system.android.h
)

ccl_list_append_once (cclsystem_android_java_sources
	${CCL_DIR}/platform/android/system/java/OutputStreamWrapper.java
)

ccl_list_append_once (cclsystem_posix_sources
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.h
    ${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.cpp
)

ccl_list_append_once (cclsystem_sources
	${cclsystem_android_sources}
	${cclsystem_android_java_sources}
	${cclsystem_posix_sources}

	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source/platform" FILES ${cclsystem_android_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source/platform" FILES ${cclsystem_android_java_sources})
source_group (TREE ${CCL_DIR}/platform/shared/posix PREFIX "source/platform" FILES ${cclsystem_posix_sources})

ccl_add_aar_project (${cclsystem} NAMESPACE "dev.ccl.cclsystem" DEPENDS corelib)
ccl_install_aar (${cclsystem} COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})

ccl_add_java_sourcedir (${cclsystem} "${CCL_DIR}/platform/android/system/java")
ccl_add_proguard_file (${cclsystem} "${CCL_DIR}/packaging/android/ccl.proguard")
