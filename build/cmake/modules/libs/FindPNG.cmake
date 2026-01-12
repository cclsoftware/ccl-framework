include_guard (DIRECTORY)

set (ZLIB_USE_STATIC_LIBS ON)
find_package (ZLIB REQUIRED)

ccl_find_path (PNG_INCLUDE_DIR NAMES "png.c" HINTS "${CCL_SUBMODULES_DIR}/libpng" DOC "libpng source directory")
mark_as_advanced (PNG_INCLUDE_DIR)

set (PNG_GENERATED_INCLUDES_DIR "${CMAKE_CURRENT_BINARY_DIR}/pnglib/include")

configure_file(${PNG_INCLUDE_DIR}/scripts/pnglibconf.h.prebuilt
               ${PNG_GENERATED_INCLUDES_DIR}/pnglibconf.h)

if (TARGET pnglib)
	set (PNG_LIBRARY pnglib)
	set (PNG_LIBRARIES pnglib)
	set (PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR} ${PNG_GENERATED_INCLUDES_DIR} ${ZLIB_INCLUDE_DIRS})
	return ()
endif ()

list (APPEND pnglib_public_headers
	${PNG_INCLUDE_DIR}/png.h
	${PNG_GENERATED_INCLUDES_DIR}/pnglibconf.h	
)

list (APPEND pnglib_sources
	${PNG_INCLUDE_DIR}/arm/arm_init.c
	${PNG_INCLUDE_DIR}/arm/filter_neon_intrinsics.c
	${PNG_INCLUDE_DIR}/arm/palette_neon_intrinsics.c
	${PNG_INCLUDE_DIR}/png.c
	${PNG_INCLUDE_DIR}/pngconf.h
	${PNG_INCLUDE_DIR}/pngdebug.h
	${PNG_INCLUDE_DIR}/pngerror.c
	${PNG_INCLUDE_DIR}/pngget.c
	${PNG_INCLUDE_DIR}/pnginfo.h
	${PNG_INCLUDE_DIR}/pngmem.c
	${PNG_INCLUDE_DIR}/pngpread.c
	${PNG_INCLUDE_DIR}/pngpriv.h
	${PNG_INCLUDE_DIR}/pngread.c
	${PNG_INCLUDE_DIR}/pngrio.c
	${PNG_INCLUDE_DIR}/pngrtran.c
	${PNG_INCLUDE_DIR}/pngrutil.c
	${PNG_INCLUDE_DIR}/pngset.c
	${PNG_INCLUDE_DIR}/pngstruct.h
	${PNG_INCLUDE_DIR}/pngtrans.c
	${PNG_INCLUDE_DIR}/pngwio.c
	${PNG_INCLUDE_DIR}/pngwrite.c
	${PNG_INCLUDE_DIR}/pngwtran.c
	${PNG_INCLUDE_DIR}/pngwutil.c
)
source_group ("source" FILES ${pnglib_sources} ${pnglib_public_headers})

ccl_add_library (pnglib STATIC)
ccl_add_library (PNG::PNG ALIAS pnglib)

set_target_properties (pnglib PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")

ccl_include_platform_specifics (pnglib)

target_sources (pnglib PRIVATE ${pnglib_sources})
#ccl_target_headers (pnglib INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/png BASE_DIRS ${PNG_INCLUDE_DIR} ${PNG_GENERATED_INCLUDES_DIR} FILES ${pnglib_public_headers})

target_include_directories (pnglib PUBLIC $<BUILD_INTERFACE:${PNG_INCLUDE_DIR}>)# $<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}/png>)
target_include_directories (pnglib PUBLIC $<BUILD_INTERFACE:${PNG_GENERATED_INCLUDES_DIR}>)# $<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}/png>)
target_link_libraries (pnglib PUBLIC ${ZLIB_LIBRARIES})

if (CCL_SYSTEM_INSTALL)
	if (VENDOR_STATIC_LIBRARY_DESTINATION)
		set (png_destination "${VENDOR_STATIC_LIBRARY_DESTINATION}")
	else ()
		set (png_destination "${VENDOR_LIBRARY_DESTINATION}")		
	endif ()
	install (TARGETS pnglib EXPORT ccl-targets DESTINATION "${png_destination}"
							ARCHIVE DESTINATION "${png_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
							FRAMEWORK DESTINATION "${png_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
							#FILE_SET HEADERS DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/png COMPONENT public_headers
	)
	install (FILES $<TARGET_FILE_DIR:pnglib>/pnglib$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${png_destination}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
endif ()

if (PNG_INCLUDE_DIR AND EXISTS "${PNG_INCLUDE_DIR}/png.h")
	file (STRINGS "${PNG_INCLUDE_DIR}/png.h" png_version_str REGEX "^#define[ \t]+PNG_LIBPNG_VER_STRING[ \t]+\".+\"")
	string (REGEX REPLACE "^#define[ \t]+PNG_LIBPNG_VER_STRING[ \t]+\"([^\"]+)\".*" "\\1" PNG_VERSION_STRING "${png_version_str}")
	unset (png_version_str)
endif ()

set (PNG_LIBRARY pnglib)
set (PNG_LIBRARIES pnglib)
set (PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR} ${PNG_GENERATED_INCLUDES_DIR} ${ZLIB_INCLUDE_DIRS})

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (PNG
	REQUIRED_VARS PNG_LIBRARY PNG_INCLUDE_DIR
	VERSION_VAR PNG_VERSION_STRING
)
