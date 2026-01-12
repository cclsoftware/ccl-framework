include_guard (DIRECTORY)

# On Windows, there should be exactly one definition of replacement new/delete operators in each shared library or executable.
# Add corenewoperator.cpp to cclbase, so that each module linking cclbase has these definitions.
ccl_list_append_once (cclbase_core_sources 
	${corelib_DIR}/malloc/corenewoperator.cpp
)

ccl_list_append_once (cclbase_sources
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/cclbase.natstepfilter 
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/cclbase.natvis
)
source_group ("" FILES 
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/cclbase.natstepfilter 
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/cclbase.natvis
)
