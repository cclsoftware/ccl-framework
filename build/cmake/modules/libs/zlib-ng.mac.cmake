target_compile_options (zlib-ng PRIVATE "-Wno-shorten-64-to-32;-Wno-unused-command-line-argument")

foreach (arch IN ITEMS ${VENDOR_TARGET_ARCHITECTURE})
    if ("${arch}" MATCHES "(x86_64)")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_sse2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-msse2")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_sse2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-msse2")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_sse2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-msse2")

		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_ssse3.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mssse3")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_ssse3.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mssse3")

		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_sse42.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-msse4.2")

		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx2")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx2")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_avx2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx2")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_avx2.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx2")

		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx512f;-Xarch_x86_64;-mavx512bw;-Xarch_x86_64;-mavx512dq;-Xarch_x86_64;-mavx512vl")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512_vnni.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx512f;-Xarch_x86_64;-mavx512bw;-Xarch_x86_64;-mavx512vl;-Xarch_x86_64;-mavx512vnni")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx512.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx512f;-Xarch_x86_64;-mavx512bw;-Xarch_x86_64;-mavx512vl;-Xarch_x86_64;-mbmi2")

		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_pclmulqdq.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mssse3;-Xarch_x86_64;-mpclmul")
		set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_vpclmulqdq.c PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx512f;-Xarch_x86_64;-mvpclmulqdq")

        set_property (TARGET zlib-ng PROPERTY XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS[arch=x86_64] "$(inherited) X86_FEATURES X86_SSE2 X86_SSSE3 X86_SSE42 X86_AVX2 X86_AVX512 X86_AVX512VNNI X86_PCLMULQDQ_CRC X86_VPCLMULQDQ_CRC")
    elseif ("${arch}" MATCHES "(arm64)|(aarch64)")
        set_property (TARGET zlib-ng PROPERTY XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS[arch=arm64] "$(inherited) ARM_FEATURES ARM_NEON ARM_NEON_HASLD4")
    endif ()
endforeach ()
