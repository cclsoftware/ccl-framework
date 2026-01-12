cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("(Service Template)")
set (PROJECT_VENDOR servicevendor)

include (vendor OPTIONAL)
find_package (ccl(FrameworkVersion))

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/(RelPathToRoot)/build/cmake/modules/services)

find_package (servicetemplate)
