ccl_find_path (sqlite_DIR NAMES "../source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/sqlite/cmake" DOC "SQLite service directory.")
mark_as_advanced (sqlite_DIR)
include ("${sqlite_DIR}/sqlite-config.cmake")
