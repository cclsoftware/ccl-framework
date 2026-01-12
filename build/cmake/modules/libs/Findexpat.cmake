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

list (APPEND expat_sources
    ${expat_DIR}/lib/expat.h
    ${expat_DIR}/lib/xmlparse.c
    ${expat_DIR}/lib/xmlrole.c
    ${expat_DIR}/lib/xmltok.c
)
source_group ("libs\\expat" FILES ${expat_sources})

if (NOT TARGET expat)
	ccl_add_library (expat INTERFACE)
	add_dependencies (expat expat-config)

	target_sources (expat INTERFACE ${expat_sources})
	target_include_directories (expat INTERFACE ${expat-config_CONFIG_DIR} ${expat_DIR}/lib)
	target_compile_definitions (expat INTERFACE XML_STATIC XML_UNICODE)
endif ()

set (expat_INCLUDE_DIR ${expat_DIR}/lib)
set (expat_LIBRARY expat)
