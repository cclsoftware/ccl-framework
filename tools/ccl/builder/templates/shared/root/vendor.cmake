include_guard (DIRECTORY)

set (REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/(RelPathToRoot)" CACHE PATH "Root directory of the repository" FORCE)
set (REPOSITORY_IDENTITIES_DIR "${REPOSITORY_ROOT}/build/identities" CACHE PATH "Identities directory" FORCE)

list (APPEND VENDOR_IDENTITY_DIRS "${REPOSITORY_IDENTITIES_DIR}")

set (current_vendor_file "${CMAKE_CURRENT_LIST_FILE}")
set (repository_vendor_file "${REPOSITORY_ROOT}/(RelPathToFramework)/build/cmake/modules/shared/vendor.cmake")
cmake_path (NORMAL_PATH current_vendor_file)
cmake_path (NORMAL_PATH repository_vendor_file)

if (NOT "${current_vendor_file}" STREQUAL "${repository_vendor_file}" AND EXISTS "${repository_vendor_file}")
	# found a vendor override, include this file
	include ("${REPOSITORY_ROOT}/(RelPathToFramework)/build/cmake/modules/shared/vendor.cmake")
else ()
	# no vendor override found, include platform specifics
	find_file (CCL_PLATFORMMACROS_FILE NAMES "vendor.${VENDOR_PLATFORM}.cmake" HINTS "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings.")

	if (EXISTS "${CCL_PLATFORMMACROS_FILE}")
		include ("${CCL_PLATFORMMACROS_FILE}")
	endif ()
endif ()
