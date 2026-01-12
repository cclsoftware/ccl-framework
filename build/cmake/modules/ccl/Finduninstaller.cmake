include_guard (DIRECTORY)

find_package (corelib REQUIRED)

if ("${VENDOR_PLATFORM}" STREQUAL "win")
	ccl_find_path (uninstaller_DIR NAMES "../packaging/resource/uninstaller.ico" HINTS "${CCL_REPOSITORY_ROOT}/build/win/nsis/uninstaller/cmake" DOC "Uninstaller directory.")
	mark_as_advanced (uninstaller_DIR)
	include ("${uninstaller_DIR}/uninstaller-config.cmake")
endif ()
