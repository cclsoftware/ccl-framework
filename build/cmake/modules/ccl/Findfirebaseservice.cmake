find_package (corelib REQUIRED)

ccl_find_path (firebase_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/firebase" DOC "Firebase service directory")
mark_as_advanced (firebase_DIR)

ccl_import_prebuilt_targets (firebaseservice)

include ("${firebase_DIR}/cmake/firebaseservice-config.cmake")
