include_guard (DIRECTORY)

ccl_find_path (yoga_DIR NAMES "yoga/Yoga.h" HINTS "${CCL_SUBMODULES_DIR}/yoga" DOC "Yoga directory")
mark_as_advanced (yoga_DIR)

if (NOT TARGET yoga)
    set (skip_install_command ${CMAKE_COMMAND} -E echo "Findyoga.cmake skipping cmake install.")
	ccl_add_external_project (yoga "${yoga_DIR}/yoga" INSTALL_COMMAND "${skip_install_command}")
	set_target_properties (yoga PROPERTIES USE_FOLDERS ON FOLDER ccl/libs)
endif ()

if (NOT TARGET yoga_library)
	add_library (yoga_library STATIC IMPORTED GLOBAL)
	if (yoga_LIBRARY_OUTPUT_DEBUG)
		set_target_properties (yoga_library PROPERTIES IMPORTED_LOCATION_DEBUG "${yoga_LIBRARY_OUTPUT_DEBUG}" IMPORTED_LOCATION_RELEASE "${yoga_LIBRARY_OUTPUT_RELEASE}")
	else ()
		set_target_properties (yoga_library PROPERTIES IMPORTED_LOCATION "${yoga_LIBRARY_OUTPUT}")		
	endif ()
	target_include_directories (yoga_library INTERFACE "${yoga_DIR}")
	add_dependencies (yoga_library yoga)
endif ()

set (yoga_LIBRARIES yoga_library)
