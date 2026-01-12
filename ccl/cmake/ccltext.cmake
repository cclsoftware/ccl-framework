#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : ccltext.cmake
# Description : CCL text library
#
#************************************************************************************************

set (ccltext_exports
	CCLModuleMain
	${CCL_EXPORT_PREFIX}GetEmptyString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetConstantString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetUnicodeUtilities${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateTranslationTable${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateStringDictionary${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}ParseVariantString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateRegularExpression${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateMutableCString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetConstantCString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateCStringDictionary${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateXmlParser${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateXmlWriter${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}JsonParse${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}JsonStringify${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}Json5Parse${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}Json5Stringify${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}UBJsonParse${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}UBJsonWrite${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateDataTransformer${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateTransformStream${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}Crc32${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateTextStreamer${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateTextWriter${CCL_EXPORT_POSTFIX}
	core_malloc
	core_malloc_debug
	core_realloc
	core_realloc_debug
	core_free
	core_alloc_use
	core_alloc_unuse
	core_check_heap
	core_check_ptr
)

# collect source files
ccl_list_append_once (ccltext_public_headers
	${CCL_DIR}/public/text/cclstring.h
	${CCL_DIR}/public/text/cclstdstring.h
	${CCL_DIR}/public/text/cstring.h
	${CCL_DIR}/public/text/iattributehandler.h
	${CCL_DIR}/public/text/ihtmlwriter.h
	${CCL_DIR}/public/text/iregexp.h
	${CCL_DIR}/public/text/istring.h
	${CCL_DIR}/public/text/istringdict.h
	${CCL_DIR}/public/text/istringprivate.h
	${CCL_DIR}/public/text/itextbuilder.h
	${CCL_DIR}/public/text/itextstreamer.h
	${CCL_DIR}/public/text/itextwriter.h
	${CCL_DIR}/public/text/itranslationtable.h
	${CCL_DIR}/public/text/ixmlparser.h
	${CCL_DIR}/public/text/ixmlwriter.h
	${CCL_DIR}/public/text/language.h
	${CCL_DIR}/public/text/stringbuilder.h
	${CCL_DIR}/public/text/textencoding.h
	${CCL_DIR}/public/text/translation.h
	${CCL_DIR}/public/text/translationformat.h
	${CCL_DIR}/public/text/xmlcontentparser.h
	
	${CCL_DIR}/public/textservices.h
)

ccl_list_append_once (ccltext_public_source_files
	${CCL_DIR}/public/base/debug.cpp
	${CCL_DIR}/public/base/debug.h
	${CCL_DIR}/public/base/debug.output.cpp
	${CCL_DIR}/public/base/iunknown.cpp
	${CCL_DIR}/public/base/iunknown.h
	${CCL_DIR}/public/base/platform.h
	${CCL_DIR}/public/base/unknown.cpp
	${CCL_DIR}/public/base/unknown.h
	${CCL_DIR}/public/base/variant.cpp
	${CCL_DIR}/public/base/variant.h
	${CCL_DIR}/public/base/buffer.h
	${CCL_DIR}/public/base/idatatransformer.h
	${CCL_DIR}/public/base/istream.h
	${CCL_DIR}/public/base/streamer.cpp
	${CCL_DIR}/public/base/streamer.h

	${CCL_DIR}/public/text/cclstring.cpp
	${CCL_DIR}/public/text/cstring.cpp
)

ccl_list_append_once (ccltext_system_source_files
	${CCL_DIR}/system/allocator.cpp
	${CCL_DIR}/system/allocator.h
	${CCL_DIR}/system/threading/atomic.cpp
)

ccl_list_append_once (ccltext_source_files
	${CCL_DIR}/text/cclmalloc.cpp

	${CCL_DIR}/text/strings/cstringbuffer.cpp
	${CCL_DIR}/text/strings/cstringbuffer.h
	${CCL_DIR}/text/strings/formatparser.cpp
	${CCL_DIR}/text/strings/formatparser.h
	${CCL_DIR}/text/strings/jsonhandler.cpp
	${CCL_DIR}/text/strings/jsonhandler.h
	${CCL_DIR}/text/strings/regularexpression.cpp
	${CCL_DIR}/text/strings/regularexpression.h
	${CCL_DIR}/text/strings/stringstats.h
	${CCL_DIR}/text/strings/stringtable.cpp
	${CCL_DIR}/text/strings/stringtable.h
	${CCL_DIR}/text/strings/textdictionary.h
	${CCL_DIR}/text/strings/translationtable.cpp
	${CCL_DIR}/text/strings/translationtable.h
	${CCL_DIR}/text/strings/unicode-cross-platform/ascii-encode.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/charset-lowercase.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/charset-numeric.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/charset-uppercase.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/charset-whitespace.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/cp437-decode.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/cp437-encode.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/latin1-encode.inc
	${CCL_DIR}/text/strings/unicode-cross-platform/ucharfunctions.cpp
	${CCL_DIR}/text/strings/unicode-cross-platform/ucharfunctions.h
	${CCL_DIR}/text/strings/unicodestring.cpp
	${CCL_DIR}/text/strings/unicodestring.h

	${CCL_DIR}/text/textservices.cpp

	${CCL_DIR}/text/transform/encodings/baseencoding.cpp
	${CCL_DIR}/text/transform/encodings/baseencoding.h
	${CCL_DIR}/text/transform/encodings/cstringencoding.cpp
	${CCL_DIR}/text/transform/encodings/cstringencoding.h
	${CCL_DIR}/text/transform/encodings/textencoding.cpp
	${CCL_DIR}/text/transform/encodings/textencoding.h
	${CCL_DIR}/text/transform/encodings/utfencoding.cpp
	${CCL_DIR}/text/transform/encodings/utfencoding.h
	${CCL_DIR}/text/transform/textstreamer.cpp
	${CCL_DIR}/text/transform/textstreamer.h
	${CCL_DIR}/text/transform/transformstreams.cpp
	${CCL_DIR}/text/transform/transformstreams.h
	${CCL_DIR}/text/transform/zlibcompression.cpp
	${CCL_DIR}/text/transform/zlibcompression.h

	${CCL_DIR}/text/writer/htmlwriter.cpp
	${CCL_DIR}/text/writer/htmlwriter.h
	${CCL_DIR}/text/writer/markupencoder.h
	${CCL_DIR}/text/writer/plaintextwriter.cpp
	${CCL_DIR}/text/writer/plaintextwriter.h
	${CCL_DIR}/text/writer/textbuilder.cpp
	${CCL_DIR}/text/writer/textbuilder.h
	${CCL_DIR}/text/writer/textwriter.cpp
	${CCL_DIR}/text/writer/textwriter.h

	${CCL_DIR}/text/xml/xmlentities.cpp
	${CCL_DIR}/text/xml/xmlentities.h
	${CCL_DIR}/text/xml/xmlparser.cpp
	${CCL_DIR}/text/xml/xmlparser.h
	${CCL_DIR}/text/xml/xmlstringdict.cpp
	${CCL_DIR}/text/xml/xmlstringdict.h
	${CCL_DIR}/text/xml/xmlwriter.cpp
	${CCL_DIR}/text/xml/xmlwriter.h
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ccltext_SOURCE_DIR)
	source_group (TREE ${CCL_DIR}/public PREFIX "public" FILES ${ccltext_public_source_files} ${ccltext_public_headers})
	source_group (TREE ${CCL_DIR}/text PREFIX "source" FILES ${ccltext_source_files})
	#source_group (TREE ${CCL_DIR}/system PREFIX "source/system" FILES ${ccltext_system_source_files})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (ccltext_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

# Find Core
ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package (corelib REQUIRED)
find_package (pcre REQUIRED)
find_package (expat REQUIRED)

# Options
option (CCL_PRINT_STRING_STATS "Print the string statistics for debug builds." ON)
if (CCL_PRINT_STRING_STATS)
	set (print_string_stats 1)
else ()
	set (print_string_stats 0)
endif ()

set (ccltext "ccltext")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND ccltext ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
ccl_read_version (${ccltext} "${CCL_DIR}/public/cclversion.h" "CCLTEXT")
if (NOT TARGET ${ccltext})
	if (${CCL_STATIC_ONLY})
		ccl_add_library (ccltext INTERFACE POSTFIX "${CCL_ISOLATION_POSTFIX}")
	else ()
		ccl_add_library (ccltext SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set_target_properties (${ccltext} PROPERTIES FOLDER "ccl/${CCL_ISOLATION_POSTFIX}")
	endif ()

	# Preprocessor Definitions
	if (${CCL_STATIC_ONLY})
		target_compile_definitions (${ccltext} INTERFACE "CCL_PRINT_STRING_STATS=${print_string_stats}")
	else ()
		target_compile_definitions (${ccltext} PRIVATE "CCL_PRINT_STRING_STATS=${print_string_stats}")
	endif ()

	# collect source files for target
	ccl_list_append_once (ccltext_sources
		${ccltext_public_source_files}
		${ccltext_source_files}
		${ccltext_system_source_files}
	)

	if (NOT ${CCL_STATIC_ONLY})
		target_sources (${ccltext} PRIVATE ${ccltext_sources} ${CMAKE_CURRENT_LIST_FILE})
		ccl_target_headers (${ccltext} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${ccltext_public_headers})

		target_link_libraries (${ccltext} PRIVATE corelib ${expat_LIBRARY} ${PCRE_LIBRARY})
		target_include_directories (${ccltext} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")

		ccl_export_symbols (${ccltext} ${ccltext_exports})
		
		ccl_add_resources (${ccltext} ${ccltext_resources})

		if (CCL_SYSTEM_INSTALL)
			set_target_properties (${ccltext} PROPERTIES SOVERSION ${CCL_VERSION})
			install (TARGETS ${ccltext} EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
										LIBRARY DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										RUNTIME DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										ARCHIVE DESTINATION "${CCL_IMPORT_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										FRAMEWORK DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
			)
			install (FILES $<TARGET_FILE_DIR:${ccltext}>/${ccltext}.pdb DESTINATION "${CCL_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
		elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
			install (TARGETS ${ccltext} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
										RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
										FRAMEWORK DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
			)
		endif ()
	endif ()
elseif (NOT XCODE)
	ccl_include_platform_specifics (ccltext)
endif ()

ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${expat_LIBRARY} ${PCRE_LIBRARY})
ccl_list_append_once (CCL_STATIC_INCLUDE_DIRS ${expat_INCLUDE_DIR} ${PCRE_INCLUDE_DIR})
ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS PCRE2_STATIC=1 PCRE2_CODE_UNIT_WIDTH=16)
