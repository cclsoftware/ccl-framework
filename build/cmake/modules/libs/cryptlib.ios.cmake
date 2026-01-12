include_guard (DIRECTORY)

target_compile_options (cryptopp-static PRIVATE "-frtti")
target_compile_options (cryptopp-static PRIVATE "-Wno-shorten-64-to-32")

set_target_properties (cryptopp-static PROPERTIES
    XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS[sdk=iphonesimulator*] "$(inherited) CRYPTOPP_DISABLE_ASM"
)

set_target_properties (cryptopp-static PROPERTIES
    XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS[arch=arm64] "$(inherited) CRYPTOPP_DISABLE_ARM_CRC32 CRYPTOPP_DISABLE_ARM_AES CRYPTOPP_DISABLE_ARM_SHA CRYPTOPP_DISABLE_ARM_PMULL"
)
