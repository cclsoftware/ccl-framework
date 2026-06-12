# Find CCL directory
find_path (CCL_FRAMEWORK_DIR NAMES "public/cclversion.h" HINTS "${CCL_REPOSITORY_ROOT}/ccl" DOC "CCL directory.")
mark_as_advanced (CCL_FRAMEWORK_DIR)

if (CCL_PREFER_PREBUILT_EXPORTS)
	set (export_file_name "ccl.cmake")
	if (CCL_ISOLATION_POSTFIX)
		set (export_file_name "ccl.${CCL_ISOLATION_POSTFIX}.cmake")
	endif ()
	find_file (CCL_EXPORTS_FILE NAMES "${export_file_name}" HINTS "${CMAKE_BINARY_DIR}/exports" ${CCL_EXPORTS_PATH})
	if (CCL_EXPORTS_FILE)
		set (CCL_USING_PREBUILT_EXPORTS ON)
		include ("${CCL_EXPORTS_FILE}")
	endif ()
endif ()

include ("${CCL_FRAMEWORK_DIR}/cmake/ccl-config.cmake")
