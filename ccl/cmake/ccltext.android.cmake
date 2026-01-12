include_guard (DIRECTORY)

set_source_files_properties (
	${CCL_DIR}/text/transform/zlibcompression.cpp
	PROPERTIES COMPILE_FLAGS "-fexceptions"
)

ccl_list_append_once (ccltext_android_sources
    ${CCL_DIR}/platform/android/system/debug.android.cpp
	${CCL_DIR}/platform/android/text/unicodestring.android.cpp
	${CCL_DIR}/platform/android/text/unicodestring.android.h
)

ccl_list_append_once (ccltext_source_files 
	${CCL_DIR}/text/strings/unicodestringbuffer.h
	${CCL_DIR}/text/strings/unicodestringbuffer.impl.h
	${CCL_DIR}/text/strings/unicodestringbuffer.cpp
)

ccl_list_append_once (ccltext_sources
	${ccltext_android_sources}

	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
source_group ("source" FILES ${ccltext_android_sources})

# On Android, there should be only one definition of replacement new/delete operators in a process.
# If ccltext is part of a project, disable replacement new/delete operators in core-based libraries.
set_source_files_properties (${corelib_malloc_sources} PROPERTIES COMPILE_DEFINITIONS "CORE_DISABLE_NEW_OPERATOR=1")