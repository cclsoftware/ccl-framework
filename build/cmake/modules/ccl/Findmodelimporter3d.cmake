include_guard (DIRECTORY)

if (TARGET modelimporter3d)
	return ()
endif ()

ccl_find_path (model3d_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/model3d" DOC "Model3d directory")
mark_as_advanced (model3d_DIR)
include ("${model3d_DIR}/cmake/modelimporter3d-config.cmake")
