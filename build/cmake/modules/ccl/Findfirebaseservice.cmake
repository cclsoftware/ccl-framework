include_guard (DIRECTORY)

ccl_find_path (firebase_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/firebase" DOC "Firebase service directory")
mark_as_advanced (firebase_DIR)
include ("${firebase_DIR}/cmake/firebaseservice-config.cmake")
