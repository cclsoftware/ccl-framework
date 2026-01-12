include_guard (DIRECTORY)

ccl_find_path (skia_DIR NAMES "../BUILD.gn" HINTS "${CCL_SUBMODULES_DIR}/skia/cmake" DOC "Skia directory.")
mark_as_advanced (skia_DIR)

include ("${skia_DIR}/skia-config.cmake")
