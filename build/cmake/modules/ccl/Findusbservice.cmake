find_package (corelib REQUIRED)

ccl_find_path (usb_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/usb" DOC "USB service directory")

ccl_import_prebuilt_targets (usbservice)

include ("${usb_DIR}/cmake/usbservice-config.cmake")
