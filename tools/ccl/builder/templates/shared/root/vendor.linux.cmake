include_guard (DIRECTORY)

add_compile_options ("$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
add_link_options ("-fuse-ld=lld;-Wl,--no-allow-shlib-undefined")