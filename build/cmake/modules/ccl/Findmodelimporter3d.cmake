find_package (corelib REQUIRED)

ccl_find_path (model3d_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/model3d" DOC "Model3d directory")
mark_as_advanced (model3d_DIR)

ccl_import_prebuilt_targets (modelimporter3d)

include ("${model3d_DIR}/cmake/modelimporter3d-config.cmake")
