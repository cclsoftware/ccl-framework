include_guard (DIRECTORY)

ccl_find_path (dapservice_DIR NAMES "source/dapservice.h" HINTS "${CCL_REPOSITORY_ROOT}/services/dapservice" DOC "DAP Service directory")
mark_as_advanced (dapservice_DIR)
include ("${dapservice_DIR}/cmake/dapservice-config.cmake")
