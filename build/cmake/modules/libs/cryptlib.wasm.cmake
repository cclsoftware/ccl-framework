include_guard (DIRECTORY)

target_compile_options (cryptopp-static PRIVATE "-frtti")
target_compile_options (cryptopp-static PRIVATE "-Wno-shorten-64-to-32")

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)")
	target_compile_definitions (cryptopp-static PUBLIC CRYPTOPP_DISABLE_SSSE3=1)
else ()
	target_compile_definitions (cryptopp-static PUBLIC CRYPTOPP_DISABLE_ASM=1)
endif ()
