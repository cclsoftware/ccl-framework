include_guard (DIRECTORY)

ccl_find_path (assimp_DIR NAMES "code/Common/Assimp.cpp" HINTS "${CCL_SUBMODULES_DIR}/assimp" DOC "Assimp directory")
mark_as_advanced (assimp_DIR)

list (APPEND assimp_options
	-DBUILD_SHARED_LIBS=OFF
	-DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF
	-DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF
	-DASSIMP_BUILD_TESTS=OFF
	-DASSIMP_BUILD_TOOLS=OFF
	-DASSIMP_BUILD_FBX_IMPORTER=ON
	-DASSIMP_BUILD_OBJ_IMPORTER=ON
	-DASSIMP_BUILD_ZLIB=ON
	-DASSIMP_NO_EXPORT=ON
)

if (NOT WIN32)
	list (APPEND assimp_options -DASSIMP_INJECT_DEBUG_POSTFIX=OFF)
else ()
	list (APPEND assimp_options -DASSIMP_INJECT_DEBUG_POSTFIX=ON)
endif ()

# Disable assimp Ccache support when path to Ccache contains spaces
if (NOT WIN32 AND "${CCACHE}" MATCHES ".* .*")
	list (APPEND assimp_options -DASSIMP_BUILD_USE_CCACHE=OFF)
endif ()

if (NOT TARGET assimp)
	ccl_add_external_project (assimp "${assimp_DIR}" OPTIONS ${assimp_options})
	set_target_properties (assimp PROPERTIES USE_FOLDERS ON FOLDER services/ccl/libs)
endif ()

if (NOT TARGET assimp_library)
	add_library (assimp_library STATIC IMPORTED)
	if (assimp_LIBRARY_OUTPUT_DEBUG)
		set_target_properties (assimp_library PROPERTIES IMPORTED_LOCATION_DEBUG "${assimp_LIBRARY_OUTPUT_DEBUG}" IMPORTED_LOCATION_RELEASE "${assimp_LIBRARY_OUTPUT_RELEASE}")
	else ()
		set_target_properties (assimp_library PROPERTIES IMPORTED_LOCATION "${assimp_LIBRARY_OUTPUT}")		
	endif ()
	target_include_directories (assimp_library INTERFACE "${assimp_INCLUDE_DIR}")
	add_dependencies (assimp_library assimp)
endif ()

set (assimp_LIBRARIES assimp_library)
