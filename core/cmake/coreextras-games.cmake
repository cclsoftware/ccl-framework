
ccl_list_append_once (coreextras_games_source_files
	${corelib_DIR}/extras/games/gamecore.cpp
	${corelib_DIR}/extras/games/staticgameenv.cpp
	${corelib_DIR}/extras/games/testgame.cpp
)

ccl_list_append_once (coreextras_games_api_headers
	${corelib_DIR}/extras/games/gamecore.h
	${corelib_DIR}/extras/games/staticgameenv.h
	${corelib_DIR}/extras/games/testgame.h
)

source_group ("source" FILES ${coreextras_games_source_files})
source_group ("public" FILES ${coreextras_games_api_headers})

ccl_list_append_once (coreextras_games_sources
	${coreextras_games_source_files}
	${coreextras_games_api_headers}
)

if (NOT TARGET coreextras-games)
	ccl_add_library (coreextras-games STATIC)
	set_target_properties (coreextras-games PROPERTIES USE_FOLDERS ON FOLDER "ccl/extras")

	target_sources (coreextras-games PRIVATE ${coreextras_games_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (coreextras-games INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${corelib_DIR} FILES ${coreextras_games_api_headers})
	target_include_directories (coreextras-games PRIVATE "$<BUILD_INTERFACE:${CCL_SUBMODULES_DIR}/mbedtls/include>")
	target_link_libraries (coreextras-games INTERFACE corelib)
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS coreextras-games EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
										  ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:coreextras-games>/coreextras-games$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
