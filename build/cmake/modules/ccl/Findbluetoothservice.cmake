include_guard (DIRECTORY)

ccl_find_path (bluetooth_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/bluetooth" DOC "Bluetooth service directory")
include ("${bluetooth_DIR}/cmake/bluetoothservice-config.cmake")
