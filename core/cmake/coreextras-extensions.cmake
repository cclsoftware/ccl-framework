
ccl_list_append_once (coreextras_extensions_source_files
	${corelib_DIR}/extras/extensions/coreextensions.cpp
	${corelib_DIR}/extras/extensions/coresignature.cpp
)

ccl_list_append_once (coreextras_extensions_api_headers
	${corelib_DIR}/extras/extensions/coreextensions.h
	${corelib_DIR}/extras/extensions/coremetainfo.h
	${corelib_DIR}/extras/extensions/coresignature.h
)

source_group ("source" FILES ${coreextras_extensions_source_files})
source_group ("public" FILES ${coreextras_extensions_api_headers})

ccl_list_append_once (coreextras_extensions_sources
	${coreextras_extensions_source_files}
	${coreextras_extensions_api_headers}
)

if (NOT TARGET coreextras-extensions)
	ccl_add_library (coreextras-extensions STATIC)
	set_target_properties (coreextras-extensions PROPERTIES USE_FOLDERS ON FOLDER "ccl/extras")

	target_sources (coreextras-extensions PRIVATE ${coreextras_extensions_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (coreextras-extensions INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${corelib_DIR} FILES ${coreextras_extensions_api_headers})
	target_include_directories (coreextras-extensions PRIVATE "$<BUILD_INTERFACE:${CCL_SUBMODULES_DIR}/mbedtls/include>")
	target_link_libraries (coreextras-extensions INTERFACE corelib)
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS coreextras-extensions EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
											   ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											   FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
											   FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:coreextras-extensions>/coreextras-extensions$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
