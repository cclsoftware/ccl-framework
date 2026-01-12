install (FILES
	"${CCL_DIR}/platform/android/androidmain.cpp"
	"${CCL_DIR}/platform/android/cclandroidjni.h"
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/platform/android"
	COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

install (TARGETS corelib FILE_SET HEADERS DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/core COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX})

install (DIRECTORY
	${CCL_REPOSITORY_ROOT}/build/android/gradle
	DESTINATION "build/android"
	COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/corelib.android.cmake "
	if (NOT \"\${gradle_files_generated}\" STREQUAL \"\${CMAKE_CURRENT_BINARY_DIR}\")
		ccl_add_gradle_property (corelib \"solutionDir\" \"\${CMAKE_CURRENT_BINARY_DIR}\")
		ccl_add_gradle_property (corelib \"repositoryRoot\" \"\${CCL_FRAMEWORK_ROOT}\")

		ccl_add_gradle_dependency (corelib \"\${CCL_LIBRARY_DIR}/android-java/corelib.aar\")

		set (gradle_files_generated \"\${CMAKE_CURRENT_BINARY_DIR}\")
	endif ()

	set_target_properties (corelib PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcorelib.a\")

	set_target_properties (pnglib PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libpnglib.a\")
	set_target_properties (zlib PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libzlib.a\")
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclapp.android.cmake "
	set_target_properties (cclapp PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclapp.a\")
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclbase.android.cmake "
	set_target_properties (cclbase PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclbase.a\")
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclsystem.android.cmake "
	ccl_add_gradle_dependency (cclsystem \"\${CCL_LIBRARY_DIR}/android-java/cclsystem.aar\")

	set_target_properties (cclsystem PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclsystem.so\")
	ccl_copy_imported_jnilib (cclsystem)
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/ccltext.android.cmake "
	set_target_properties (ccltext PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libccltext.so\")
	ccl_copy_imported_jnilib (ccltext)
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclgui.android.cmake "
	ccl_add_gradle_dependency (cclgui \"androidx.documentfile:documentfile:1.0.1\" TRANSITIVE)
	ccl_add_gradle_dependency (cclgui \"\${CCL_LIBRARY_DIR}/android-java/cclgui.aar\")

	set_target_properties (cclgui PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclgui.so\")
	ccl_copy_imported_jnilib (cclgui)
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclnet.android.cmake "
	set_target_properties (cclnet PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclnet.so\")
	ccl_copy_imported_jnilib (cclnet)
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclsecurity.android.cmake "
	set_target_properties (cclsecurity PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclsecurity.so\")
	ccl_copy_imported_jnilib (cclsecurity)
	"
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/cclextras-stores.android.cmake "
	ccl_add_gradle_dependency (cclextras-stores \"com.android.billingclient:billing:6.1.0\" TRANSITIVE)
	ccl_add_gradle_dependency (cclextras-stores \"com.amazon.device:amazon-appstore-sdk:3.0.4\" TRANSITIVE)
	ccl_add_gradle_dependency (cclextras-stores \"\${CCL_LIBRARY_DIR}/android-java/cclextrasstores.aar\")

	set_target_properties (cclextras-stores PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/libcclextras-stores.a\")
	"
)

install (FILES 
	${CMAKE_CURRENT_BINARY_DIR}/corelib.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclapp.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclbase.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclsystem.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/ccltext.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclgui.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclnet.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclsecurity.android.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cclextras-stores.android.cmake
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
	COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/bluetoothservice.android.cmake "
	ccl_add_gradle_dependency (bluetoothservice \"\${CCL_LIBRARY_DIR}/android-java/bluetoothservice.aar\")

	set_target_properties (bluetoothservice PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/Plugins/libbluetoothservice.so\")
	ccl_copy_imported_jnilib (bluetoothservice)
	"
)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/bluetoothservice.android.cmake
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
	COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

foreach (service dapservice jsengine modelimporter3d sqlite)
	if (TARGET "${service}")
		file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/${service}.android.cmake "
			set_target_properties (${service} PROPERTIES IMPORTED_LOCATION \"\${CCL_LIBRARY_DIR}/android-\${VENDOR_TARGET_ARCHITECTURE}/Plugins/lib${service}.so\")
			ccl_copy_imported_jnilib (${service})
			"
		)

		install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${service}.android.cmake
			DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
			COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
		)
	endif ()
endforeach ()
