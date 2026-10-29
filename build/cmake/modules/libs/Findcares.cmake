include_guard (DIRECTORY)

ccl_find_path (cares_DIR NAMES "include/ares.h" HINTS ${CCL_SUBMODULES_DIR}/cares)
mark_as_advanced (cares_DIR)

list (APPEND cares_options
	-DCARES_STATIC=ON
	-DCARES_SHARED=OFF
	-DCARES_STATIC_PIC=ON
	-DCARES_INSTALL=ON
	-DCARES_BUILD_TESTS=OFF
	-DCARES_BUILD_CONTAINER_TESTS=OFF
	-DCARES_BUILD_TOOLS=OFF
	-DCARES_THREADS=ON
)

if (WIN32)
	list (APPEND cares_options -DCMAKE_DEBUG_POSTFIX=d)
elseif (APPLE)
	# pipe2 is detected with newer SDKs but is not available on older deployment targets, see https://github.com/c-ares/c-ares/issues/1305
	list (APPEND cares_options -DHAVE_PIPE2=OFF)
endif ()

if (NOT TARGET cares)
	ccl_add_external_project (cares "${cares_DIR}" OPTIONS ${cares_options})
	set_target_properties (cares PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")
else ()
	ccl_include_platform_specifics (cares)
endif ()

if (NOT TARGET cares_library)
	add_library (cares_library STATIC IMPORTED GLOBAL)
	if (cares_LIBRARY_OUTPUT_DEBUG)
		set_target_properties (cares_library PROPERTIES IMPORTED_LOCATION_DEBUG "${cares_LIBRARY_OUTPUT_DEBUG}" IMPORTED_LOCATION_RELEASE "${cares_LIBRARY_OUTPUT_RELEASE}")
	else ()
		set_target_properties (cares_library PROPERTIES IMPORTED_LOCATION "${cares_LIBRARY_OUTPUT}")
	endif ()
	target_include_directories (cares_library INTERFACE "${cares_INCLUDE_DIR}")
	target_compile_definitions (cares_library INTERFACE CARES_STATICLIB)
	add_dependencies (cares_library cares)

	if (WIN32)
		target_link_libraries (cares_library INTERFACE ws2_32 advapi32 iphlpapi)
	elseif (APPLE)
		# ares_sysconfig_mac.c / event/ares_event_configchg.c use SCDynamicStore APIs on Apple platforms.
		target_link_libraries (cares_library INTERFACE "-framework SystemConfiguration" "-framework CoreFoundation")
	elseif (ANDROID)
		target_link_libraries (cares_library INTERFACE log)
	endif ()
endif ()

set (cares_LIBRARIES cares_library)
set (cares_FOUND ON)

if (cares_DIR AND EXISTS "${cares_DIR}/CMakeLists.txt")
	file (STRINGS "${cares_DIR}/CMakeLists.txt" line REGEX "^PROJECT \\(c-ares .*VERSION \".*\"")
	string (REGEX REPLACE ".*VERSION \"([0-9.]+)\".*" "\\1" cares_VERSION "${line}")
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (cares
	FOUND_VAR cares_FOUND
	REQUIRED_VARS cares_LIBRARIES
	VERSION_VAR cares_VERSION
)
