set_target_properties (tuio PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
target_compile_options (tuio PRIVATE "-Wno-deprecated-declarations;-Wno-shorten-64-to-32")
