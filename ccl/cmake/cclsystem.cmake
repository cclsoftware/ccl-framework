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
# Filename    : cclsystem.cmake
# Description : CCL system library
#
#************************************************************************************************

set (cclsystem_exports
	CCLModuleMain
	${CCL_EXPORT_PREFIX}AtomicAdd${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicGet${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicGetPtr${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicSet${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicSetPtr${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicTestAndSet${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}AtomicTestAndSetPtr${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CleanupThreadLocalStorage${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateAdvancedLock${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateAtomicStack${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateIPCPipe${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateIPCSemaphore${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateIPCSharedMemory${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateMultiThreadWorker${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateNativeThread${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateSyncPrimitive${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateThreadLocalSlot${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateThreadPool${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateUID${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DebugBreakPoint${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DebugExitProcess${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DebugPrintCString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DebugPrintString${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DebugReportWarning${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}DestroyThreadLocalSlot${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetAnalyticsManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetAtomTable${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetConsole${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetDiagnosticStore${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetErrorHandler${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetExecutableLoader${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFileManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFileSystem${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFileSystemSecurityStore${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFileTypeRegistry${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFileUtilities${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetLocaleManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetLogger${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetMainModuleRef${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetMainThread${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetMediaThreadService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetMemoryAllocator${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetModuleIdentifier${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetModuleWithIdentifier${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetObjectTable${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetPackageHandler${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetPlugInManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetProcessSelfID${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetProfileTime${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetSafetyManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetScriptingManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetServiceManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetSignalHandler${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetSystem${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetSystemTicks${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetThreadLocalData${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetThreadPool${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateThreadSelf${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetThreadSelfID${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateThreadWithIdentifier${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetTypeLibRegistry${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}Hash${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}LockMemory${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}SetThreadLocalData${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}SpinLockLock${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}SpinLockTryLock${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}SpinLockUnlock${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}SwitchMainThread${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}ThreadSleep${CCL_EXPORT_POSTFIX}
)

# collect source files
ccl_list_append_once (cclsystem_main_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/main/cclmodmain.empty.cpp
)

ccl_list_append_once (cclsystem_public_headers
	${CCL_DIR}/public/plugins/classfactory.h
	${CCL_DIR}/public/plugins/iclassfactory.h
	${CCL_DIR}/public/plugins/icoderesource.h
	${CCL_DIR}/public/plugins/icomponent.h
	${CCL_DIR}/public/plugins/icoreplugin.h
	${CCL_DIR}/public/plugins/idatabase.h
	${CCL_DIR}/public/plugins/iobjecttable.h
	${CCL_DIR}/public/plugins/ipluginmanager.h
	${CCL_DIR}/public/plugins/iscriptcodeloader.h
	${CCL_DIR}/public/plugins/iscriptengine.h
	${CCL_DIR}/public/plugins/iscriptingmanager.h
	${CCL_DIR}/public/plugins/iservicemanager.h
	${CCL_DIR}/public/plugins/itypelibregistry.h
	${CCL_DIR}/public/plugins/pluginst.h
	${CCL_DIR}/public/plugins/plugmetaclass.h
	${CCL_DIR}/public/plugins/scripthelper.h
	${CCL_DIR}/public/plugins/serviceplugin.h
	${CCL_DIR}/public/plugins/stubobject.h
	${CCL_DIR}/public/plugins/versionnumber.h
	
	${CCL_DIR}/public/system/alerttypes.h
	${CCL_DIR}/public/system/atomic.h
	${CCL_DIR}/public/system/cclanalytics.h
	${CCL_DIR}/public/system/cclerror.h
	${CCL_DIR}/public/system/cclsafety.h
	${CCL_DIR}/public/system/floatcontrol.h
	${CCL_DIR}/public/system/formatter.h
	${CCL_DIR}/public/system/iallocator.h
	${CCL_DIR}/public/system/ianalyticsmanager.h
	${CCL_DIR}/public/system/iatomtable.h
	${CCL_DIR}/public/system/iconsole.h
	${CCL_DIR}/public/system/idiagnosticdataprovider.h
	${CCL_DIR}/public/system/idiagnosticstore.h
	${CCL_DIR}/public/system/ierrorhandler.h
	${CCL_DIR}/public/system/iexecutable.h
	${CCL_DIR}/public/system/ifilemanager.h
	${CCL_DIR}/public/system/ifilesystemsecuritystore.h
	${CCL_DIR}/public/system/ifileutilities.h
	${CCL_DIR}/public/system/iinterprocess.h
	${CCL_DIR}/public/system/ilocaleinfo.h
	${CCL_DIR}/public/system/ilocalemanager.h
	${CCL_DIR}/public/system/ilockable.h
	${CCL_DIR}/public/system/ilogger.h
	${CCL_DIR}/public/system/imediathreading.h
	${CCL_DIR}/public/system/imultiworker.h
	${CCL_DIR}/public/system/inativefilesystem.h
	${CCL_DIR}/public/system/ipackagefile.h
	${CCL_DIR}/public/system/ipackagehandler.h
	${CCL_DIR}/public/system/ipackagemetainfo.h
	${CCL_DIR}/public/system/iperformance.h
	${CCL_DIR}/public/system/ipersistentexpression.h
	${CCL_DIR}/public/system/ipersistentstore.h
	${CCL_DIR}/public/system/iprotocolhandler.h
	${CCL_DIR}/public/system/isafetymanager.h
	${CCL_DIR}/public/system/isearcher.h
	${CCL_DIR}/public/system/isignalhandler.h
	${CCL_DIR}/public/system/istatistics.h
	${CCL_DIR}/public/system/isysteminfo.h
	${CCL_DIR}/public/system/ithreading.h
	${CCL_DIR}/public/system/ithreadpool.h
	${CCL_DIR}/public/system/lockfree.h
	${CCL_DIR}/public/system/logging.h
	${CCL_DIR}/public/system/threadlocal.h
	${CCL_DIR}/public/system/threadsync.h
	${CCL_DIR}/public/system/userthread.h
	
	${CCL_DIR}/public/devices/ibluetoothstatics.h
	${CCL_DIR}/public/devices/ideviceenumerator.h
	${CCL_DIR}/public/devices/iusbhidstatics.h

	${CCL_DIR}/public/atomicexports.h
	${CCL_DIR}/public/plugservices.h
	${CCL_DIR}/public/systemservices.h
)

ccl_list_append_once (cclsystem_source_files
	${CCL_DIR}/system/allocator.cpp
	${CCL_DIR}/system/allocator.h
	${CCL_DIR}/system/analyticsmanager.cpp
	${CCL_DIR}/system/analyticsmanager.h
	${CCL_DIR}/system/atomtable.cpp
	${CCL_DIR}/system/atomtable.h
	${CCL_DIR}/system/console.cpp
	${CCL_DIR}/system/console.h
	${CCL_DIR}/system/diagnosticstore.cpp
	${CCL_DIR}/system/diagnosticstore.h
	${CCL_DIR}/system/errorhandler.cpp
	${CCL_DIR}/system/errorhandler.h
	${CCL_DIR}/system/filemanager.cpp
	${CCL_DIR}/system/filemanager.h
	${CCL_DIR}/system/fileutilities.cpp
	${CCL_DIR}/system/fileutilities.h
	${CCL_DIR}/system/filesystemsecuritystore.cpp
	${CCL_DIR}/system/filesystemsecuritystore.h
	${CCL_DIR}/system/hash.cpp

	${CCL_DIR}/system/localization/languagepack.cpp
	${CCL_DIR}/system/localization/languagepack.h
	${CCL_DIR}/system/localization/localeinfo.cpp
	${CCL_DIR}/system/localization/localeinfo.h
	${CCL_DIR}/system/localization/localemanager.cpp
	${CCL_DIR}/system/localization/localemanager.h

	${CCL_DIR}/system/logger.cpp
	${CCL_DIR}/system/logger.h
	${CCL_DIR}/system/memoryfilesystem.cpp
	${CCL_DIR}/system/memoryfilesystem.h
	${CCL_DIR}/system/nativefilesystem.cpp
	${CCL_DIR}/system/nativefilesystem.h

	${CCL_DIR}/system/packaging/bufferedstream.cpp
	${CCL_DIR}/system/packaging/bufferedstream.h
	${CCL_DIR}/system/packaging/filearchive.cpp
	${CCL_DIR}/system/packaging/filearchive.h
	${CCL_DIR}/system/packaging/filetree.cpp
	${CCL_DIR}/system/packaging/filetree.h
	${CCL_DIR}/system/packaging/folderpackage.cpp
	${CCL_DIR}/system/packaging/folderpackage.h
	${CCL_DIR}/system/packaging/packagefile.cpp
	${CCL_DIR}/system/packaging/packagefile.h
	${CCL_DIR}/system/packaging/packagefileformat.h
	${CCL_DIR}/system/packaging/packagehandler.cpp
	${CCL_DIR}/system/packaging/packagehandler.h
	${CCL_DIR}/system/packaging/sectionstream.cpp
	${CCL_DIR}/system/packaging/sectionstream.h
	${CCL_DIR}/system/packaging/zipfile.cpp
	${CCL_DIR}/system/packaging/zipfile.h
	${CCL_DIR}/system/packaging/zipfileformat.cpp
	${CCL_DIR}/system/packaging/zipfileformat.h

	${CCL_DIR}/system/persistence/classinfo.cpp
	${CCL_DIR}/system/persistence/classinfo.h
	${CCL_DIR}/system/persistence/objectcache.cpp
	${CCL_DIR}/system/persistence/objectcache.h
	${CCL_DIR}/system/persistence/persistentstore.cpp
	${CCL_DIR}/system/persistence/persistentstore.h
	${CCL_DIR}/system/persistence/queryresult.cpp
	${CCL_DIR}/system/persistence/queryresult.h
	${CCL_DIR}/system/persistence/schema.cpp
	${CCL_DIR}/system/persistence/schema.h
	${CCL_DIR}/system/persistence/sqlwriter.cpp
	${CCL_DIR}/system/persistence/sqlwriter.h

	${CCL_DIR}/system/plugins/coderesource.cpp
	${CCL_DIR}/system/plugins/coderesource.h
	${CCL_DIR}/system/plugins/corecoderesource.cpp
	${CCL_DIR}/system/plugins/corecoderesource.h
	${CCL_DIR}/system/plugins/module.cpp
	${CCL_DIR}/system/plugins/module.h
	${CCL_DIR}/system/plugins/objecttable.cpp
	${CCL_DIR}/system/plugins/objecttable.h
	${CCL_DIR}/system/plugins/plugcollect.cpp
	${CCL_DIR}/system/plugins/plugcollect.h
	${CCL_DIR}/system/plugins/plugmanager.cpp
	${CCL_DIR}/system/plugins/plugmanager.h
	${CCL_DIR}/system/plugins/scriptcoderesource.cpp
	${CCL_DIR}/system/plugins/scriptcoderesource.h
	${CCL_DIR}/system/plugins/scriptinghost.cpp
	${CCL_DIR}/system/plugins/scriptinghost.h
	${CCL_DIR}/system/plugins/scriptingmanager.cpp
	${CCL_DIR}/system/plugins/scriptingmanager.h
	${CCL_DIR}/system/plugins/servicemanager.cpp
	${CCL_DIR}/system/plugins/servicemanager.h
	${CCL_DIR}/system/plugins/stubclasses.cpp
	${CCL_DIR}/system/plugins/stubclasses.h
	${CCL_DIR}/system/plugins/typelibregistry.cpp
	${CCL_DIR}/system/plugins/typelibregistry.h

	${CCL_DIR}/system/safetymanager.cpp
	${CCL_DIR}/system/safetymanager.h
	${CCL_DIR}/system/signalhandler.cpp
	${CCL_DIR}/system/signalhandler.h
	${CCL_DIR}/system/system.cpp
	${CCL_DIR}/system/system.h
	${CCL_DIR}/system/systemservices.cpp

	${CCL_DIR}/system/threading/atomic.cpp
	${CCL_DIR}/system/threading/interprocess.cpp
	${CCL_DIR}/system/threading/interprocess.h
	${CCL_DIR}/system/threading/mediathreadservice.cpp
	${CCL_DIR}/system/threading/mediathreadservice.h
	${CCL_DIR}/system/threading/multiworker.cpp
	${CCL_DIR}/system/threading/multiworker.h
	${CCL_DIR}/system/threading/thread.cpp
	${CCL_DIR}/system/threading/thread.h
	${CCL_DIR}/system/threading/threadlocalstorage.cpp
	${CCL_DIR}/system/threading/threadlocalstorage.h
	${CCL_DIR}/system/threading/threadlocks.cpp
	${CCL_DIR}/system/threading/threadlocks.h
	${CCL_DIR}/system/threading/threadpool.cpp
	${CCL_DIR}/system/threading/threadpool.h

	${CCL_DIR}/system/virtualfilesystem.cpp
	${CCL_DIR}/system/virtualfilesystem.h
)

# collect resource files
ccl_list_append_once (cclsystem_resources
	${CCL_DIR}/resource/localeinfo.xml
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclsystem_SOURCE_DIR)
	source_group ("main" FILES ${cclsystem_main_sources})
	source_group (TREE ${CCL_DIR}/system PREFIX "source" FILES ${cclsystem_source_files})
	source_group (TREE ${CCL_DIR}/public PREFIX "public" FILES ${cclsystem_public_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclsystem_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

# Find Core
ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclbase)
include (ccltext)

set (cclsystem "cclsystem")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclsystem ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
ccl_read_version (${cclsystem} "${CCL_VERSION_FILE}" "CCLSYSTEM")
set (install_cclsystem OFF)
if (NOT TARGET ${cclsystem})
	set (install_cclsystem ON)
	if (${CCL_STATIC_ONLY})
		ccl_add_library (cclsystem INTERFACE POSTFIX "${CCL_ISOLATION_POSTFIX}")
	else ()
		ccl_add_library (cclsystem SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set_target_properties (${cclsystem} PROPERTIES FOLDER "ccl/${CCL_ISOLATION_POSTFIX}")
	endif ()

	# collect source files for target
	ccl_list_append_once (cclsystem_sources
		${cclsystem_main_sources}
		${cclsystem_source_files}
	)

	if (NOT ${CCL_STATIC_ONLY})
		target_sources (${cclsystem} PRIVATE ${cclsystem_sources} ${CMAKE_CURRENT_LIST_FILE})
		ccl_target_headers (${cclsystem} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclsystem_public_headers})
		
		ccl_add_resources (${cclsystem} ${cclsystem_resources})

		target_link_libraries (${cclsystem} PUBLIC ${cclbase} ${ccltext} corelib)
		target_include_directories (${cclsystem} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")
		ccl_export_symbols (${cclsystem} ${cclsystem_exports})

		if (CCL_SYSTEM_INSTALL)
			set_target_properties (${cclsystem} PROPERTIES SOVERSION ${CCL_VERSION})
			install (TARGETS ${cclsystem} EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
										  LIBRARY DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  RUNTIME DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  ARCHIVE DESTINATION "${CCL_IMPORT_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FRAMEWORK DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
			)
			install (FILES $<TARGET_FILE_DIR:${cclsystem}>/${cclsystem}.pdb DESTINATION "${CCL_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
		elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
			install (TARGETS ${cclsystem} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
										  RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
										  FRAMEWORK DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
			)
		endif ()
	endif ()
	target_sources (${cclsystem} INTERFACE ${cclsystem_public_sources})
elseif (NOT XCODE)
	ccl_include_platform_specifics (cclsystem)
endif ()

ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS CCL_STATIC_ENABLE_SYSTEM=1)
