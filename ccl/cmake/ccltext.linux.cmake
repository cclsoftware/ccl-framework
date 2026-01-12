
ccl_list_append_once (ccltext_linux_sources
	${CCL_DIR}/platform/linux/text/unicodestring.linux.cpp
	${CCL_DIR}/platform/linux/text/unicodestring.linux.h
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.cpp
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.h
)

ccl_list_append_once (ccltext_source_files 
	${CCL_DIR}/text/strings/unicodestringbuffer.h
	${CCL_DIR}/text/strings/unicodestringbuffer.impl.h
	${CCL_DIR}/text/strings/unicodestringbuffer.cpp
)

ccl_list_append_once (ccltext_sources ${ccltext_linux_sources})

source_group ("source" FILES ${ccltext_linux_sources})

set_source_files_properties (${CCL_DIR}/text/xml/xmlparser.cpp PROPERTIES COMPILE_OPTIONS "-Wno-shorten-64-to-32")

option (CCL_ENABLE_LIBUNISTRING "Use libunistring for unicode text functions" ON)
find_library (unistring_library unistring)
if (NOT unistring_library)
	message (WARNING "Missing dependency: libunistring. Building with reduced unicode support.")
	set (CCL_ENABLE_LIBUNISTRING OFF CACHE BOOL "" FORCE)
endif ()

if (CCL_ENABLE_LIBUNISTRING)
	if (NOT ${CCL_STATIC_ONLY})
		target_link_libraries (${ccltext} INTERFACE ${unistring_library})
	endif ()
	ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${unistring_library})
	target_compile_definitions (${ccltext} PRIVATE "CCLTEXT_LIBUNISTRING_ENABLED=1")
else ()
	target_compile_definitions (${ccltext} PRIVATE "CCLTEXT_LIBUNISTRING_ENABLED=0")
endif ()

# On Linux, there should be only one definition of replacement new/delete operators in a process.
# If ccltext is part of a project, disable replacement new/delete operators in core-based libraries.
set_source_files_properties (${corelib_malloc_sources} PROPERTIES COMPILE_DEFINITIONS "CORE_DISABLE_NEW_OPERATOR=1")
