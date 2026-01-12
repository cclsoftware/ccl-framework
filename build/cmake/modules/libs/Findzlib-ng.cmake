ccl_find_path (ZLIBNG_INCLUDE_DIR NAMES "adler32.c" HINTS "${ZLIB_ROOT}" "${CCL_SUBMODULES_DIR}/zlib-ng" DOC "zlib-ng source directory")

mark_as_advanced (ZLIBNG_INCLUDE_DIR)

set (ZLIBNG_GENERATED_INCLUDES_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng/include")

set (ZLIB_INCLUDE_DIRS ${ZLIBNG_INCLUDE_DIR} ${ZLIBNG_GENERATED_INCLUDES_DIR})

configure_file (${ZLIBNG_INCLUDE_DIR}/zconf.h.in 
				${ZLIBNG_GENERATED_INCLUDES_DIR}/zconf.h)

configure_file (${ZLIBNG_INCLUDE_DIR}/zlib.h.in 
				${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib.h)

configure_file (${ZLIBNG_INCLUDE_DIR}/zlib_name_mangling.h.empty 
				${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib_name_mangling.h)

if (TARGET zlib-ng)
	set (ZLIBNG_LIBRARY zlib-ng)
	set (ZLIB_LIBRARIES zlib-ng)
	return ()
endif ()

list (APPEND zlibng_public_headers
	${ZLIBNG_GENERATED_INCLUDES_DIR}/zconf.h
	${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib.h
	${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib_name_mangling.h
)

list (APPEND zlibng_sources
	${ZLIBNG_INCLUDE_DIR}/adler32.c
	${ZLIBNG_INCLUDE_DIR}/compress.c
	${ZLIBNG_INCLUDE_DIR}/cpu_features.c
	${ZLIBNG_INCLUDE_DIR}/crc32.c
	${ZLIBNG_INCLUDE_DIR}/crc32_braid_comb.c
	${ZLIBNG_INCLUDE_DIR}/deflate.c
	${ZLIBNG_INCLUDE_DIR}/deflate_fast.c
	${ZLIBNG_INCLUDE_DIR}/deflate_huff.c
	${ZLIBNG_INCLUDE_DIR}/deflate_medium.c
	${ZLIBNG_INCLUDE_DIR}/deflate_quick.c
	${ZLIBNG_INCLUDE_DIR}/deflate_rle.c
	${ZLIBNG_INCLUDE_DIR}/deflate_slow.c
	${ZLIBNG_INCLUDE_DIR}/deflate_stored.c
	${ZLIBNG_INCLUDE_DIR}/functable.c
	${ZLIBNG_INCLUDE_DIR}/infback.c
	${ZLIBNG_INCLUDE_DIR}/inflate.c
	${ZLIBNG_INCLUDE_DIR}/inftrees.c
	${ZLIBNG_INCLUDE_DIR}/insert_string.c
	${ZLIBNG_INCLUDE_DIR}/insert_string_roll.c
	${ZLIBNG_INCLUDE_DIR}/trees.c
	${ZLIBNG_INCLUDE_DIR}/uncompr.c
	${ZLIBNG_INCLUDE_DIR}/zutil.c
)

list (APPEND zlibng_generic_sources
	${ZLIBNG_INCLUDE_DIR}/arch/generic/adler32_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/adler32_fold_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/chunkset_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/compare256_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/crc32_braid_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/crc32_fold_c.c
	${ZLIBNG_INCLUDE_DIR}/arch/generic/slide_hash_c.c
)

foreach (arch IN ITEMS ${VENDOR_TARGET_ARCHITECTURE})
	if ("${arch}" MATCHES "(x86_64)|(i386)")
		list (APPEND zlibng_x86_sources
			${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512_vnni.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_sse42.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_ssse3.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx512.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_sse2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_ssse3.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_avx2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_sse2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_pclmulqdq.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_vpclmulqdq.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_avx2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_sse2.c
			${ZLIBNG_INCLUDE_DIR}/arch/x86/x86_features.c
		)

		if (NOT MSVC AND NOT APPLE)
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_sse2.c PROPERTIES COMPILE_OPTIONS "-msse2")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_sse2.c PROPERTIES COMPILE_OPTIONS "-msse2")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_sse2.c PROPERTIES COMPILE_OPTIONS "-msse2")

			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_ssse3.c PROPERTIES COMPILE_OPTIONS "-mssse3")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_ssse3.c PROPERTIES COMPILE_OPTIONS "-mssse3")

			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_sse42.c PROPERTIES COMPILE_OPTIONS "-msse4.2")

			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx2")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx2")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/compare256_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx2")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/slide_hash_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx2")

			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512.c PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw;-mavx512dq;-mavx512vl")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/adler32_avx512_vnni.c PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw;-mavx512vl;-mavx512vnni")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/chunkset_avx512.c PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw;-mavx512vl;-mbmi2")

			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_pclmulqdq.c PROPERTIES COMPILE_OPTIONS "-mssse3;-mpclmul")
			set_source_files_properties (${ZLIBNG_INCLUDE_DIR}/arch/x86/crc32_vpclmulqdq.c PROPERTIES COMPILE_OPTIONS "-mavx512f;-mvpclmulqdq")
		endif ()
	elseif ("${arch}" MATCHES "(armv7)|(armv8)|(arm64)|(aarch64)")
		list (APPEND zlibng_arm_sources
			${ZLIBNG_INCLUDE_DIR}/arch/arm/adler32_neon.c
			${ZLIBNG_INCLUDE_DIR}/arch/arm/arm_features.c
			${ZLIBNG_INCLUDE_DIR}/arch/arm/chunkset_neon.c
			${ZLIBNG_INCLUDE_DIR}/arch/arm/compare256_neon.c
			${ZLIBNG_INCLUDE_DIR}/arch/arm/slide_hash_neon.c
		)
	endif ()
endforeach ()

source_group ("source" FILES ${zlibng_sources} ${zlibng_public_headers})
source_group ("source/generic" FILES ${zlibng_generic_sources})
source_group ("source/x86" FILES ${zlibng_x86_sources})
source_group ("source/arm" FILES ${zlibng_arm_sources})

ccl_add_library (zlib-ng STATIC)
ccl_add_library (ZLIB::ZLIB ALIAS zlib-ng)

set_target_properties (zlib-ng PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")

ccl_include_platform_specifics (zlib-ng)

target_sources (zlib-ng PRIVATE ${zlibng_sources} ${zlibng_generic_sources} ${zlibng_x86_sources} ${zlibng_arm_sources})
#ccl_target_headers (zlib-ng INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib BASE_DIRS ${ZLIBNG_INCLUDE_DIR} ${ZLIBNG_GENERATED_INCLUDES_DIR} FILES ${zlibng_public_headers})

target_include_directories (zlib-ng PUBLIC $<BUILD_INTERFACE:${ZLIBNG_INCLUDE_DIR}>)# $<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib>)
target_include_directories (zlib-ng PUBLIC $<BUILD_INTERFACE:${ZLIBNG_GENERATED_INCLUDES_DIR}>)# $<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib>)
target_compile_definitions (zlib-ng PRIVATE ZLIB_COMPAT)

if (NOT MSVC)
	target_compile_definitions (zlib-ng PRIVATE HAVE_ATTRIBUTE_ALIGNED)
	target_compile_definitions (zlib-ng PRIVATE HAVE_BUILTIN_CTZ HAVE_BUILTIN_CTZLL)
endif ()

if (NOT APPLE)
	foreach (arch IN ITEMS ${VENDOR_TARGET_ARCHITECTURE})
		if ("${arch}" MATCHES "(x86_64)|(i386)")
			target_compile_definitions (zlib-ng PRIVATE X86_FEATURES)
			target_compile_definitions (zlib-ng PRIVATE X86_SSE2 X86_SSSE3 X86_SSE42)
			target_compile_definitions (zlib-ng PRIVATE X86_AVX2 X86_AVX512 X86_AVX512VNNI)
			target_compile_definitions (zlib-ng PRIVATE X86_PCLMULQDQ_CRC X86_VPCLMULQDQ_CRC)
		elseif ("${arch}" MATCHES "(armv7)|(armv8)|(arm64)|(aarch64)")
			target_compile_definitions (zlib-ng PRIVATE ARM_FEATURES)
			target_compile_definitions (zlib-ng PRIVATE ARM_NEON ARM_NEON_HASLD4)
		endif ()
	endforeach ()
endif ()

if (CCL_SYSTEM_INSTALL)
	if (VENDOR_STATIC_LIBRARY_DESTINATION)
		set (zlib_destination "${VENDOR_STATIC_LIBRARY_DESTINATION}")
	else ()
		set (zlib_destination "${VENDOR_LIBRARY_DESTINATION}")		
	endif ()
	install (TARGETS zlib-ng EXPORT ccl-targets DESTINATION "${zlib_destination}"
							 ARCHIVE DESTINATION "${zlib_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
							 FRAMEWORK DESTINATION "${zlib_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
							 #FILE_SET HEADERS DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib COMPONENT public_headers
	)
	install (FILES $<TARGET_FILE_DIR:zlib-ng>/zlib-ng$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${zlib_destination}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
endif ()

set (ZLIBNG_LIBRARY zlib-ng)
set (ZLIB_LIBRARIES zlib-ng)

if (EXISTS "${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib.h")
	file (STRINGS "${ZLIBNG_GENERATED_INCLUDES_DIR}/zlib.h" ZLIB_H REGEX "^#define ZLIBNG_VERSION \"[^\"]*\"$")

	string (REGEX REPLACE "^.*ZLIBNG_VERSION \"([0-9]+).*$" "\\1" ZLIBNG_VERSION_MAJOR "${ZLIB_H}")
	string (REGEX REPLACE "^.*ZLIBNG_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" ZLIBNG_VERSION_MINOR  "${ZLIB_H}")
	string (REGEX REPLACE "^.*ZLIBNG_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" ZLIBNG_VERSION_PATCH "${ZLIB_H}")
	set (ZLIBNG_VERSION_STRING "${ZLIBNG_VERSION_MAJOR}.${ZLIBNG_VERSION_MINOR}.${ZLIBNG_VERSION_PATCH}")

	# only append a TWEAK version if it exists:
	set (ZLIBNG_VERSION_TWEAK "")
	if ("${ZLIB_H}" MATCHES "ZLIBNG_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)")
		set (ZLIBNG_VERSION_TWEAK "${CMAKE_MATCH_1}")
		string (APPEND ZLIBNG_VERSION_STRING ".${ZLIBNG_VERSION_TWEAK}")
	endif()

	set (ZLIBNG_MAJOR_VERSION "${ZLIBNG_VERSION_MAJOR}")
	set (ZLIBNG_MINOR_VERSION "${ZLIBNG_VERSION_MINOR}")
	set (ZLIBNG_PATCH_VERSION "${ZLIBNG_VERSION_PATCH}")
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (zlib-ng
	REQUIRED_VARS ZLIBNG_LIBRARY
	VERSION_VAR ZLIBNG_VERSION_STRING
)
