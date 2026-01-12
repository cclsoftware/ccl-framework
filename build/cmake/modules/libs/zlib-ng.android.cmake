target_compile_options (zlib-ng PRIVATE "-Wno-shorten-64-to-32")

target_compile_definitions (zlib-ng PRIVATE HAVE_SYS_AUXV_H ARM_AUXV_HAS_CRC32 ARM_AUXV_HAS_NEON)
