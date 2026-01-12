ccl_list_append_once (cclsystem_main_sources
	${CCL_DIR}/platform/win/cclsystemmain.cpp
)

ccl_list_append_once (cclsystem_platform_win32_sources
	${CCL_DIR}/platform/win/system/cclcoinit.h
	${CCL_DIR}/platform/win/system/cclcom.cpp
	${CCL_DIR}/platform/win/system/cclcom.h
	${CCL_DIR}/platform/win/system/cclcom.impl.h
	${CCL_DIR}/platform/win/system/cclwinrt.cpp
	${CCL_DIR}/platform/win/system/cclwinrt.h
	${CCL_DIR}/platform/win/system/management.cpp
	${CCL_DIR}/platform/win/system/management.h
	${CCL_DIR}/platform/win/system/registry.cpp
	${CCL_DIR}/platform/win/system/registry.h
	${CCL_DIR}/platform/win/system/winrtplatform.cpp
)

ccl_list_append_once (cclsystem_platform_sources
	${CCL_DIR}/platform/win/system/debug.win.cpp
	${CCL_DIR}/platform/win/system/filemanager.win.cpp
	${CCL_DIR}/platform/win/system/filemanager.win.h
	${CCL_DIR}/platform/win/system/localemanager.win.cpp
	${CCL_DIR}/platform/win/system/mediathreadservice.win.cpp	
	${CCL_DIR}/platform/win/system/nativefilesearcher.win.cpp
	${CCL_DIR}/platform/win/system/nativefilesystem.win.cpp
	${CCL_DIR}/platform/win/system/nativefilesystem.win.h
	${CCL_DIR}/platform/win/system/resourceloader.win.cpp
	${CCL_DIR}/platform/win/system/safetymanager.win.cpp
	${CCL_DIR}/platform/win/system/system.win.cpp
	${CCL_DIR}/platform/win/system/system.win.h
)

ccl_list_append_once (cclsystem_sources
	${cclsystem_platform_sources}
	${cclsystem_platform_win32_sources}
)

ccl_list_append_once (${cclsystem}_exports
	${CCL_EXPORT_PREFIX}GetWinRTPlatform${CCL_EXPORT_POSTFIX}
)

source_group ("main" FILES ${cclsystem_main_sources})
source_group (TREE ${CCL_DIR}/platform/win/system PREFIX "source" FILES ${cclsystem_platform_sources})
source_group (TREE ${CCL_DIR}/platform/win/system PREFIX "source/win32" FILES ${cclsystem_platform_win32_sources})