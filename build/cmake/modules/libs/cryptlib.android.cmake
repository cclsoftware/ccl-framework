include_guard (DIRECTORY)

target_compile_options (cryptopp-static PRIVATE "-frtti" "-fexceptions")
target_compile_options (cryptopp-static PRIVATE "-Wno-shorten-64-to-32")

if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "armv7")
	list (APPEND cryptlib_compile_definitions CRYPTOPP_DISABLE_ASM)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
	# Disable CRC and crypto extensions as there are Android devices based on chips that do not have these
	list (APPEND cryptlib_compile_definitions CRYPTOPP_DISABLE_ARM_CRC32 CRYPTOPP_DISABLE_ARM_AES CRYPTOPP_DISABLE_ARM_SHA CRYPTOPP_DISABLE_ARM_PMULL)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "i386")
	# Disable SSE4 and newer as there are Android x86 devices that do not support it
	list (APPEND cryptlib_compile_definitions CRYPTOPP_DISABLE_SSE4)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "x86_64")
	# Disable AVX and crypto instructions as there are Android x86-64 devices that do not support these
	list (APPEND cryptlib_compile_definitions CRYPTOPP_DISABLE_AVX CRYPTOPP_DISABLE_AESNI CRYPTOPP_DISABLE_SHANI CRYPTOPP_DISABLE_CLMUL)
endif ()

target_compile_definitions (cryptopp-static PUBLIC "${cryptlib_compile_definitions}")
