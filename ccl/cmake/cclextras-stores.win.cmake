include_guard (DIRECTORY)

ccl_list_append_once (cclextras_stores_platform_sources
	${CCL_DIR}/extras/stores/platform/win/storecontextmanager.win.cpp
)

source_group ("source/platform" FILES ${cclextras_stores_platform_sources})

ccl_list_append_once (cclextras_stores_sources
	${cclextras_stores_platform_sources}
)

ccl_check_imported (cclextras-stores imported)
if (NOT imported)
	target_link_libraries (cclextras-stores PRIVATE WindowsApp.lib)
	target_link_options (cclextras-stores PRIVATE "/NODEFAULTLIB:XmlLite.lib")
endif ()