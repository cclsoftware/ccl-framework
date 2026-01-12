include_guard (DIRECTORY)

ccl_list_append_once (corelib_platform_sources
	${corelib_DIR}/platform/zephyr/corethread.zephyr.cpp
	${corelib_DIR}/platform/zephyr/corethread.zephyr.h
	${corelib_DIR}/platform/zephyr/coretime.zephyr.h
	${corelib_DIR}/platform/zephyr/coredebug.zephyr.h
	${corelib_DIR}/platform/zephyr/coreatomic.zephyr.h
	${corelib_DIR}/platform/zephyr/corestaticthread.zephyr.h)

source_group (TREE ${corelib_DIR}/platform/zephyr PREFIX "source\\platform" FILES ${corelib_platform_sources})

ccl_list_append_once (corelib_source_files ${corelib_platform_sources})

zephyr_append_cmake_library (corelib)
target_link_libraries (corelib PUBLIC zephyr_interface kernel)

target_include_directories (corelib PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated)
