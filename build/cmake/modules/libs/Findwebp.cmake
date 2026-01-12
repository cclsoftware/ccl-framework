include_guard (DIRECTORY)

ccl_find_path (webp_DIR NAMES "src/webp/decode.h" HINTS "${CCL_SUBMODULES_DIR}/libwebp" DOC "webp Directory")
mark_as_advanced (webp_DIR)

if (NOT TARGET webp)
	list (APPEND webp_options
		-DWEBP_BUILD_ANIM_UTILS=OFF
		-DWEBP_BUILD_CWEBP=OFF
		-DWEBP_BUILD_DWEBP=OFF
		-DWEBP_BUILD_GIF2WEBP=OFF
		-DWEBP_BUILD_IMG2WEBP=OFF
		-DWEBP_BUILD_VWEBP=OFF
		-DWEBP_BUILD_WEBPINFO=OFF
		-DWEBP_BUILD_LIBWEBPMUX=OFF
		-DWEBP_BUILD_WEBPMUX=OFF
		-DWEBP_BUILD_EXTRAS=OFF
		-DWEBP_BUILD_WEBP_JS=OFF
		-DWEBP_USE_THREAD=ON
		-DWEBP_NEAR_LOSSLESS=ON
		-DWEBP_ENABLE_SWAP_16BIT_CSP=OFF
	)

	if(XCODE)
		# CMAKE_OSX_ARCHITECTURES typically contains ;, which is the list separator in cmake
		string (REPLACE ";" "\\\\;" OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
		list (APPEND webp_options
			-DCMAKE_OSX_ARCHITECTURES:STRING=${OSX_ARCHITECTURES}
			-DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT}
		)
		# can not pass a $ sign in an argument to BUILD_COMMAND, workaround : write a shell script
		file (WRITE ${CMAKE_BINARY_DIR}/external/tmp/build_webp.sh "#!/bin/bash\n${CMAKE_COMMAND} --build . --config \$CONFIGURATION -- -sdk \$PLATFORM_NAME")
		file (CHMOD ${CMAKE_BINARY_DIR}/external/tmp/build_webp.sh PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
		file (WRITE ${CMAKE_BINARY_DIR}/external/tmp/install_webp.sh "#!/bin/bash\n${CMAKE_COMMAND} --build . --config \$CONFIGURATION  --target install -- -sdk \$PLATFORM_NAME")
		file (CHMOD ${CMAKE_BINARY_DIR}/external/tmp/install_webp.sh PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

		set (webp_BUILD_COMMAND ${CMAKE_BINARY_DIR}/external/tmp/build_webp.sh)
		set (webp_INSTALL_COMMAND ${CMAKE_BINARY_DIR}/external/tmp/install_webp.sh)
	endif()

	ccl_add_external_project (webp "${webp_DIR}" BUILD_COMMAND "${webp_BUILD_COMMAND}" INSTALL_COMMAND "${webp_INSTALL_COMMAND}" OPTIONS ${webp_options})
	set_target_properties (webp PROPERTIES USE_FOLDERS ON FOLDER "ccl/libs")

	add_library (webp_library STATIC IMPORTED GLOBAL)
	add_library (sharpyuv_library STATIC IMPORTED GLOBAL)

	target_include_directories (webp_library INTERFACE "${webp_INCLUDE_DIR}")

	set_target_properties (webp_library PROPERTIES IMPORTED_LOCATION "${webp_LIBRARY_OUTPUT}")		
	set_target_properties (sharpyuv_library PROPERTIES IMPORTED_LOCATION "${sharpyuv_LIBRARY_OUTPUT}")		
	add_dependencies (webp_library webp)
	add_dependencies (sharpyuv_library webp)
else ()
	set (webp_FIND_QUIETLY ON)
endif ()

set (webp_LIBRARIES webp_library sharpyuv_library)
set (webp_LIBRARY webp_library)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (webp
	REQUIRED_VARS webp_LIBRARY
)