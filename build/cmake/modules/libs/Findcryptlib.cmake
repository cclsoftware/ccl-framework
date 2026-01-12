include_guard (DIRECTORY)

if (TARGET cryptopp-static)
	return ()
endif ()

ccl_find_path (cryptlib_DIR NAMES "cryptlib.h" HINTS "${CCL_SUBMODULES_DIR}/cryptopp" DOC "Crypto++ directory.")
mark_as_advanced (cryptlib_DIR)

set (cryptocpp_DISPLAY_CMAKE_SUPPORT_WARNING OFF)
set (BUILD_STATIC ON)
set (BUILD_SHARED OFF)
set (BUILD_TESTING OFF)
set (BUILD_DOCUMENTATION OFF)
set (USE_INTERMEDIATE_OBJECTS_TARGET OFF)

# Disable cryptopp compiler checks. We know our platforms and compilers and set compiler flags directly.
set (CRYPTOPP_ENABLE_COMPILER_CHECKS OFF CACHE BOOL "" FORCE)
mark_as_advanced (
	CRYPTOPP_ENABLE_COMPILER_CHECKS
	BUILD_STATIC
	BUILD_SHARED
	BUILD_TESTING
	BUILD_DOCUMENTATION
	USE_INTERMEDIATE_OBJECTS_TARGET
	DISABLE_ASM
	DISABLE_SSSE3
	DISABLE_SSE4
	DISABLE_AESNI
	DISABLE_SHA
	DISABLE_AVX
	DISABLE_AVX2
	CRYPTOPP_DATA_DIR
)

set (ANDROID_NDK "${CMAKE_ANDROID_NDK}")

add_subdirectory ("${cryptlib_DIR}" cryptopp)

ccl_configure_target (cryptopp-static)
set_target_properties (cryptopp-static PROPERTIES FOLDER "ccl/libs" USE_FOLDERS ON)

get_target_property (cryptopp_sources cryptopp-static SOURCES)
source_group ("source" FILES ${cryptopp_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE} "${cryptlib_DIR}/CMakeLists.txt")

target_sources (cryptopp-static PRIVATE "${cryptlib_DIR}/CMakeLists.txt")

add_library (cryptlib ALIAS cryptopp-static)

ccl_include_platform_specifics (cryptlib)
