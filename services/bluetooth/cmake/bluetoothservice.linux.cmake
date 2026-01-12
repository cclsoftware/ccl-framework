include_guard (DIRECTORY)

list (APPEND bluetoothservice_ccl_sources
	${CCL_DIR}/platform/linux/interfaces/linuxiids.cpp
)

list (APPEND bluetoothservice_source_files
	${bluetooth_DIR}/source/linux/gattcentral.linux.cpp
	${bluetooth_DIR}/source/linux/gattcentral.linux.h
	${bluetooth_DIR}/source/linux/gattperipheral.linux.cpp
	${bluetooth_DIR}/source/linux/gattperipheral.linux.h
	${bluetooth_DIR}/source/linux/gattshared.linux.cpp
	${bluetooth_DIR}/source/linux/gattshared.linux.h
	${bluetooth_DIR}/source/linux/linuxbluetoothstatics.cpp
)

set_target_properties (bluetoothservice PROPERTIES INSTALL_RPATH "$ORIGIN/..")

ccl_use_dbus_interface (bluetoothservice ${bluetooth_DIR}/source/linux/interfaces/dbus/org.bluez.Adapter1.xml)
ccl_use_dbus_interface (bluetoothservice ${bluetooth_DIR}/source/linux/interfaces/dbus/org.bluez.Device1.xml)
ccl_use_dbus_interface (bluetoothservice ${bluetooth_DIR}/source/linux/interfaces/dbus/org.bluez.Service1.xml)
ccl_use_dbus_interface (bluetoothservice ${bluetooth_DIR}/source/linux/interfaces/dbus/org.bluez.Characteristics1.xml)
ccl_use_dbus_interface (bluetoothservice ${bluetooth_DIR}/source/linux/interfaces/dbus/org.bluez.Descriptor1.xml)
