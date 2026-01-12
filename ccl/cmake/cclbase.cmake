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
# Filename    : cclbase.cmake
# Description : CCL Base library
#
#************************************************************************************************

# collect source files
ccl_list_append_once (cclbase_core_sources 
	${corelib_public_sources}
)

ccl_list_append_once (cclbase_public_sources
	${CCL_DIR}/public/cclbaseiids.cpp
)

ccl_list_append_once (cclbase_public_headers
	${CCL_DIR}/public/base/datetime.h
	${CCL_DIR}/public/base/debug.h
	${CCL_DIR}/public/base/enumdef.h
	${CCL_DIR}/public/base/ccldefpop.h
	${CCL_DIR}/public/base/ccldefpush.h
	${CCL_DIR}/public/base/iactivatable.h
	${CCL_DIR}/public/base/iarrayobject.h
	${CCL_DIR}/public/base/iasyncoperation.h
	${CCL_DIR}/public/base/iconverter.h
	${CCL_DIR}/public/base/iextensible.h
	${CCL_DIR}/public/base/iformatter.h
	${CCL_DIR}/public/base/imessage.h
	${CCL_DIR}/public/base/iobject.h
	${CCL_DIR}/public/base/iobjectnode.h
	${CCL_DIR}/public/base/iobserver.h
	${CCL_DIR}/public/base/irecognizer.h
	${CCL_DIR}/public/base/itrigger.h
	${CCL_DIR}/public/base/itypelib.h
	${CCL_DIR}/public/base/iunknown.h
	${CCL_DIR}/public/base/platform.h
	${CCL_DIR}/public/base/primitives.h
	${CCL_DIR}/public/base/profiler.h
	${CCL_DIR}/public/base/smartptr.h
	${CCL_DIR}/public/base/uid.h
	${CCL_DIR}/public/base/uiddef.h
	${CCL_DIR}/public/base/cclmacros.h
	${CCL_DIR}/public/base/unknown.h
	${CCL_DIR}/public/base/variant.h
	${CCL_DIR}/public/base/iunittest.h
	${CCL_DIR}/public/base/buffer.h
	${CCL_DIR}/public/base/ibuffer.h	
	${CCL_DIR}/public/base/idatatransformer.h
	${CCL_DIR}/public/base/iprogress.h
	${CCL_DIR}/public/base/istream.h
	${CCL_DIR}/public/base/memorystream.h
	${CCL_DIR}/public/base/multiplexstream.h
	${CCL_DIR}/public/base/streamer.h	
	${CCL_DIR}/public/collections/bitset.h
	${CCL_DIR}/public/collections/bufferchain.h
	${CCL_DIR}/public/collections/deque.h
	${CCL_DIR}/public/collections/hashmap.h
	${CCL_DIR}/public/collections/hashtable.h
	${CCL_DIR}/public/collections/intrusivelist.h
	${CCL_DIR}/public/collections/iunknownlist.h
	${CCL_DIR}/public/collections/linkedlist.h
	${CCL_DIR}/public/collections/map.h
	${CCL_DIR}/public/collections/ringbuffer.h
	${CCL_DIR}/public/collections/stack.h
	${CCL_DIR}/public/collections/treeset.h
	${CCL_DIR}/public/collections/unknownlist.h
	${CCL_DIR}/public/collections/variantvector.h
	${CCL_DIR}/public/collections/vector.h
	${CCL_DIR}/public/storage/filetype.h
	${CCL_DIR}/public/storage/iattributelist.h
	${CCL_DIR}/public/storage/iconfiguration.h
	${CCL_DIR}/public/storage/ifileresource.h
	${CCL_DIR}/public/storage/ipersistattributes.h
	${CCL_DIR}/public/storage/istorage.h
	${CCL_DIR}/public/storage/iurl.h
	${CCL_DIR}/public/storage/ixmltree.h
	${CCL_DIR}/public/storage/metainfo.h
	${CCL_DIR}/public/system/cryptotypes.h
	${CCL_DIR}/public/system/icryptor.h	
	${CCL_DIR}/public/system/ifileitem.h
	${CCL_DIR}/public/system/ifilesystem.h
	${CCL_DIR}/public/system/ikeyprovider.h
	${CCL_DIR}/public/math/mathprimitives.h
	${CCL_DIR}/public/math/rational.h
	${CCL_DIR}/public/cclexports.h
	${CCL_DIR}/public/cclversion.h
)

ccl_list_append_once (cclbase_api_headers
	${CCL_DIR}/base/asyncoperation.h
	${CCL_DIR}/base/boxedtypes.h
	${CCL_DIR}/base/collections/arraybox.h
	${CCL_DIR}/base/collections/container.h
	${CCL_DIR}/base/collections/linkablelist.h
	${CCL_DIR}/base/collections/objectarray.h
	${CCL_DIR}/base/collections/objecthashtable.h
	${CCL_DIR}/base/collections/objectlist.h
	${CCL_DIR}/base/collections/objectstack.h
	${CCL_DIR}/base/collections/observedlist.h
	${CCL_DIR}/base/collections/stringdictionary.h
	${CCL_DIR}/base/collections/stringlist.h
	${CCL_DIR}/base/development.h
	${CCL_DIR}/base/initterm.h
	${CCL_DIR}/base/kernel.h
	${CCL_DIR}/base/math/mathcurve.h
	${CCL_DIR}/base/math/mathpoint.h
	${CCL_DIR}/base/math/mathrange.h
	${CCL_DIR}/base/math/mathrangelist.h
	${CCL_DIR}/base/math/mathregion.h
	${CCL_DIR}/base/memorypool.h
	${CCL_DIR}/base/message.h
	${CCL_DIR}/base/metaclass.h
	${CCL_DIR}/base/object.h
	${CCL_DIR}/base/objectconverter.h
	${CCL_DIR}/base/objectmacros.h
	${CCL_DIR}/base/objectnode.h
	${CCL_DIR}/base/performance.h
	${CCL_DIR}/base/security/cipher.h
	${CCL_DIR}/base/security/classauthorizer.h
	${CCL_DIR}/base/security/cryptobox.h
	${CCL_DIR}/base/security/cryptomaterial.h
	${CCL_DIR}/base/security/featureauthorizer.h
	${CCL_DIR}/base/security/fingerprint.h
	${CCL_DIR}/base/security/packagesignature.h
	${CCL_DIR}/base/security/policybuilder.h
	${CCL_DIR}/base/security/signature.h
	${CCL_DIR}/base/signalslot.h
	${CCL_DIR}/base/signalsource.h
	${CCL_DIR}/base/singleton.h
	${CCL_DIR}/base/storage/archive.h
	${CCL_DIR}/base/storage/archivehandler.h
	${CCL_DIR}/base/storage/attributes.h
	${CCL_DIR}/base/storage/binaryarchive.h
	${CCL_DIR}/base/storage/compressionhandler.h
	${CCL_DIR}/base/storage/configuration.h
	${CCL_DIR}/base/storage/expressionparser.h
	${CCL_DIR}/base/storage/file.h
	${CCL_DIR}/base/storage/filefilter.h
	${CCL_DIR}/base/storage/fileresource.h
	${CCL_DIR}/base/storage/isettings.h
	${CCL_DIR}/base/storage/jsonarchive.h
	${CCL_DIR}/base/storage/logfile.h
	${CCL_DIR}/base/storage/objectpackage.h
	${CCL_DIR}/base/storage/packageinfo.h
	${CCL_DIR}/base/storage/propertyfile.h
	${CCL_DIR}/base/storage/protocolhandler.h
	${CCL_DIR}/base/storage/settings.h
	${CCL_DIR}/base/storage/storableobject.h
	${CCL_DIR}/base/storage/storage.h
	${CCL_DIR}/base/storage/stringtemplate.h
	${CCL_DIR}/base/storage/textfile.h
	${CCL_DIR}/base/storage/textparser.h
	${CCL_DIR}/base/storage/url.h
	${CCL_DIR}/base/storage/urlencoder.h
	${CCL_DIR}/base/storage/xmlarchive.h
	${CCL_DIR}/base/storage/xmlpihandler.h
	${CCL_DIR}/base/storage/xmltree.h
	${CCL_DIR}/base/storage/persistence/dataitem.h
	${CCL_DIR}/base/storage/persistence/datastore.h
	${CCL_DIR}/base/storage/persistence/expression.h
	${CCL_DIR}/base/storage/persistence/persistence.h
	${CCL_DIR}/base/storage/persistence/sqlclient.h
	${CCL_DIR}/base/trigger.h
	${CCL_DIR}/base/typelib.h
	${CCL_DIR}/base/unittest.h
)

ccl_list_append_once (cclbase_public_source_files
	${CCL_DIR}/public/base/datetime.cpp
	${CCL_DIR}/public/base/debug.cpp
	${CCL_DIR}/public/base/debug.output.cpp
	${CCL_DIR}/public/base/iasyncoperation.cpp
	${CCL_DIR}/public/base/iformatter.cpp
	${CCL_DIR}/public/base/iobserver.cpp
	${CCL_DIR}/public/base/iunknown.cpp
	${CCL_DIR}/public/base/primitives.cpp
	${CCL_DIR}/public/base/uid.cpp
	${CCL_DIR}/public/base/unknown.cpp
	${CCL_DIR}/public/base/variant.cpp
	${CCL_DIR}/public/base/multiplexstream.cpp
	${CCL_DIR}/public/base/streamer.cpp
	${CCL_DIR}/public/collections/unknownlist.cpp
	${CCL_DIR}/public/storage/filetype.cpp
	${CCL_DIR}/public/math/mathprimitives.cpp
	${CCL_DIR}/public/plugins/classfactory.cpp
	${CCL_DIR}/public/plugins/plugmetaclass.cpp
	${CCL_DIR}/public/plugins/serviceplugin.cpp
	${CCL_DIR}/public/plugins/stubobject.cpp
	${CCL_DIR}/public/plugins/versionnumber.cpp
	${CCL_DIR}/public/system/logging.cpp
	${CCL_DIR}/public/system/alerttypes.cpp
	${CCL_DIR}/public/system/cclanalytics.cpp
	${CCL_DIR}/public/system/cclerror.cpp
	${CCL_DIR}/public/system/cclsafety.cpp
	${CCL_DIR}/public/system/formatter.cpp
	${CCL_DIR}/public/system/threadsync.cpp
	${CCL_DIR}/public/system/userthread.cpp
	${CCL_DIR}/public/text/cclstring.cpp
	${CCL_DIR}/public/text/cstring.cpp
	${CCL_DIR}/public/text/stringbuilder.cpp
	${CCL_DIR}/public/text/translation.cpp
	${CCL_DIR}/public/text/xmlcontentparser.cpp
)

ccl_list_append_once (cclbase_main_headers
	${CCL_DIR}/main/cclargs.h
)
ccl_list_append_once (cclbase_main_sources
	${CCL_DIR}/main/cclargs.cpp
)

ccl_list_append_once (cclbase_source_files
	${CCL_DIR}/base/asyncoperation.cpp
	${CCL_DIR}/base/boxedtypes.cpp
	${CCL_DIR}/base/collections/arraybox.cpp
	${CCL_DIR}/base/collections/container.cpp
	${CCL_DIR}/base/collections/linkablelist.cpp
	${CCL_DIR}/base/collections/objectarray.cpp
	${CCL_DIR}/base/collections/objecthashtable.cpp
	${CCL_DIR}/base/collections/objectlist.cpp
	${CCL_DIR}/base/collections/stringdictionary.cpp
	${CCL_DIR}/base/collections/stringlist.cpp
	${CCL_DIR}/base/development.cpp
	${CCL_DIR}/base/kernel.cpp
	${CCL_DIR}/base/math/mathcurve.cpp
	${CCL_DIR}/base/math/mathpoint.cpp
	${CCL_DIR}/base/math/mathregion.cpp
	${CCL_DIR}/base/memorypool.cpp
	${CCL_DIR}/base/message.cpp
	${CCL_DIR}/base/metaclass.cpp
	${CCL_DIR}/base/object.cpp
	${CCL_DIR}/base/objectconverter.cpp
	${CCL_DIR}/base/objectnode.cpp
	${CCL_DIR}/base/performance.cpp
	${CCL_DIR}/base/security/cipher.cpp
	${CCL_DIR}/base/security/classauthorizer.cpp
	${CCL_DIR}/base/security/cryptobox.cpp
	${CCL_DIR}/base/security/cryptomaterial.cpp
	${CCL_DIR}/base/security/featureauthorizer.cpp
	${CCL_DIR}/base/security/fingerprint.cpp
	${CCL_DIR}/base/security/packagesignature.cpp
	${CCL_DIR}/base/security/signature.cpp
	${CCL_DIR}/base/signalslot.cpp
	${CCL_DIR}/base/signalsource.cpp
	${CCL_DIR}/base/storage/archive.cpp
	${CCL_DIR}/base/storage/archive.h
	${CCL_DIR}/base/storage/archivehandler.cpp
	${CCL_DIR}/base/storage/attributes.cpp
	${CCL_DIR}/base/storage/binaryarchive.cpp
	${CCL_DIR}/base/storage/compressionhandler.cpp
	${CCL_DIR}/base/storage/configuration.cpp
	${CCL_DIR}/base/storage/expressionparser.cpp
	${CCL_DIR}/base/storage/file.cpp
	${CCL_DIR}/base/storage/filefilter.cpp
	${CCL_DIR}/base/storage/fileresource.cpp
	${CCL_DIR}/base/storage/jsonarchive.cpp
	${CCL_DIR}/base/storage/logfile.cpp
	${CCL_DIR}/base/storage/objectpackage.cpp
	${CCL_DIR}/base/storage/packageinfo.cpp
	${CCL_DIR}/base/storage/propertyfile.cpp
	${CCL_DIR}/base/storage/protocolhandler.cpp
	${CCL_DIR}/base/storage/settings.cpp
	${CCL_DIR}/base/storage/storableobject.cpp
	${CCL_DIR}/base/storage/stringtemplate.cpp
	${CCL_DIR}/base/storage/textfile.cpp
	${CCL_DIR}/base/storage/textparser.cpp
	${CCL_DIR}/base/storage/url.cpp
	${CCL_DIR}/base/storage/urlencoder.cpp
	${CCL_DIR}/base/storage/xmlarchive.cpp
	${CCL_DIR}/base/storage/xmlpihandler.cpp
	${CCL_DIR}/base/storage/xmltree.cpp
	${CCL_DIR}/base/storage/persistence/dataitem.cpp
	${CCL_DIR}/base/storage/persistence/datastore.cpp
	${CCL_DIR}/base/storage/persistence/expression.cpp
	${CCL_DIR}/base/storage/persistence/sqlclient.cpp
	${CCL_DIR}/base/trigger.cpp
	${CCL_DIR}/base/typelib.cpp
	${CCL_DIR}/base/unittest.cpp
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclbase_SOURCE_DIR)
	source_group ("source\\main" FILES ${cclbase_main_sources})
	source_group (TREE ${CCL_DIR}/public PREFIX "public" FILES ${cclbase_public_sources} ${cclbase_public_source_files} ${cclbase_public_headers})
	source_group (TREE ${CCL_DIR}/base PREFIX "source" FILES ${cclbase_source_files} ${cclbase_api_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclbase_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

# Find Core
ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package (corelib REQUIRED)

set (cclbase "cclbase")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclbase ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclbase})
	ccl_add_library (cclbase STATIC VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")

	# collect source files for target
	ccl_list_append_once (cclbase_sources
		${cclbase_core_sources}
		${cclbase_main_sources}
		${cclbase_public_sources}
		${cclbase_public_source_files}
		${cclbase_source_files}
	)

	cmake_path (RELATIVE_PATH CCL_REPOSITORY_ROOT BASE_DIRECTORY "${REPOSITORY_ROOT}" OUTPUT_VARIABLE ccl_framework_dir)

	set_source_files_properties (${CCL_DIR}/base/development.cpp PROPERTIES COMPILE_DEFINITIONS "CCL_TOPLEVEL_DIRECTORY=\"${REPOSITORY_ROOT}\"")

	target_sources (${cclbase} PRIVATE ${cclbase_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclbase} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclbase_public_headers} ${cclbase_api_headers})
	
	target_compile_definitions (${cclbase} PRIVATE ${corelib_COMPILE_DEFINITIONS} PUBLIC "CCL_FRAMEWORK_DIRECTORY=\"${ccl_framework_dir}/\"")
	target_link_libraries (${cclbase} INTERFACE corelib)
	target_include_directories (${cclbase} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")

	if (CCL_SYSTEM_INSTALL)
		install (TARGETS ${cclbase} EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
									ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclbase_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclbase}>/${cclbase}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()

endif ()

ccl_list_append_once (CCL_IS_STATIC ${cclbase})
