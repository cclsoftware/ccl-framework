include_guard (DIRECTORY)

ccl_find_path (servicetemplate_DIR NAMES "source/servicetemplate.cpp" HINTS "${CMAKE_CURRENT_LIST_DIR}/(RelPathToRoot)/(Destination)" DOC "(Service Template) service directory")
mark_as_advanced (servicetemplate_DIR)
include ("${servicetemplate_DIR}/cmake/servicetemplate-config.cmake")
