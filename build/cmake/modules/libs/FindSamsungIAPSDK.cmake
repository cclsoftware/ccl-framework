include_guard (DIRECTORY)

if (NOT ANDROID)
	return ()
endif ()

file (GLOB SamsungIAPSDK_AAR "${SAMSUNG_IAP_SDK_DIR}/Libs/samsung-iap-*.aar")

if (NOT SamsungIAPSDK_AAR)
	message (STATUS "Warning: Samsung In-App-Purchase SDK not found! Building without Samsung Store support.")
	message (STATUS "         Please set SAMSUNG_IAP_SDK_DIR to the location of the Samsung IAP SDK to enable support.")
	return ()
endif ()

string (REGEX REPLACE ".*/samsung-iap-(.*)\.aar" "\\1" SamsungIAPSDK_VERSION "${SamsungIAPSDK_AAR}")

if ("${SamsungIAPSDK_VERSION}" VERSION_LESS "6.4.0")
	message (STATUS "Warning: Samsung In-App-Purchase SDK version ${SamsungIAPSDK_VERSION} is no longer supported. Please upgrade the SDK to")
	message (STATUS "         version 6.4.0 or later. Building without Samsung Store support.")
	return ()
endif ()

if ("${SamsungIAPSDK_VERSION}" VERSION_GREATER "6.4.0")
	message (STATUS "Warning: Samsung In-App-Purchase SDK version ${SamsungIAPSDK_VERSION} found, but only versions up to 6.4.0 have been tested.")
	message (STATUS "         Continuing, but builds may fail.")
endif ()

set (SamsungIAPSDK_FOUND ON)
