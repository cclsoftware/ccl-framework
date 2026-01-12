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
# Filename    : cclsecurity.cmake
# Description : CCL security library
#
#************************************************************************************************

set (cclsecurity_exports
	CCLModuleMain
	${CCL_EXPORT_PREFIX}GetCryptoService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetCryptoKeyStore${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetCryptoFactory${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetAuthorizationManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetCredentialManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateNameBasedUID${CCL_EXPORT_POSTFIX}
)

# collect source files
ccl_list_append_once (cclsecurity_main_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/main/cclmodmain.empty.cpp
)

ccl_list_append_once (cclsecurity_public_headers	
	${CCL_DIR}/public/security/iasn1contenthandler.h
	${CCL_DIR}/public/security/iauthorizationmanager.h
	${CCL_DIR}/public/security/iauthorizationpolicy.h
	${CCL_DIR}/public/security/icredentialmanager.h
	${CCL_DIR}/public/security/icryptointeger.h
	${CCL_DIR}/public/security/icryptokeystore.h
	${CCL_DIR}/public/security/icryptoservice.h

	${CCL_DIR}/public/securityservices.h
)

ccl_list_append_once (cclsecurity_source_files
	${CCL_DIR}/security/authorizationmanager.cpp
	${CCL_DIR}/security/authorizationmanager.h
	${CCL_DIR}/security/authorizationpolicy.cpp
	${CCL_DIR}/security/authorizationpolicy.h
	${CCL_DIR}/security/credentialmanager.cpp
	${CCL_DIR}/security/credentialmanager.h
	${CCL_DIR}/security/cryptofactory.cpp
	${CCL_DIR}/security/cryptofactory.h
	${CCL_DIR}/security/cryptokeystore.cpp
	${CCL_DIR}/security/cryptokeystore.h
	${CCL_DIR}/security/cryptoppglue.cpp
	${CCL_DIR}/security/cryptoppglue.h
	${CCL_DIR}/security/cryptor.cpp
	${CCL_DIR}/security/cryptor.h
	${CCL_DIR}/security/cryptoservice.cpp
	${CCL_DIR}/security/cryptoservice.h
	${CCL_DIR}/security/securityhost.cpp
	${CCL_DIR}/security/securityhost.h
	${CCL_DIR}/security/securityservices.cpp
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclsecurity_SOURCE_DIR)
	source_group ("public\\security" FILES ${cclsecurity_public_headers})
	source_group ("source" FILES ${cclsecurity_source_files})
	source_group ("main" FILES ${cclsecurity_main_sources})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclsecurity_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclbase)
include (cclsystem)
include (ccltext)

set (cclsecurity "cclsecurity")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclsecurity ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
ccl_read_version (${cclsecurity} "${CCL_DIR}/public/cclversion.h" "CCLSECURITY")
if (NOT TARGET ${cclsecurity})
	find_package (cryptlib REQUIRED)

	if (${CCL_STATIC_ONLY})
		ccl_add_library (cclsecurity INTERFACE POSTFIX "${CCL_ISOLATION_POSTFIX}")
	else ()
		ccl_add_library (cclsecurity SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set_target_properties (${cclsecurity} PROPERTIES FOLDER "ccl/${CCL_ISOLATION_POSTFIX}")
	endif ()

	# collect source files for target
	ccl_list_append_once (cclsecurity_sources
		${cclsecurity_main_sources}
		${cclsecurity_source_files}
	)

	if (NOT ${CCL_STATIC_ONLY})
		target_sources (${cclsecurity} PRIVATE ${cclsecurity_sources} ${CMAKE_CURRENT_LIST_FILE})
		ccl_target_headers (${cclsecurity} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclsecurity_public_headers})

		target_link_libraries (${cclsecurity} PUBLIC ${cclbase} ${cclsystem} ${ccltext} PRIVATE cryptlib)
		target_include_directories (${cclsecurity} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")
		
		ccl_export_symbols (${cclsecurity} ${cclsecurity_exports})
		
		ccl_add_resources (${cclsecurity} ${cclsecurity_resources})

		if (CCL_SYSTEM_INSTALL)
			set_target_properties (${cclsecurity} PROPERTIES SOVERSION ${CCL_VERSION})
			install (TARGETS ${cclsecurity} EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
											LIBRARY DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											RUNTIME DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											ARCHIVE DESTINATION "${CCL_IMPORT_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FRAMEWORK DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
			)
			install (FILES $<TARGET_FILE_DIR:${cclsecurity}>/${cclsecurity}.pdb DESTINATION "${CCL_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
		elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
			install (TARGETS ${cclsecurity} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
											RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
											FRAMEWORK DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
			)
		endif ()
	endif ()
	target_sources (${cclsecurity} INTERFACE ${cclsecurity_public_sources})
elseif (NOT XCODE)
	ccl_include_platform_specifics (cclsecurity)
endif ()
