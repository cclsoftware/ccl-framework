include_guard (DIRECTORY)

ccl_list_append_once (corelib_api_headers
	${corelib_DIR}/platform/cocoa/coreatomic.cocoa.h
	${corelib_DIR}/platform/cocoa/coreatomicstack.cocoa.h
	${corelib_DIR}/platform/cocoa/coredynamiclibrary.cocoa.h
	${corelib_DIR}/platform/cocoa/corefilesystem.cocoa.h
	${corelib_DIR}/platform/cocoa/corenetwork.cocoa.h
	${corelib_DIR}/platform/cocoa/coresocket.cocoa.h
	${corelib_DIR}/platform/cocoa/coresslcontext.cocoa.h
	${corelib_DIR}/platform/cocoa/corethread.cocoa.h
	${corelib_DIR}/platform/cocoa/coretime.cocoa.h
	${corelib_DIR}/platform/shared/posix/corediscovery.posix.h
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.h
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.h
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.h
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.h
	${corelib_DIR}/platform/shared/posix/coresocket.posix.h
	${corelib_DIR}/platform/shared/posix/corethread.posix.h
)

ccl_list_append_once (corelib_platform_sources
	${corelib_DIR}/platform/cocoa/coreatomicstack.cocoa.cpp
	${corelib_DIR}/platform/cocoa/coredynamiclibrary.cocoa.cpp
	${corelib_DIR}/platform/cocoa/corefilesystem.cocoa.cpp
	${corelib_DIR}/platform/cocoa/corenetwork.cocoa.cpp
	${corelib_DIR}/platform/cocoa/coresocket.cocoa.cpp
	${corelib_DIR}/platform/cocoa/coresslcontext.cocoa.cpp
	${corelib_DIR}/platform/cocoa/corethread.cocoa.cpp
	${corelib_DIR}/platform/shared/posix/corediscovery.posix.cpp
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.cpp
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.cpp
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.cpp
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.cpp
	${corelib_DIR}/platform/shared/posix/coresocket.posix.cpp
	${corelib_DIR}/platform/shared/posix/corethread.posix.cpp
)

source_group (TREE ${corelib_DIR}/platform PREFIX "source\\platform" FILES ${corelib_platform_sources})

ccl_list_append_once (corelib_sources ${corelib_platform_sources})
