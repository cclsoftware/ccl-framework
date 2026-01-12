ccl_list_append_once (cclsystem_cocoa_sources
	${CCL_DIR}/platform/cocoa/macutils.h
	${CCL_DIR}/platform/cocoa/macutils.mm
	${CCL_DIR}/platform/cocoa/system/debug.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/filemanager.ios.h
	${CCL_DIR}/platform/cocoa/system/filemanager.ios.mm
	${CCL_DIR}/platform/cocoa/system/filesystemsecuritystore.cocoa.h
	${CCL_DIR}/platform/cocoa/system/filesystemsecuritystore.cocoa.cpp
	${CCL_DIR}/platform/cocoa/system/localemanager.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/mediathreadservice.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/nativefilesearcher.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/nativefilesystem.cocoa.h
	${CCL_DIR}/platform/cocoa/system/nativefilesystem.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/resourceloader.cocoa.mm
	${CCL_DIR}/platform/cocoa/system/safetymanager.cocoa.cpp
	${CCL_DIR}/platform/cocoa/system/system.ios.mm
	${CCL_DIR}/platform/cocoa/system/system.shared.h
	${CCL_DIR}/platform/cocoa/system/system.shared.mm
)

ccl_list_append_once (cclsystem_platform_sources
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.cpp
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.h
)

ccl_list_append_once (cclsystem_sources
	${cclsystem_cocoa_sources}
	${cclsystem_platform_sources}
)

source_group (TREE ${CCL_DIR}/platform/cocoa PREFIX "source" FILES ${cclsystem_cocoa_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclsystem_platform_sources})

find_library (UIKIT_LIBRARY UIKit REQUIRED)
find_library (AUDIOTOOLBOX_LIBRARY AudioToolbox REQUIRED)

ccl_list_append_once (cclsystem_apple_frameworks
	${UIKIT_LIBRARY}
	${AUDIOTOOLBOX_LIBRARY}
)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclsystem} PUBLIC ${cclsystem_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${cclsystem_apple_frameworks})


