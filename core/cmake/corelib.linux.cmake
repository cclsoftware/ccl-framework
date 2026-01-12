include_guard (DIRECTORY)

ccl_list_append_once (corelib_api_headers
	${corelib_DIR}/platform/shared/posix/coreatomic.posix.h
	${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.h
	${corelib_DIR}/platform/shared/posix/corediscovery.posix.h
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.h
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.h
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.h
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.h
	${corelib_DIR}/platform/shared/posix/coresocket.posix.h
	${corelib_DIR}/platform/shared/posix/corethread.posix.h
	${corelib_DIR}/platform/shared/posix/coretime.posix.h
	${corelib_DIR}/platform/linux/corethread.linux.h
	${corelib_DIR}/platform/linux/coreinterprocess.linux.h
)

ccl_list_append_once (corelib_platform_shared_sources
	${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.cpp
	${corelib_DIR}/platform/shared/posix/corediscovery.posix.cpp
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.cpp
	${corelib_DIR}/platform/shared/posix/corefilesystem.posix.cpp
	${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.cpp
	${corelib_DIR}/platform/shared/posix/corenetwork.posix.cpp
	${corelib_DIR}/platform/shared/posix/coresocket.posix.cpp
	${corelib_DIR}/platform/shared/posix/corethread.posix.cpp
)

ccl_list_append_once (corelib_platform_linux_sources
	${corelib_DIR}/platform/linux/corethread.linux.cpp
	${corelib_DIR}/platform/linux/coreinterprocess.linux.cpp
)

source_group (TREE ${corelib_DIR}/platform/shared PREFIX "source\\platform" FILES ${corelib_platform_shared_sources})
source_group (TREE ${corelib_DIR}/platform/linux PREFIX "source\\platform" FILES ${corelib_platform_linux_sources})

list (APPEND corelib_sources ${corelib_platform_shared_sources} ${corelib_platform_linux_sources})

find_package (OpenSSL)
if (${OPENSSL_FOUND})
	target_compile_definitions (corelib PUBLIC -DCORE_USE_OPENSSL=1)
	target_include_directories (corelib PRIVATE ${OPENSSL_INCLUDE_DIR})
	target_link_libraries (corelib PRIVATE ${OPENSSL_LIBRARIES})
	list (APPEND corelib_network_sources
		${corelib_DIR}/platform/shared/openssl/coresslcontext.openssl.cpp
	)
	list (APPEND corelib_network_headers
		${corelib_DIR}/platform/shared/openssl/coresslcontext.openssl.h
	)
else ()
	message (WARNING "Building without SSL support")
endif ()

ccl_find_path (dnssd_INCLUDE_DIR NAMES dns_sd.h HINTS /usr/include PATH_SUFFIXES avahi-compat-libdns_sd)
if (dnssd_INCLUDE_DIR)
	target_include_directories (corelib PUBLIC ${dnssd_INCLUDE_DIR})
else ()
	message (WARNING "Building without dns_sd")
endif ()

set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
target_link_libraries (corelib PUBLIC $<BUILD_INTERFACE:Threads::Threads> m)

target_link_options (corelib INTERFACE "-Wl,--exclude-libs,$<TARGET_FILE:corelib>")
target_compile_options (corelib PUBLIC "-pthread")
