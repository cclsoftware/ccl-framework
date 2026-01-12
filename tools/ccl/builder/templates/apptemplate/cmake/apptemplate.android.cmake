include_guard (DIRECTORY)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/platform/android/androidmain.cpp
)

source_group ("source\\core" FILES ${corelib_android_jnionload_sources})

list (APPEND apptemplate_ccl_sources
	${corelib_android_jnionload_sources}
)

ccl_add_deployment_project (apptemplate "(ProjectPackageID)")
