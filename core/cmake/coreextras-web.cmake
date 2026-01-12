
ccl_list_append_once (coreextras_web_source_files
	${corelib_DIR}/extras/web/corewebserver.cpp
	${corelib_DIR}/extras/web/corewebserverloop.cpp
)

ccl_list_append_once (coreextras_web_api_headers
	${corelib_DIR}/extras/web/corewebserver.h
	${corelib_DIR}/extras/web/corewebserverloop.h
)

source_group ("source" FILES ${coreextras_web_source_files})
source_group ("public" FILES ${coreextras_web_api_headers})

ccl_list_append_once (coreextras_web_sources
	${coreextras_web_source_files}
	${coreextras_web_api_headers}
)

if (NOT TARGET coreextras-web)
	ccl_add_library (coreextras-web STATIC)
	set_target_properties (coreextras-web PROPERTIES USE_FOLDERS ON FOLDER "ccl/extras")

	target_sources (coreextras-web PRIVATE ${coreextras_web_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (coreextras-web INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${corelib_DIR} FILES ${coreextras_web_api_headers})
	target_include_directories (coreextras-web PRIVATE "$<BUILD_INTERFACE:${CCL_SUBMODULES_DIR}/mbedtls/include>")
	target_link_libraries (coreextras-web INTERFACE corelib)
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS coreextras-web EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
										  ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										  FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:coreextras-web>/coreextras-web$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
