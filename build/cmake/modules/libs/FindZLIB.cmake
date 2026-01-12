option (ZLIB_USE_ZLIB_NG "Use zlib-ng instead of zlib" OFF)

if (ZLIB_USE_ZLIB_NG)
    find_package (zlib-ng)
	set (ZLIB_LIBRARY ${ZLIBNG_LIBRARY})
	set (ZLIB_VERSION_STRING "${ZLIBNG_VERSION_STRING}")
	find_package_handle_standard_args (ZLIB
		REQUIRED_VARS ZLIB_LIBRARY
		VERSION_VAR ZLIB_VERSION_STRING
	)
    return ()
endif ()

ccl_find_path (ZLIB_INCLUDE_DIR NAMES "adler32.c" HINTS "${ZLIB_ROOT}" "${CCL_SUBMODULES_DIR}/zlib" DOC "zlib source directory")

mark_as_advanced (ZLIB_INCLUDE_DIR)
set (ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})

if (TARGET zlib)
	set (ZLIB_LIBRARIES zlib)
	return ()
endif ()

list (APPEND zlib_public_headers
	${ZLIB_INCLUDE_DIR}/zlib.h
	${ZLIB_INCLUDE_DIR}/zconf.h	
)

list (APPEND zlib_sources
    ${ZLIB_INCLUDE_DIR}/adler32.c
    ${ZLIB_INCLUDE_DIR}/compress.c
    ${ZLIB_INCLUDE_DIR}/crc32.c
    ${ZLIB_INCLUDE_DIR}/deflate.c
    ${ZLIB_INCLUDE_DIR}/infback.c
    ${ZLIB_INCLUDE_DIR}/inffast.c
    ${ZLIB_INCLUDE_DIR}/inflate.c
    ${ZLIB_INCLUDE_DIR}/inftrees.c
    ${ZLIB_INCLUDE_DIR}/trees.c
    ${ZLIB_INCLUDE_DIR}/uncompr.c
    ${ZLIB_INCLUDE_DIR}/zutil.c
)
source_group (TREE "${ZLIB_INCLUDE_DIR}" FILES ${zlib_sources} ${zlib_public_headers})

ccl_add_library (zlib STATIC)
ccl_add_library (ZLIB::ZLIB ALIAS zlib)

set_target_properties (zlib PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")

ccl_include_platform_specifics (zlib)

target_sources (zlib PRIVATE ${zlib_sources})
#ccl_target_headers (zlib INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib BASE_DIRS ${ZLIB_INCLUDE_DIR} FILES ${zlib_public_headers})
target_include_directories (zlib PUBLIC $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>)# $<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib>)

if (CCL_SYSTEM_INSTALL)
	if (VENDOR_STATIC_LIBRARY_DESTINATION)
		set (zlib_destination "${VENDOR_STATIC_LIBRARY_DESTINATION}")
	else ()
		set (zlib_destination "${VENDOR_LIBRARY_DESTINATION}")		
	endif ()
	install (TARGETS zlib EXPORT ccl-targets DESTINATION "${zlib_destination}"
						  ARCHIVE DESTINATION "${zlib_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
						  FRAMEWORK DESTINATION "${zlib_destination}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
						  #FILE_SET HEADERS DESTINATION ${VENDOR_PUBLIC_HEADERS_DESTINATION}/zlib COMPONENT public_headers
	)
	install (FILES $<TARGET_FILE_DIR:zlib>/zlib$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${zlib_destination}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
endif ()

set (ZLIB_LIBRARY zlib)
set (ZLIB_LIBRARIES zlib)

if (ZLIB_INCLUDE_DIR AND EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
    file (STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" line REGEX "^#define ZLIB_VERSION \"[^\"]*\"$")
	string (REGEX REPLACE "^#define ZLIB_VERSION \"([^\"]*)\"$" "\\1" ZLIB_VERSION_STRING "${line}")
    file (STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" line REGEX "^#define ZLIB_VER_MAJOR [^\"]*$")
	string (REGEX REPLACE "^#define ZLIB_VER_MAJOR ([^\"]*)$" "\\1" ZLIB_MAJOR_VERSION "${line}")
    file (STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" line REGEX "^#define ZLIB_VER_MINOR [^\"]*$")
	string (REGEX REPLACE "^#define ZLIB_VER_MINOR ([^\"]*)$" "\\1" ZLIB_MINOR_VERSION "${line}")
    file (STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" line REGEX "^#define ZLIB_VER_REVISION [^\"]*$")
	string (REGEX REPLACE "^#define ZLIB_VER_REVISION ([^\"]*)$" "\\1" ZLIB_PATCH_VERSION "${line}")
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (ZLIB
	REQUIRED_VARS ZLIB_LIBRARY
	VERSION_VAR ZLIB_VERSION_STRING
)
