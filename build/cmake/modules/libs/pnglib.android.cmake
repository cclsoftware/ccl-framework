target_compile_options (pnglib PRIVATE "-Wno-macro-redefined")

# Disable ASan/HWASan for libpng to avoid ODR violation warnings
if (${ENABLE_ASAN})
	target_compile_options (pnglib PRIVATE "-fno-sanitize=address")
elseif (${ENABLE_HWASAN} AND "${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)|(arm64)")
	target_compile_options (pnglib PRIVATE "-fno-sanitize=hwaddress")
endif ()

find_library (Math_LIBRARY NAMES m)
target_link_libraries (pnglib PUBLIC ${Math_LIBRARY})
