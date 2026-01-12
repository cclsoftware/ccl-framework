include_guard (DIRECTORY)

ccl_find_path (tuioservice_DIR NAMES "source/tuioservice.h" HINTS "${CCL_REPOSITORY_ROOT}/services/tuio" DOC "TUIO service directory")
mark_as_advanced (tuioservice_DIR)
include ("${tuioservice_DIR}/cmake/tuioservice-config.cmake")
