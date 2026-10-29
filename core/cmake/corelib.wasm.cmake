include_guard (DIRECTORY)

ccl_list_append_once (corelib_api_headers
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.h
	${corelib_DIR}/platform/shared/posix/coretime.posix.h
)

ccl_list_append_once (corelib_platform_shared_sources
	${corelib_DIR}/platform/shared/posix/coredynamiclibrary.posix.cpp
	${corelib_DIR}/platform/shared/coreplatformthread.cpp
)

if (VENDOR_WASM_USE_PTHREADS)
	ccl_list_append_once (corelib_api_headers
		${corelib_DIR}/platform/shared/posix/coreatomic.posix.h
		${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.h
		${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.h
		${corelib_DIR}/platform/shared/posix/corethread.posix.h
	)

	ccl_list_append_once (corelib_platform_shared_sources
		${corelib_DIR}/platform/shared/posix/coreatomicstack.posix.cpp
		${corelib_DIR}/platform/shared/posix/coreinterprocess.posix.cpp
		${corelib_DIR}/platform/shared/posix/corethread.posix.cpp
	)
	ccl_list_append_once (corelib_platform_wasm_sources
		${corelib_DIR}/platform/wasm/corethread.wasm.cpp
	)
endif ()

source_group (TREE ${corelib_DIR}/platform/shared PREFIX "source\\platform" FILES ${corelib_platform_shared_sources})
source_group (TREE ${corelib_DIR}/platform/wasm PREFIX "source\\platform" FILES ${corelib_platform_wasm_sources})

list (APPEND corelib_sources ${corelib_platform_shared_sources} ${corelib_platform_wasm_sources})

ccl_check_imported (corelib imported)

if (VENDOR_WASM_USE_PTHREADS)
	set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
	set (THREADS_PREFER_PTHREAD_FLAG TRUE)
	find_package (Threads REQUIRED)

	if (NOT imported)
		target_link_libraries (corelib PUBLIC $<BUILD_INTERFACE:Threads::Threads>)
		target_compile_options (corelib PUBLIC "-pthread")
		target_compile_definitions (corelib PUBLIC -DCORE_USE_PTHREADS=1)
	endif ()
endif ()

if (NOT imported)
	target_link_libraries (corelib PUBLIC m)
endif ()
