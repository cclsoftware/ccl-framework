include_guard (DIRECTORY)

ccl_find_path (usb_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/usb" DOC "USB service directory")
include ("${usb_DIR}/cmake/usbservice-config.cmake")
