ccl_list_append_once (corelib_api_headers
	${corelib_DIR}/platform/android/corediscovery.android.h
	${corelib_DIR}/platform/android/corefilesystem.android.h
	${corelib_DIR}/platform/android/coreinterprocess.android.h
	${corelib_DIR}/platform/android/coresocket.android.h
	${corelib_DIR}/platform/android/coresslcontext.android.h
	${corelib_DIR}/platform/android/corethread.android.h

	${corelib_DIR}/platform/shared/posix/coreatomic.posix.h
	${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.h
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.h
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.h
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.h
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.h
	${corelib_DIR}/platform/shared/posix/coresocket.posix.h
	${corelib_DIR}/platform/shared/posix/corethread.posix.h
	${corelib_DIR}/platform/shared/posix/coretime.posix.h

	${corelib_DIR}/platform/shared/jni/corejniarray.h
	${corelib_DIR}/platform/shared/jni/corejniclass.h
	${corelib_DIR}/platform/shared/jni/corejniclassdefs.h
	${corelib_DIR}/platform/shared/jni/corejnienvironment.h
	${corelib_DIR}/platform/shared/jni/corejnihelper.h
	${corelib_DIR}/platform/shared/jni/corejniobject.h
	${corelib_DIR}/platform/shared/jni/corejniobject.inline.h
	${corelib_DIR}/platform/shared/jni/corejnistring.h
)

ccl_list_append_once (corelib_platform_sources
	${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.cpp
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.cpp
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.cpp
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.cpp
	${corelib_DIR}/platform/shared/posix/coresocket.posix.cpp
	${corelib_DIR}/platform/shared/posix/corethread.posix.cpp

	${corelib_DIR}/platform/shared/jni/corejniclass.cpp
	${corelib_DIR}/platform/shared/jni/corejniclassdefs.cpp
	${corelib_DIR}/platform/shared/jni/corejnihelper.cpp
)

ccl_list_append_once (corelib_android_sources
	${corelib_DIR}/platform/android/corediscovery.android.cpp
	${corelib_DIR}/platform/android/corefilesystem.android.cpp
	${corelib_DIR}/platform/android/coresocket.android.cpp
	${corelib_DIR}/platform/android/coresslcontext.android.cpp
	${corelib_DIR}/platform/android/coreinterprocess.android.cpp
	${corelib_DIR}/platform/android/corethread.android.cpp
)

ccl_list_append_once (corelib_java_sources
    ${corelib_DIR}/platform/android/java/CurrentContext.java
    ${corelib_DIR}/platform/android/java/Logger.java
    ${corelib_DIR}/platform/android/java/NsdAdapter.java
    ${corelib_DIR}/platform/android/java/SSLChannel.java
)

ccl_list_append_once (corelib_android_jnionload_sources
	${corelib_DIR}/platform/shared/jni/corejnionload.cpp
)

source_group (TREE ${corelib_DIR}/platform/shared PREFIX "source\\platform" FILES ${corelib_platform_sources})
source_group (TREE ${corelib_DIR}/platform/android PREFIX "source\\platform\\android" FILES ${corelib_android_sources})
source_group (TREE ${corelib_DIR}/platform/android/java PREFIX "source\\platform\\android\\java" FILES ${corelib_java_sources})

list (APPEND corelib_sources ${corelib_platform_sources} ${corelib_android_sources} ${corelib_java_sources})

find_library (Android_LIBRARY NAMES android)
find_library (Log_LIBRARY NAMES log)
find_library (Math_LIBRARY NAMES m)
target_link_libraries (corelib PUBLIC ${Android_LIBRARY} ${Log_LIBRARY} ${Math_LIBRARY})

if (NOT "${gradle_files_generated}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	ccl_add_gradle_property (corelib "solutionDir" ${CMAKE_CURRENT_BINARY_DIR})
	ccl_add_gradle_property (corelib "repositoryRoot" ${REPOSITORY_ROOT})

	set (vendor_properties ${VENDOR_GRADLE_PROPERTIES})
	while (vendor_properties)
		list (POP_FRONT vendor_properties key value)
		ccl_add_gradle_property (corelib ${key} ${value})
	endwhile ()

	if (NOT TARGET corelib.aar)
		ccl_add_aar_project (corelib NAMESPACE "dev.ccl.core")
		ccl_install_aar (corelib COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()

	ccl_add_java_sourcedir (corelib "${corelib_DIR}/meta/generated/java")
	ccl_add_java_sourcedir (corelib "${corelib_DIR}/platform/android/java")

	set (gradle_files_generated "${CMAKE_CURRENT_BINARY_DIR}")
endif ()
