include_guard (DIRECTORY)

if (TARGET hidapi::hidapi)
	return ()
endif ()

ccl_find_path (hidapi_DIR NAMES "hidapi/hidapi.h" HINTS "${CCL_SUBMODULES_DIR}/hidapi/" DOC "hidapi directory.")

set (HIDAPI_WITH_LIBUSB FALSE) # TODO: surely will be used only on Linux
set (BUILD_SHARED_LIBS FALSE) # HIDAPI as static library on all platforms
add_subdirectory ("${hidapi_DIR}" hidapi)

if (TARGET hidapi::hidapi)
	if (TARGET hidapi_winapi)
		set_target_properties (hidapi_winapi PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")
	endif ()
	set (hidapi_LIBRARIES hidapi::hidapi)
	set (hidapi_INCLUDE_DIRS ${hidapi_DIR}/hidapi)
	if (XCODE)
		list (APPEND hidapi_INCLUDE_DIRS ${hidapi_DIR}/mac)
	endif ()
	set (hidapi_FOUND ON)
endif ()
