
ccl_list_append_once (cclsystem_linux_sources
	${CCL_DIR}/platform/linux/interfaces/linuxiids.cpp
	${CCL_DIR}/platform/linux/system/filemanager.linux.h
	${CCL_DIR}/platform/linux/system/localemanager.linux.cpp
	${CCL_DIR}/platform/linux/system/debug.linux.cpp
	${CCL_DIR}/platform/linux/system/filemanager.linux.cpp
	${CCL_DIR}/platform/linux/system/mediathreadservice.linux.cpp
	${CCL_DIR}/platform/linux/system/nativefilesystem.linux.cpp
	${CCL_DIR}/platform/linux/system/resourceloader.linux.cpp
	${CCL_DIR}/platform/linux/system/system.linux.cpp
	${CCL_DIR}/platform/linux/system/mountinfo.cpp
	${CCL_DIR}/platform/linux/system/mountinfo.h
)

ccl_list_append_once (cclsystem_platform_sources
	${CCL_DIR}/platform/shared/host/platformintegration.cpp
	${CCL_DIR}/platform/shared/host/platformintegrationloader.cpp
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.h
	${CCL_DIR}/platform/shared/posix/system/nativefilesystem.posix.cpp
	${CCL_DIR}/platform/shared/posix/system/safetymanager.posix.cpp
)

ccl_list_append_once (cclsystem_sources
	${cclsystem_linux_sources}
	${cclsystem_platform_sources}
)

source_group (TREE ${CCL_DIR}/platform/linux PREFIX "source" FILES ${cclsystem_linux_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclsystem_platform_sources})

find_package (XKB)

find_library (uuid_library uuid)
find_library (rt_library rt)
find_library (dl_library dl)
if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclsystem} PUBLIC ${CMAKE_DL_LIBS} ${dl_library} PRIVATE ${uuid_library} ${rt_library} ${XKB_LIBRARIES})
	target_include_directories (${cclsystem} PRIVATE ${XKB_INCLUDE_PATH})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${dl_library} ${uuid_library} ${rt_library} ${XKB_LIBRARIES})
ccl_list_append_once (CCL_STATIC_INCLUDE_DIRS ${XKB_INCLUDE_PATH})

if (CMAKE_INSTALL_PREFIX)
	target_compile_definitions (${cclsystem} PRIVATE "CCL_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}/\"")
endif ()
option (CCL_PREFER_USERDATA_DIRECTORY "Use user data directory instead of shared data directory" ON)
if (CCL_PREFER_USERDATA_DIRECTORY)
	target_compile_definitions (${cclsystem} PRIVATE "CCL_PREFER_USERDATA_DIRECTORY=1")
endif ()
