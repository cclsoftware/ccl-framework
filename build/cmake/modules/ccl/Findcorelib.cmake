find_path (CCL_CORELIB_DIR NAMES "public/coreversion.h" HINTS "${CCL_REPOSITORY_ROOT}/core" DOC "corelib directory")
mark_as_advanced (CCL_CORELIB_DIR)
include (${CCL_CORELIB_DIR}/cmake/corelib-config.cmake)
