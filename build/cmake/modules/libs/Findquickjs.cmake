include_guard (DIRECTORY)

ccl_find_path (quickjs_DIR NAMES "quickjs.c" HINTS "${CCL_SUBMODULES_DIR}/quickjs" DOC "QuickJS-ng directory")
mark_as_advanced (quickjs_DIR)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (quickjs
	REQUIRED_VARS quickjs_DIR
)

if (NOT quickjs_FOUND)
	return ()
endif ()

list (APPEND quickjs_options
	-DBUILD_SHARED_LIBS=OFF
	-DQJS_BUILD_EXAMPLES=OFF
	-DQJS_BUILD_WERROR=OFF
)

if(XCODE)
	list (APPEND quickjs_options
		-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}
	)
endif()

if (NOT TARGET quickjs)
	ccl_add_external_project (quickjs "${quickjs_DIR}" OPTIONS ${quickjs_options})
	set_target_properties (quickjs PROPERTIES USE_FOLDERS ON FOLDER ccl/libs)
else ()
	ccl_include_platform_specifics (quickjs)
endif ()

if (NOT TARGET quickjs_library)
	add_library (quickjs_library STATIC IMPORTED GLOBAL)
	if (quickjs_LIBRARY_OUTPUT_DEBUG)
		set_target_properties (quickjs_library PROPERTIES IMPORTED_LOCATION_DEBUG "${quickjs_LIBRARY_OUTPUT_DEBUG}" IMPORTED_LOCATION_RELEASE "${quickjs_LIBRARY_OUTPUT_RELEASE}")
	else ()
		set_target_properties (quickjs_library PROPERTIES IMPORTED_LOCATION "${quickjs_LIBRARY_OUTPUT}")
	endif ()
	target_include_directories (quickjs_library INTERFACE "${quickjs_INCLUDE_DIR}")
	add_dependencies (quickjs_library quickjs)
endif ()

set (QUICKJS_LIBRARY quickjs_library)
