install (FILES
	"${CCL_DIR}/platform/win/winmain.cpp"
	"${CCL_DIR}/platform/win/winconsolemain.cpp"
	"${CCL_DIR}/platform/win/cclwindows.h"
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/platform/win"
	COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

cpack_add_component (public_headers_win_arm64 DISPLAY_NAME "win_arm64" DESCRIPTION "Public header files for Windows arm64" DEPENDS public_headers HIDDEN)
cpack_add_component (prebuilt_libraries_win_arm64 DISPLAY_NAME "win_arm64" DESCRIPTION "Prebuilt libraries targeting Windows arm64" GROUP prebuilt_libraries DEPENDS public_headers_win_arm64)
cpack_add_component (services_win_arm64 DISPLAY_NAME "win_arm64" DESCRIPTION "Prebuilt services targeting Windows arm64" GROUP services DEPENDS prebuilt_libraries_win_arm64)

cpack_add_component (public_headers_android_arm DISPLAY_NAME "android_arm" DESCRIPTION "Public header files for Android arm" DEPENDS public_headers HIDDEN)
cpack_add_component (prebuilt_libraries_android_arm DISPLAY_NAME "android_arm" DESCRIPTION "Prebuilt libraries targeting Android arm" GROUP prebuilt_libraries DEPENDS public_headers_android_arm)
cpack_add_component (services_android_arm DISPLAY_NAME "android_arm" DESCRIPTION "Prebuilt services targeting Android arm" GROUP services DEPENDS prebuilt_libraries_android_arm)

cpack_add_component (public_headers_android_arm64 DISPLAY_NAME "android_arm" DESCRIPTION "Public header files for Android arm64" DEPENDS public_headers HIDDEN)
cpack_add_component (prebuilt_libraries_android_arm64 DISPLAY_NAME "android_arm64" DESCRIPTION "Prebuilt libraries targeting Android arm64" GROUP prebuilt_libraries DEPENDS public_headers_android_arm64)
cpack_add_component (services_android_arm64 DISPLAY_NAME "android_arm64" DESCRIPTION "Prebuilt services targeting Android arm64" GROUP services DEPENDS prebuilt_libraries_android_arm64)

cpack_add_component (public_headers_android_x86 DISPLAY_NAME "android_x86" DESCRIPTION "Public header files for Android x86" DEPENDS public_headers HIDDEN)
cpack_add_component (prebuilt_libraries_android_x86 DISPLAY_NAME "android_x86" DESCRIPTION "Prebuilt libraries targeting Android x86" GROUP prebuilt_libraries DEPENDS public_headers_android_x86)
cpack_add_component (services_android_x86 DISPLAY_NAME "android_x86" DESCRIPTION "Prebuilt services targeting Android x86" GROUP services DEPENDS prebuilt_libraries_android_x86)

cpack_add_component (public_headers_android_x64 DISPLAY_NAME "android_x64" DESCRIPTION "Public header files for Android x64" DEPENDS public_headers HIDDEN)
cpack_add_component (prebuilt_libraries_android_x64 DISPLAY_NAME "android_x64" DESCRIPTION "Prebuilt libraries targeting Android x64" GROUP prebuilt_libraries DEPENDS public_headers_android_x64)
cpack_add_component (services_android_x64 DISPLAY_NAME "android_x64" DESCRIPTION "Prebuilt services targeting Android x64" GROUP services DEPENDS prebuilt_libraries_android_x64)
