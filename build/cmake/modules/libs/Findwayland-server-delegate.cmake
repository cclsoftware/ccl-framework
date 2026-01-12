include_guard (DIRECTORY)

ccl_find_path (serverdelegate_DIR NAMES "../source/waylandserver.h" HINTS "${CCL_SUBMODULES_DIR}/wayland-server-delegate/cmake" DOC "Wayland Server Delegate directory.")
mark_as_advanced (serverdelegate_DIR)
list (APPEND CMAKE_MODULE_PATH "${serverdelegate_DIR}")

set (WAYLAND_PROTOCOLS_DIR "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/${CMAKE_BUILD_TYPE}/wayland-protocols" CACHE PATH "Directory for generated wayland protocol headers and source files")
include ("${serverdelegate_DIR}/wayland-server-delegate-config.cmake")
