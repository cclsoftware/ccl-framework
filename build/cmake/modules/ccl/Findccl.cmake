# Find CCL directory
find_path (CCL_FRAMEWORK_DIR NAMES "public/cclversion.h" HINTS "${CCL_REPOSITORY_ROOT}/ccl" DOC "CCL directory.")
mark_as_advanced (CCL_FRAMEWORK_DIR)
include ("${CCL_FRAMEWORK_DIR}/cmake/ccl-config.cmake")
