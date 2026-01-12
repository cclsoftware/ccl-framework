include_guard (DIRECTORY)

target_compile_options (cryptopp-static PRIVATE "-frtti")
target_compile_options (cryptopp-static PRIVATE "-Wno-shorten-64-to-32")

target_compile_definitions (cryptopp-static PUBLIC CRYPTOPP_DISABLE_ASM=1)
