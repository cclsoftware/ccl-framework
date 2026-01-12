include_guard (DIRECTORY) 

find_package (libusb REQUIRED)
target_include_directories (usbservice PUBLIC ${libusb_INCLUDE_DIRS})
target_link_libraries (usbservice PRIVATE ${libusb_LIBRARIES})

list (APPEND usbservice_source_files
    ${usb_DIR}/source/libusb/libusbmanager.cpp
	${usb_DIR}/source/libusb/libusbmanager.h
	${usb_DIR}/source/libusb/libusbstatics.cpp
)

set (usbservice_DYLIBS
    usb-1.0
)

