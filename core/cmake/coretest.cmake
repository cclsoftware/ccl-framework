
ccl_list_append_once (coretestcase_sources
	${corelib_DIR}/test/coreallocatortest.cpp
	${corelib_DIR}/test/coreallocatortest.h
	${corelib_DIR}/test/coreatomictest.cpp
	${corelib_DIR}/test/coreatomictest.h
	${corelib_DIR}/test/corecrctest.cpp
	${corelib_DIR}/test/corecrctest.h
	${corelib_DIR}/test/coredequetest.cpp
	${corelib_DIR}/test/coredequetest.h
	${corelib_DIR}/test/corefiletest.cpp
	${corelib_DIR}/test/corefiletest.h
	${corelib_DIR}/test/corelinkedlisttest.cpp
	${corelib_DIR}/test/corelinkedlisttest.h
	${corelib_DIR}/test/corestorabletest.cpp
	${corelib_DIR}/test/corestorabletest.h
	${corelib_DIR}/test/corestringtest.cpp
	${corelib_DIR}/test/corestringtest.h
	${corelib_DIR}/test/corethreadtest.cpp
	${corelib_DIR}/test/corethreadtest.h
	${corelib_DIR}/test/coretimetest.cpp
	${corelib_DIR}/test/coretimetest.h
	${corelib_DIR}/test/coretreesettest.h
	${corelib_DIR}/test/coretreesettest.cpp
	${corelib_DIR}/test/corevectortest.cpp
	${corelib_DIR}/test/corevectortest.h
)

if (corelib_ENABLE_NETWORK)
	ccl_list_append_once (coretestcase_sources 
		${corelib_DIR}/test/corenetworktest.cpp
		${corelib_DIR}/test/corenetworktest.h 
		${corelib_DIR}/test/coresockettest.cpp
		${corelib_DIR}/test/coresockettest.h
	)
endif ()

ccl_list_append_once (coretestbase_source_files
	${corelib_DIR}/test/coretestbase.cpp
	${corelib_DIR}/test/coretestrunner.cpp
	${corelib_DIR}/test/coretestrunner.h
)

ccl_list_append_once (coretestbase_api_headers
	${corelib_DIR}/test/coretestbase.h
	${corelib_DIR}/test/coretestcontext.h
)

ccl_list_append_once (coretestbase_sources
	${coretestbase_source_files}
	${coretestbase_api_headers}
)

source_group ("source\\test" FILES ${coretestcase_sources} ${coretestbase_source_files} ${coretestbase_api_headers})

if (NOT TARGET coretestbase)
	ccl_add_library (coretestbase INTERFACE)
	set_target_properties (coretestbase PROPERTIES USE_FOLDERS ON FOLDER "ccl")

	target_sources (coretestbase INTERFACE ${coretestbase_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (coretestbase INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${corelib_DIR} FILES ${coretestbase_api_headers})
	target_include_directories (coretestbase INTERFACE ${corelib_BASEDIR})
	target_link_libraries (coretestbase INTERFACE corelib)
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS coretestbase EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
									  ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									  FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									  FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:coretestbase>/coretestbase$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()

if (NOT TARGET coretest)
	ccl_add_library (coretest INTERFACE)
	set_target_properties (coretest PROPERTIES USE_FOLDERS ON FOLDER "ccl")

	target_sources (coretest INTERFACE ${coretestcase_sources} ${CMAKE_CURRENT_LIST_FILE})
	target_include_directories (coretest INTERFACE ${corelib_BASEDIR})
	target_link_libraries (coretest INTERFACE coretestbase)
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS coretest EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
								  ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								  FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								  FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES $<TARGET_FILE_DIR:coretest>/coretest$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
