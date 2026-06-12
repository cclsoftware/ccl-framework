find_package (corelib REQUIRED)

ccl_find_path (bluetooth_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/bluetooth" DOC "Bluetooth service directory")
mark_as_advanced (bluetooth_DIR)

ccl_import_prebuilt_targets (bluetoothservice)

include ("${bluetooth_DIR}/cmake/bluetoothservice-config.cmake")
