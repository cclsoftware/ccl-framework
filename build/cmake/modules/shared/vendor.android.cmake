include_guard (DIRECTORY)

include (vendor.clang)

set (VENDOR_LTO_CACHE_DIRECTORY "${REPOSITORY_ROOT}/build/cmake/android/cache/lto" CACHE PATH "Link time optimization cache directory")
set (VENDOR_LTO_CACHE_SIZE "2g" CACHE STRING "Link time optimization cache size")

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

# detect target architecture
if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "i386")
	set (VENDOR_PLATFORM_SUBDIR "x86")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "x86_64")
	set (VENDOR_PLATFORM_SUBDIR "x64")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "armv7")
	set (VENDOR_PLATFORM_SUBDIR "arm")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
	set (VENDOR_PLATFORM_SUBDIR "arm64")
else ()
	set (VENDOR_PLATFORM_SUBDIR "unknown")
endif ()

# add default definitions
add_compile_definitions (ANDROID)

# add default compiler options
add_compile_options (
	"-fno-delete-null-pointer-checks"					# don't treat usage of null pointers as undefined behavior
	"$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"				# we don't use RTTI
	"$<$<COMPILE_LANGUAGE:CXX>:-fsized-deallocation>"	# enable sized deallocation for C++11 (is default for C++14 and above)
	"-ffunction-sections"								# strip unused functions
	"-fdata-sections"									# strip unused data
	"-Wno-varargs"
)

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(armv7)")
	add_compile_options ("-mfpu=neon")
endif ()

if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
	add_compile_options ("$<$<CONFIG:Release,RelWithDebInfo>:-flto=thin>")	# incremental link time optimization
endif ()

# add default linker options
add_link_options (
	"-Wl,--no-allow-shlib-undefined"						# don't allow unresolved symbols in shared libraries, i.e. show an error at link time, not at runtime
	"-Wl,-z,max-page-size=16384"							# use 16 kB page alignment
	"-Wl,--thinlto-cache-dir=${VENDOR_LTO_CACHE_DIRECTORY}"	# link time optimization cache
	"-Wl,--thinlto-cache-policy,cache_size_bytes=${VENDOR_LTO_CACHE_SIZE}"	# limit lto cache size
)

if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
	add_link_options ("$<$<CONFIG:Release,RelWithDebInfo>:-flto=thin>")	# enable link time optimization for release builds
endif ()

option (ENABLE_ASAN "Enable Address Sanitizer" OFF)
option (ENABLE_HWASAN "Enable HWAddress Sanitizer (64 bit, Android 14 or newer)" OFF)

if (${ENABLE_ASAN})
	add_compile_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=address"
	)
	add_link_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=address"
	)
elseif (${ENABLE_HWASAN} AND "${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)|(arm64)")
	add_compile_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=hwaddress"
	)
	add_link_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=hwaddress"
	)
endif ()

set (CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "" FORCE)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_C_VISIBILITY_PRESET hidden)

set (CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Debug")
