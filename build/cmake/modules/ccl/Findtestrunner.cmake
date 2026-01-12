include_guard (DIRECTORY)

set (testrunner_DIR "${CCL_REPOSITORY_ROOT}/tools/ccl/testrunner/cmake")
mark_as_advanced (testrunner_DIR)
add_subdirectory ("${testrunner_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/testrunner")