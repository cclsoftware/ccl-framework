include_guard (DIRECTORY)

ccl_find_path (cmark_DIR NAMES "src/commonmark.c" HINTS "${CCL_SUBMODULES_DIR}/cmark" DOC "Commonmark cmark directory")
mark_as_advanced (cmark_DIR)

list (APPEND cmark_options
	-DBUILD_SHARED_LIBS=OFF
	-DCMARK_SHARED=OFF
	-DCMARK_STATIC=ON
	-DCMARK_TESTS=OFF
	-DCMARK_UTILS=OFF
	-DBUILD_TESTING=OFF
)

if (NOT TARGET cmark)
	ccl_add_external_project (cmark "${cmark_DIR}" OPTIONS ${cmark_options})
	set_target_properties (cmark PROPERTIES USE_FOLDERS ON FOLDER ccl/libs)
else ()
	ccl_include_platform_specifics (cmark)
endif ()

if (NOT TARGET cmark_library)
	add_library (cmark_library STATIC IMPORTED GLOBAL)
	if (cmark_LIBRARY_OUTPUT_DEBUG)
		set_target_properties (cmark_library PROPERTIES IMPORTED_LOCATION_DEBUG "${cmark_LIBRARY_OUTPUT_DEBUG}" IMPORTED_LOCATION_RELEASE "${cmark_LIBRARY_OUTPUT_RELEASE}")
	else ()
		set_target_properties (cmark_library PROPERTIES IMPORTED_LOCATION "${cmark_LIBRARY_OUTPUT}")		
	endif ()
	target_include_directories (cmark_library INTERFACE "${cmark_INCLUDE_DIR}")
	add_dependencies (cmark_library cmark)
endif ()

set (CMARK_LIBRARY cmark_library)
