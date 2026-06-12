find_path (CCL_CORELIB_DIR NAMES "public/coreversion.h" HINTS "${CCL_REPOSITORY_ROOT}/core" DOC "corelib directory")
mark_as_advanced (CCL_CORELIB_DIR)

if (CCL_PREFER_PREBUILT_EXPORTS)
	find_file (CCL_EXPORTS_FILE NAMES "ccl.cmake" HINTS "${CMAKE_BINARY_DIR}/exports" ${CCL_EXPORTS_PATH})
	if (CCL_EXPORTS_FILE)
		set (CCL_USING_PREBUILT_EXPORTS ON)
		include ("${CCL_EXPORTS_FILE}")
	endif ()
endif ()

include (${CCL_CORELIB_DIR}/cmake/corelib-config.cmake)
