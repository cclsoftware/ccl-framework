find_package (corelib REQUIRED)

ccl_find_path (dapservice_DIR NAMES "source/dapservice.h" HINTS "${CCL_REPOSITORY_ROOT}/services/dapservice" DOC "DAP Service directory")
mark_as_advanced (dapservice_DIR)

ccl_import_prebuilt_targets (dapservice PLUGIN)

include ("${dapservice_DIR}/cmake/dapservice-config.cmake")
