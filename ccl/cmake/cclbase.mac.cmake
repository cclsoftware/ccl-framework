include_guard (DIRECTORY)

# On Apple platforms, there should be exactly one definition of replacement new/delete operators in each shared library or executable.
# Add corenewoperator.cpp to cclbase, so that each module linking cclbase has these definitions.
ccl_list_append_once (cclbase_core_sources 
	${corelib_DIR}/malloc/corenewoperator.cpp
)

ccl_list_append_once (cclbase_sources
	${CCL_DIR}/platform/cocoa/macutils.mm
	${CCL_DIR}/public/base/ccldefpush.h
	${CCL_DIR}/public/base/ccldefpop.h
)
source_group ("source" FILES ${CCL_DIR}/platform/cocoa/macutils.mm)
