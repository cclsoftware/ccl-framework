include_guard (DIRECTORY) 

set_target_properties (usbservice PROPERTIES INSTALL_RPATH "$ORIGIN/..")

find_package (libusb REQUIRED)
target_include_directories (usbservice PUBLIC ${libusb_INCLUDE_DIRS})
target_link_libraries (usbservice PRIVATE ${libusb_LIBRARIES})

list (APPEND usbservice_source_files
    ${usb_DIR}/source/libusb/libusbmanager.cpp
	${usb_DIR}/source/libusb/libusbmanager.h
	${usb_DIR}/source/libusb/libusbstatics.cpp
)

if (CCL_SYSTEM_INSTALL)
	install (TARGETS ${libusb_LIBRARIES}
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS ${libusb_LIBRARIES} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
endif ()
