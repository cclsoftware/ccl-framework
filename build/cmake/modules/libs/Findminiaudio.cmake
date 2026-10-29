include_guard (DIRECTORY)

ccl_find_path (miniaudio_DIR NAMES "miniaudio.h" HINTS "${CCL_SUBMODULES_DIR}/miniaudio" DOC "miniaudio directory")
mark_as_advanced (miniaudio_DIR)

list (APPEND miniaudio_sources
	${miniaudio_DIR}/miniaudio.c
	${miniaudio_DIR}/miniaudio.h
)
source_group ("libs\\miniaudio" FILES ${miniaudio_sources})

if (NOT TARGET miniaudio)
	ccl_add_library (miniaudio INTERFACE)
	target_sources (miniaudio INTERFACE ${miniaudio_sources})
	target_include_directories (miniaudio INTERFACE ${miniaudio_DIR})
	target_compile_definitions (miniaudio INTERFACE MA_NO_ENGINE MA_NO_RESOURCE_MANAGER)

	ccl_include_platform_specifics (miniaudio)
endif ()

set (miniaudio_LIBRARY miniaudio)

if (miniaudio_DIR AND EXISTS "${miniaudio_DIR}/miniaudio.h")
	file (STRINGS "${miniaudio_DIR}/miniaudio.h" major_line REGEX "^#define MA_VERSION_MAJOR[ \t]+[0-9]+$")
	file (STRINGS "${miniaudio_DIR}/miniaudio.h" minor_line REGEX "^#define MA_VERSION_MINOR[ \t]+[0-9]+$")
	file (STRINGS "${miniaudio_DIR}/miniaudio.h" revision_line REGEX "^#define MA_VERSION_REVISION[ \t]+[0-9]+$")
	string (REGEX REPLACE "^#define MA_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" miniaudio_VERSION_MAJOR "${major_line}")
	string (REGEX REPLACE "^#define MA_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" miniaudio_VERSION_MINOR "${minor_line}")
	string (REGEX REPLACE "^#define MA_VERSION_REVISION[ \t]+([0-9]+)$" "\\1" miniaudio_VERSION_REVISION "${revision_line}")
	set (miniaudio_VERSION "${miniaudio_VERSION_MAJOR}.${miniaudio_VERSION_MINOR}.${miniaudio_VERSION_REVISION}")
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (miniaudio
	REQUIRED_VARS miniaudio_LIBRARY
	VERSION_VAR miniaudio_VERSION
)
