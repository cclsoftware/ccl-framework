target_compile_definitions (tuio PUBLIC OSC_HOST_LITTLE_ENDIAN=1)
set_target_properties (tuio PROPERTIES CXX_VISIBILITY_PRESET default)
target_compile_options (tuio PRIVATE "-Wno-deprecated-declarations;-Wno-shorten-64-to-32")
