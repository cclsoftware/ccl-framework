include_guard (DIRECTORY)

# set toolchain and library versions
set (VENDOR_ANDROID_GRADLE_VERSION "9.2.0")				# when changing this, also update build/android/gradle/wrapper/gradle-wrapper.properties
set (VENDOR_ANDROID_GRADLE_PLUGIN_VERSION "8.13.1")

set (VENDOR_ANDROID_BUILD_PLATFORM_VERSION "36")
set (VENDOR_ANDROID_BUILD_TOOLS_VERSION "36.1.0")

set (VENDOR_ANDROID_MINIMUM_PLATFORM_VERSION "${ANDROID_API_LEVEL}")
set (VENDOR_ANDROID_TARGET_PLATFORM_VERSION "36")

set (VENDOR_ANDROID_NDK_VERSION "29.0.14206865")		# when changing this, also update CMakePresets<platform>.json
set (VENDOR_ANDROID_NATIVE_CMAKE_VERSION "4.1.2")

set (VENDOR_ANDROIDX_CORE_LIBRARY_VERSION "1.17.0")
set (VENDOR_ANDROIDX_APPCOMPAT_LIBRARY_VERSION "1.7.1")
set (VENDOR_ANDROIDX_CONSTRAINTLAYOUT_LIBRARY_VERSION "2.2.1")

set (VENDOR_ANDROID_MATERIAL_DESIGN_LIBRARY_VERSION "1.13.0")

# set additional compile flags
add_compile_options ("-fno-rtti;-Wno-inconsistent-missing-override")
