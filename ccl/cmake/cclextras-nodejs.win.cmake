include_guard (DIRECTORY)

ccl_list_append_once (cclextras_nodejs_platform_sources
	${CCL_DIR}/extras/nodejs/platform/nodeaddon.win.cpp
	${CCL_DIR}/extras/nodejs/platform/delayloadhook.win.cpp
)

source_group ("source/platform" FILES ${cclextras_nodejs_platform_sources})

ccl_list_append_once (cclextras_nodejs_sources
	${cclextras_nodejs_platform_sources}
)
