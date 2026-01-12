include_guard (DIRECTORY)

list (APPEND bluetoothservice_android_sources
	${bluetooth_DIR}/source/android/androidbluetoothstatics.cpp
	${bluetooth_DIR}/source/android/gattcentral.android.cpp
	${bluetooth_DIR}/source/android/gattcentral.android.h
	${bluetooth_DIR}/source/android/gattperipheral.android.cpp
	${bluetooth_DIR}/source/android/gattperipheral.android.h
	${bluetooth_DIR}/source/android/gattshared.android.cpp
	${bluetooth_DIR}/source/android/gattshared.android.h
)

list (APPEND bluetoothservice_java_sources
	${bluetooth_DIR}/source/android/java/GattCentral.java
	${bluetooth_DIR}/source/android/java/GattCentralDevice.java
)

list (APPEND bluetoothservice_core_sources
	${corelib_android_jnionload_sources}
)

source_group ("source/android" FILES ${bluetoothservice_android_sources})
source_group ("source/android/java" FILES ${bluetoothservice_java_sources})
source_group ("source/core" FILES ${bluetoothservice_core_sources})

list (APPEND bluetoothservice_sources
    ${bluetoothservice_android_sources}
    ${bluetoothservice_java_sources}
    ${bluetoothservice_core_sources}
)

ccl_add_aar_project (bluetoothservice NAMESPACE "dev.ccl.services.bluetooth" DEPENDS cclgui)
ccl_install_aar (bluetoothservice COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})

ccl_add_java_sourcedir (bluetoothservice "${CMAKE_CURRENT_LIST_DIR}/../source/android/java")
ccl_add_java_sourcedir (bluetoothservice "${CMAKE_CURRENT_LIST_DIR}/../meta/generated/java")
