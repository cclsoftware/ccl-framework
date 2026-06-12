find_package (corelib REQUIRED)

ccl_find_path (jsengine_DIR NAMES "source/jsengine.h" HINTS "${CCL_REPOSITORY_ROOT}/services/jsengine" DOC "JavaScript Engine directory")
mark_as_advanced (jsengine_DIR)

ccl_import_prebuilt_targets (jsengine)

include ("${jsengine_DIR}/cmake/jsengine-config.cmake")
