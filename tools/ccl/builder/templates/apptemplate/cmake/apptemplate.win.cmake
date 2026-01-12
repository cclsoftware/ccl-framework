include_guard (DIRECTORY)

find_package (uninstaller)
ccl_add_dependencies (apptemplate uninstaller)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/platform/win/winmain.cpp
)

list (APPEND apptemplate_icons
	1	${CCL_DIR}/packaging/win/resource/cclapp.ico
)

ccl_embed_manifest (apptemplate "${CCL_DIR}/packaging/win/application.manifest")

ccl_nsis_package (apptemplate "${CMAKE_CURRENT_LIST_DIR}/../packaging/win/nsis/apptemplate.nsi")
