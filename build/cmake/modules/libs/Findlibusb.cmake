include_guard (DIRECTORY)

if (NOT TARGET usb-1.0)
	ccl_find_path (libusb_DIR NAMES "libusb/libusb/libusb.h" HINTS "${CCL_SUBMODULES_DIR}/libusb/" DOC "libusb directory.")

	set (libusb_symbols
		libusb_get_device_descriptor
		libusb_handle_events_timeout_completed
		libusb_close
		libusb_open
		libusb_init_context
		libusb_exit
		libusb_get_string_descriptor_ascii
		libusb_hotplug_register_callback
		libusb_has_capability
		libusb_hotplug_deregister_callback
	)

	set (LIBUSB_BUILD_SHARED_LIBS ON CACHE BOOL "build libusb shared library")
	set (LIBUSB_INSTALL_TARGETS OFF CACHE BOOL "disable libusb install")

	add_subdirectory ("${libusb_DIR}" libusb)

	if (TARGET usb-1.0)
		ccl_configure_target (usb-1.0)
		ccl_export_symbols (usb-1.0 ${libusb_symbols})
	endif ()
endif ()

if (TARGET usb-1.0)
	set (libusb_LIBRARIES usb-1.0)
	set (libusb_INCLUDE_DIRS ${libusb_DIR}/libusb)
	set (libusb_FOUND ON)
else ()
	set (libusb_FOUND OFF)
endif ()
