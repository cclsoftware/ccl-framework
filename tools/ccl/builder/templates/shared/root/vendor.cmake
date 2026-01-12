include_guard (DIRECTORY)

set (REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/(RelPathToRoot)" CACHE PATH "Root directory of the repository" FORCE)
set (REPOSITORY_IDENTITIES_DIR "${REPOSITORY_ROOT}/build/identities" CACHE PATH "Identities directory" FORCE)

list (APPEND VENDOR_IDENTITY_DIRS "${REPOSITORY_IDENTITIES_DIR}")

if (CCL_REPOSITORY_ROOT AND EXISTS "${REPOSITORY_ROOT}/(RelPathToFramework)/build/cmake/modules/shared/vendor.cmake")
	include ("${REPOSITORY_ROOT}/(RelPathToFramework)/build/cmake/modules/shared/vendor.cmake")
else ()
	find_file (CCL_PLATFORMMACROS_FILE NAMES "vendor.${VENDOR_PLATFORM}.cmake" HINTS "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings.")

	if (EXISTS "${CCL_PLATFORMMACROS_FILE}")
		include ("${CCL_PLATFORMMACROS_FILE}")
	endif ()
endif ()
