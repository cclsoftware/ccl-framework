include ("${CCL_DIR}/cmake/cclbase.mac.cmake")

list (APPEND cclbase_sources
	${CCL_DIR}/system/hash.cpp # TODO, remove this dependency
)
