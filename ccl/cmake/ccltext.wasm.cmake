
ccl_list_append_once (ccltext_wasm_sources
	${CCL_DIR}/platform/wasm/text/unicodestring.wasm.cpp
	${CCL_DIR}/platform/wasm/text/unicodestring.wasm.h
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.cpp
	${CCL_SUBMODULES_DIR}/debug_malloc/debug_malloc.h
)

ccl_list_append_once (ccltext_source_files 
	${CCL_DIR}/text/strings/unicodestringbuffer.h
	${CCL_DIR}/text/strings/unicodestringbuffer.impl.h
	${CCL_DIR}/text/strings/unicodestringbuffer.cpp
)

ccl_list_append_once (ccltext_sources ${ccltext_wasm_sources})

source_group ("source" FILES ${ccltext_wasm_sources})

set_source_files_properties (${CCL_DIR}/text/xml/xmlparser.cpp PROPERTIES COMPILE_OPTIONS "-Wno-shorten-64-to-32")

# There should be only one definition of replacement new/delete operators in a process.
# If ccltext is part of a project, disable replacement new/delete operators in core-based libraries.
set_source_files_properties (${corelib_malloc_sources} PROPERTIES COMPILE_DEFINITIONS "CORE_DISABLE_NEW_OPERATOR=1")

ccl_list_append_once (VENDOR_EMSCRIPTEN_EXPORTED_FUNCTIONS _islower)