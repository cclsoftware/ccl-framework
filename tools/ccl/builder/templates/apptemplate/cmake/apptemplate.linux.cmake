include_guard (DIRECTORY)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/platform/linux/linuxmain.cpp
)

install (TARGETS apptemplate RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")

ccl_install_desktop_file (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/resource/apptemplate.desktop.in "${apptemplate_PACKAGE_ID}")
ccl_install_icon (apptemplate ${CCL_DIR}/packaging/linux/resource/cclapp.svg)

include (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/deb/apptemplate.package.cmake)
