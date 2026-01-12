include_guard (DIRECTORY)

target_compile_options (cryptopp-static PRIVATE /W1 $<$<COMPILE_LANGUAGE:CXX>:/GR;/wd4231;/wd4251;/wd4275;/wd4355;/wd4505>)
target_compile_definitions (cryptopp-static PUBLIC CRYPTOPP_DISABLE_ASM=1)
