include_guard (DIRECTORY)

list (APPEND bluetoothservice_source_files
	${bluetooth_DIR}/source/cocoa/cocoabluetoothstatics.mm
	${bluetooth_DIR}/source/cocoa/gattcentral.cocoa.mm
	${bluetooth_DIR}/source/cocoa/gattcentral.cocoa.h
	${bluetooth_DIR}/source/cocoa/gattperipheral.cocoa.mm
	${bluetooth_DIR}/source/cocoa/gattperipheral.cocoa.h
	${bluetooth_DIR}/source/cocoa/gattshared.cocoa.mm
	${bluetooth_DIR}/source/cocoa/gattshared.cocoa.h
)

find_library (COREBLUETOOTH_LIBRARY CoreBluetooth REQUIRED)

list (APPEND bluetoothservice_apple_frameworks
	${COREBLUETOOTH_LIBRARY}
)

target_link_libraries (bluetoothservice PUBLIC ${bluetoothservice_apple_frameworks})