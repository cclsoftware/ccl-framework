include_guard (DIRECTORY)

ccl_list_append_once (cclnet_sources
	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
