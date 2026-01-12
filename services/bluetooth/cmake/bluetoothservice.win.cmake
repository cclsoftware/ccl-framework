include_guard (DIRECTORY)

list (APPEND bluetoothservice_source_files
	${bluetooth_DIR}/source/win/gattcentral.win.cpp
	${bluetooth_DIR}/source/win/gattcentral.win.h
	${bluetooth_DIR}/source/win/gattperipheral.win.cpp
	${bluetooth_DIR}/source/win/gattperipheral.win.h
	${bluetooth_DIR}/source/win/gattshared.win.cpp
	${bluetooth_DIR}/source/win/gattshared.win.h
	${bluetooth_DIR}/source/win/windowsbluetoothstatics.cpp
)
