set (corelib_win_api_headers
	${corelib_DIR}/platform/shared/posix/coresocket.posix.h
	${corelib_DIR}/platform/win/corediscovery.win.h
	${corelib_DIR}/platform/win/corenetwork.win.h
	${corelib_DIR}/platform/win/coresocket.win.h
	${corelib_DIR}/platform/win/coresslcontext.win.h
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.h
	${corelib_DIR}/platform/win/coreatomic.win.h
	${corelib_DIR}/platform/win/coreatomicstack.win.h
	${corelib_DIR}/platform/win/coredynamiclibrary.win.h
	${corelib_DIR}/platform/win/corefilesystem.win.h
	${corelib_DIR}/platform/win/coreinterprocess.win.h
	${corelib_DIR}/platform/win/corethread.win.h
	${corelib_DIR}/platform/win/coretime.win.h
	${corelib_DIR}/platform/win/coremalloc.win.h
)

set (corelib_win_network_sources
	${corelib_DIR}/platform/shared/posix/coresocket.posix.cpp
	${corelib_DIR}/platform/win/corediscovery.win.cpp
	${corelib_DIR}/platform/win/corenetwork.win.cpp
	${corelib_DIR}/platform/win/coresocket.win.cpp
	${corelib_DIR}/platform/win/coresslcontext.win.cpp
)

set (corelib_win_source_files
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.cpp
	${corelib_DIR}/platform/win/coreatomicstack.win.cpp
	${corelib_DIR}/platform/win/coredynamiclibrary.win.cpp
	${corelib_DIR}/platform/win/corefilesystem.win.cpp
	${corelib_DIR}/platform/win/coreinterprocess.win.cpp
	${corelib_DIR}/platform/win/corethread.win.cpp
	${corelib_DIR}/platform/win/windevicenotificationhandler.h
	${corelib_DIR}/platform/win/windevicenotificationhandler.cpp
)

source_group (TREE ${corelib_DIR} PREFIX "source" FILES ${corelib_win_api_headers} ${corelib_win_network_sources} ${corelib_win_source_files})
source_group ("" ${CMAKE_CURRENT_LIST_DIR}/visualstudio/corelib.natvis)

ccl_list_append_once (corelib_network_sources ${corelib_win_network_sources})
ccl_list_append_once (corelib_api_headers ${corelib_win_api_headers})
ccl_list_append_once (corelib_sources ${corelib_win_source_files})
ccl_list_append_once (corelib_sources ${CMAKE_CURRENT_LIST_DIR}/visualstudio/corelib.natvis)
