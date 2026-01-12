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
# Filename    : cclnet.cmake
# Description : CCL network library
#
#************************************************************************************************

set (cclnet_exports
	CCLModuleMain
	${CCL_EXPORT_PREFIX}GetNetwork${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetDiscoveryHandler${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetWebService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetWebFileService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetTransferManager${CCL_EXPORT_POSTFIX}
)

# collect source files
ccl_list_append_once (cclnet_main_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
)

ccl_list_append_once (cclnet_public_sources
	${CCL_DIR}/public/cclnetiids.cpp
)

ccl_list_append_once (cclnet_public_headers
	${CCL_DIR}/public/network/inetdiscovery.h
	${CCL_DIR}/public/network/inetwork.h
	${CCL_DIR}/public/network/isocket.h
	${CCL_DIR}/public/network/web/httpstatus.h
	${CCL_DIR}/public/network/web/iwebnewsreader.h
	${CCL_DIR}/public/network/web/iwebclient.h
	${CCL_DIR}/public/network/web/iwebcredentials.h
	${CCL_DIR}/public/network/web/iwebfileclient.h
	${CCL_DIR}/public/network/web/iwebprotocol.h
	${CCL_DIR}/public/network/web/iwebrequest.h
	${CCL_DIR}/public/network/web/iwebserver.h
	${CCL_DIR}/public/network/web/iwebservice.h
	${CCL_DIR}/public/network/web/iwebsocket.h
	${CCL_DIR}/public/network/web/ixmlhttprequest.h
	${CCL_DIR}/public/network/web/itransfermanager.h
	${CCL_DIR}/public/network/web/iwebfileservice.h
	${CCL_DIR}/public/network/web/iwebfiletask.h

	${CCL_DIR}/public/netservices.h
)

ccl_list_append_once (cclnet_public_source_files
	${CCL_DIR}/public/base/debug.cpp
	${CCL_DIR}/public/base/debug.h
	${CCL_DIR}/public/base/iunknown.cpp
	${CCL_DIR}/public/base/iunknown.h
	${CCL_DIR}/public/base/unknown.cpp
	${CCL_DIR}/public/base/unknown.h
	${CCL_DIR}/public/base/variant.cpp
	${CCL_DIR}/public/base/variant.h

	${CCL_DIR}/public/system/ithreading.h
	${CCL_DIR}/public/system/threadsync.cpp
	${CCL_DIR}/public/system/threadsync.h

	${CCL_DIR}/public/text/cclstring.cpp
	${CCL_DIR}/public/text/cclstring.h
	${CCL_DIR}/public/text/cstring.h
)

ccl_list_append_once (cclnet_source_files
	${CCL_DIR}/network/netdiscovery.cpp
	${CCL_DIR}/network/netdiscovery.h
	${CCL_DIR}/network/netservices.cpp
	${CCL_DIR}/network/netsocket.cpp
	${CCL_DIR}/network/netsocket.h
	${CCL_DIR}/network/netsslsocket.cpp
	${CCL_DIR}/network/netsslsocket.h
	${CCL_DIR}/network/netstream.cpp
	${CCL_DIR}/network/netstream.h
	${CCL_DIR}/network/network.cpp
	${CCL_DIR}/network/network.h

	${CCL_DIR}/network/web/http/client.cpp
	${CCL_DIR}/network/web/http/client.h
	${CCL_DIR}/network/web/http/request.cpp
	${CCL_DIR}/network/web/http/request.h
	${CCL_DIR}/network/web/http/server.cpp
	${CCL_DIR}/network/web/http/server.h
	${CCL_DIR}/network/web/localclient.cpp
	${CCL_DIR}/network/web/localclient.h
	${CCL_DIR}/network/web/webnewsreader.cpp
	${CCL_DIR}/network/web/webnewsreader.h
	${CCL_DIR}/network/web/webclient.cpp
	${CCL_DIR}/network/web/webclient.h
	${CCL_DIR}/network/web/webrequest.cpp
	${CCL_DIR}/network/web/webrequest.h
	${CCL_DIR}/network/web/webserver.cpp
	${CCL_DIR}/network/web/webserver.h
	${CCL_DIR}/network/web/webservice.cpp
	${CCL_DIR}/network/web/webservice.h
	${CCL_DIR}/network/web/websocket.cpp
	${CCL_DIR}/network/web/websocket.h
	${CCL_DIR}/network/web/xmlhttprequest.cpp
	${CCL_DIR}/network/web/xmlhttprequest.h
	${CCL_DIR}/network/web/xmlnewsreader.cpp
	${CCL_DIR}/network/web/xmlnewsreader.h
	${CCL_DIR}/network/web/transfermanager.cpp
	${CCL_DIR}/network/web/transfermanager.h
	
	${CCL_DIR}/network/webfs/webfilesearcher.cpp
	${CCL_DIR}/network/webfs/webfilesearcher.h
	${CCL_DIR}/network/webfs/webfileservice.cpp
	${CCL_DIR}/network/webfs/webfileservice.h
	${CCL_DIR}/network/webfs/webfilesession.cpp
	${CCL_DIR}/network/webfs/webfilesession.h
	${CCL_DIR}/network/webfs/webfilesystem.cpp
	${CCL_DIR}/network/webfs/webfilesystem.h
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclnet_SOURCE_DIR)
	source_group ("main" FILES ${cclnet_main_sources})
	source_group (TREE ${CCL_DIR}/public PREFIX "public" FILES ${cclnet_public_source_files} ${cclnet_public_sources} ${cclnet_public_headers})
	source_group (TREE ${CCL_DIR}/network PREFIX "source" FILES ${cclnet_source_files})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclnet_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclbase)
include (cclsystem)
include (ccltext)

set (cclnet "cclnet")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclnet ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
ccl_read_version (${cclnet} "${CCL_DIR}/public/cclversion.h" "CCLNET")
if (NOT TARGET ${cclnet})
	if (${CCL_STATIC_ONLY})
		ccl_add_library (cclnet INTERFACE POSTFIX "${CCL_ISOLATION_POSTFIX}")
	else ()
		ccl_add_library (cclnet SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set_target_properties (${cclnet} PROPERTIES FOLDER "ccl/${CCL_ISOLATION_POSTFIX}")
	endif ()

	# collect source files for target
	ccl_list_append_once (cclnet_sources
		${cclnet_main_sources}
		${cclnet_public_source_files}
		${cclnet_public_sources}
		${cclnet_source_files}
	)

	if (NOT ${CCL_STATIC_ONLY})
		target_sources (${cclnet} PRIVATE ${cclnet_sources} ${CMAKE_CURRENT_LIST_FILE})
		ccl_target_headers (${cclnet} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclnet_public_headers})

		target_link_libraries (${cclnet} PUBLIC ${cclbase} ${cclsystem} ${ccltext} PRIVATE corelib)
		target_include_directories (${cclnet} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")
		
		ccl_export_symbols (${cclnet} ${cclnet_exports})
		
		ccl_add_resources (${cclnet} ${cclnet_resources})

		if (CCL_SYSTEM_INSTALL)
			set_target_properties (${cclnet} PROPERTIES SOVERSION ${CCL_VERSION})
			install (TARGETS ${cclnet} EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
									   LIBRARY DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   RUNTIME DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   ARCHIVE DESTINATION "${CCL_IMPORT_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FRAMEWORK DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
			)
			install (FILES ${cclnet_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
			install (FILES $<TARGET_FILE_DIR:${cclnet}>/${cclnet}.pdb DESTINATION "${CCL_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
		elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
			install (TARGETS ${cclnet} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
									   RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
									   FRAMEWORK DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
			)
		endif ()
	endif ()
	list (REMOVE_DUPLICATES cclnet_public_sources)
	target_sources (${cclnet} INTERFACE $<BUILD_INTERFACE:${cclnet_public_sources}> $<INSTALL_INTERFACE:${CCL_PUBLIC_HEADERS_DESTINATION}/public/cclnetiids.cpp>)
elseif (NOT XCODE)
	ccl_include_platform_specifics (cclnet)
endif ()

ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS CCL_STATIC_ENABLE_NETWORK=1)
