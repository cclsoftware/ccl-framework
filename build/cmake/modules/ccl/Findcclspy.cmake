if ("${VENDOR_PLATFORM}" STREQUAL "android" OR "${VENDOR_PLATFORM}" STREQUAL "ios")
	return ()
endif ()

find_package (corelib REQUIRED)

ccl_find_path (cclspy_DIR NAMES "../packaging/win/resource/spy.ico" HINTS "${CCL_REPOSITORY_ROOT}/services/cclspy/cmake" DOC "CCL Spy directory.")
mark_as_advanced (cclspy_DIR)

ccl_import_prebuilt_targets (cclspy ISOLATED)

include ("${cclspy_DIR}/cclspy-config.cmake")
