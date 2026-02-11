cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

# the individual subdirectories are included with build all, anyway
if(BUILD_ALL)
	return ()
endif()

project (EmbeddedTools)
set (PROJECT_APPLICATION_RUNTIME_DIRECTORY ".")
set (CCL_INSTALL_TOOLS ON)

include (vendor)

find_package ("corelib" REQUIRED)

ccl_set_project_vendor (ccl)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../bin2c/cmake bin2c)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../jsonconverter/cmake jsonconverter)
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/../skincrush/cmake skincrush)

if (NOT CCL_DISABLE_CPACK)
	# Package target
	set (CPACK_GENERATOR "ZIP")
	set (CPACK_PACKAGE_NAME "CCL Embedded Tools")
	set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	include (CPack)
endif ()
