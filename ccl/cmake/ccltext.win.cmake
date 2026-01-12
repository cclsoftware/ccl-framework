ccl_list_append_once (ccltext_platform_sources
	${CCL_DIR}/platform/win/system/debug.win.cpp
	${CCL_DIR}/platform/win/text/cclmalloc.win.cpp
	${CCL_DIR}/platform/win/text/unicodestring.win.cpp
	${CCL_DIR}/platform/win/text/unicodestring.win.h
)

ccl_list_append_once (ccltext_source_files
	${CCL_DIR}/text/strings/unicodestringbuffer.cpp
	${CCL_DIR}/text/strings/unicodestringbuffer.h
	${CCL_DIR}/text/strings/unicodestringbuffer.impl.h
)

ccl_list_append_once (ccltext_sources
	${ccltext_platform_sources}
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/ccltext.natstepfilter 
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/ccltext.natvis
)

source_group ("source" FILES
	${CCL_DIR}/platform/win/text/cclmalloc.win.cpp
)
source_group ("source/strings" FILES
	${CCL_DIR}/text/strings/unicodestringbuffer.cpp
	${CCL_DIR}/text/strings/unicodestringbuffer.h
	${CCL_DIR}/text/strings/unicodestringbuffer.impl.h
	${CCL_DIR}/platform/win/text/unicodestring.win.cpp
	${CCL_DIR}/platform/win/text/unicodestring.win.h
)
source_group ("source/system" FILES
	${CCL_DIR}/platform/win/system/debug.win.cpp
)
source_group ("" FILES
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/ccltext.natstepfilter 
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/ccltext.natvis
)
