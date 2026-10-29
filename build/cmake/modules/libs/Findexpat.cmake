include_guard (DIRECTORY)

ccl_find_path (expat_DIR NAMES "lib/expat.h" HINTS ${CCL_SUBMODULES_DIR}/expat/expat)
mark_as_advanced (expat_DIR)

if (NOT TARGET expat-config)
	ccl_add_external_project (expat-config "${expat_DIR}" CONFIGURE_ONLY OPTIONS
		-DEXPAT_BUILD_EXAMPLES=OFF
		-DEXPAT_BUILD_TESTS=OFF
		-DEXPAT_BUILD_TOOLS=OFF
	)

	set_target_properties (expat-config PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")
endif ()

list (APPEND expat_SOURCES
    ${expat_DIR}/lib/expat.h
    ${expat_DIR}/lib/xmlparse.c
    ${expat_DIR}/lib/xmlrole.c
    ${expat_DIR}/lib/xmlrole.h
    ${expat_DIR}/lib/xmltok.c
    ${expat_DIR}/lib/xmltok.h
)
source_group ("libs\\expat" FILES ${expat_SOURCES})

if (NOT TARGET expat)
	ccl_add_library (expat INTERFACE)
	add_dependencies (expat expat-config)

	target_sources (expat INTERFACE ${expat_SOURCES})
	target_include_directories (expat INTERFACE ${expat-config_CONFIG_DIR} ${expat_DIR}/lib)
	target_compile_definitions (expat INTERFACE XML_STATIC XML_UNICODE)
endif ()

set (expat_INCLUDE_DIR ${expat_DIR}/lib)
set (expat_LIBRARY expat)

if (expat_DIR AND EXISTS "${expat_DIR}/win32/expat.iss")
    file (STRINGS "${expat_DIR}/win32/expat.iss" line REGEX "^#define expatVer \".*\"$")
	string (REGEX REPLACE "^#define expatVer \"(.*)\"$" "\\1" expat_VERSION "${line}")
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (expat
	REQUIRED_VARS expat_LIBRARY
	VERSION_VAR expat_VERSION
)
