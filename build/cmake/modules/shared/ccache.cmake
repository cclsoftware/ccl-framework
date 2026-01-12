find_program (CCACHE ccache)
find_program (SHELL sh NAMES sh bash PATHS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer/Git/usr/bin")

if (CCACHE AND SHELL)	
	find_file (CCACHE_CONFIG_FILE NAMES "ccache.conf" HINTS "${REPOSITORY_ROOT}/build/shared" "${CCL_REPOSITORY_ROOT}/build/shared")
	if (NOT CCACHE_CONFIG_FILE)
		message (WARNING "Could not find a ccache config file.")
	endif ()
	
	set (ccache_ignore_path "${CMAKE_BINARY_DIR}/vendor")

	# Only Ninja and Makefile generators do respect the COMPILER_LAUNCHER variables. We need to replace the compiler for other generators.	
	if (CMAKE_GENERATOR MATCHES "(Ninja)|(Makefiles)")
		if (NOT "${VENDOR_HOST_PLATFORM}" STREQUAL "win")
			set (ccache_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/ccache.sh")
			set (ccache_command "${CMAKE_CURRENT_BINARY_DIR}/ccache.sh")
		
			file (WRITE "${ccache_tmp}" "#!${SHELL}
				export CCACHE_BASEDIR=\"${REPOSITORY_ROOT}\"
				export CCACHE_CONFIGPATH=\"${CCACHE_CONFIG_FILE}\"
				export CCACHE_IGNOREHEADERS=\"${ccache_ignore_path}\"
				\"${CCACHE}\" \"\$@\""
			)

		else ()
			set (ccache_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/ccache.bat")
			set (ccache_command "${CMAKE_CURRENT_BINARY_DIR}/ccache.bat")
		
			file (WRITE "${ccache_tmp}" "@echo off
				set CCACHE_BASEDIR=${REPOSITORY_ROOT}
				set CCACHE_CONFIGPATH=${CCACHE_CONFIG_FILE}
				set CCACHE_IGNOREHEADERS=${ccache_ignore_path}
				\"${CCACHE}\" %*"
			)
		endif ()

		set (CMAKE_C_COMPILER_LAUNCHER "${ccache_command}")
		set (CMAKE_CXX_COMPILER_LAUNCHER "${ccache_command}")
		
		file (COPY "${ccache_tmp}"
			DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
			FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
		
	elseif (CMAKE_GENERATOR MATCHES "(Xcode)")
		set (ccache_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/ccache.sh")
		set (ccache_command "${CMAKE_CURRENT_BINARY_DIR}/ccache.sh")
		set (ccache_c_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/ccache_c.sh")
		set (ccache_c_command "${CMAKE_CURRENT_BINARY_DIR}/ccache_c.sh")
		set (ccache_cxx_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/ccache_cxx.sh")
		set (ccache_cxx_command "${CMAKE_CURRENT_BINARY_DIR}/ccache_cxx.sh")

		file (WRITE "${ccache_tmp}" "#!${SHELL}
		export CCACHE_BASEDIR=\"${REPOSITORY_ROOT}\"
		export CCACHE_CONFIGPATH=\"${CCACHE_CONFIG_FILE}\"
		export CCACHE_IGNOREHEADERS=\"${ccache_ignore_path}\"
		\"${CCACHE}\" \"\$@\""
		)
		file (WRITE "${ccache_c_tmp}" "#!${SHELL}
			export CCACHE_BASEDIR=\"${REPOSITORY_ROOT}\"
			export CCACHE_CONFIGPATH=\"${CCACHE_CONFIG_FILE}\"
			export CCACHE_IGNOREHEADERS=\"${ccache_ignore_path}\"
			\"${CCACHE}\" \"${CMAKE_C_COMPILER}\" \"\$@\""
		)
		file (WRITE "${ccache_cxx_tmp}" "#!${SHELL}
			export CCACHE_BASEDIR=\"${REPOSITORY_ROOT}\"
			export CCACHE_CONFIGPATH=\"${CCACHE_CONFIG_FILE}\"
			export CCACHE_IGNOREHEADERS=\"${ccache_ignore_path}\"
			\"${CCACHE}\" \"${CMAKE_CXX_COMPILER}\" \"\$@\""
		)
		file (COPY "${ccache_tmp}"
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
		FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
		file (COPY "${ccache_c_tmp}"
			DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
			FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
		file (COPY "${ccache_cxx_tmp}"
			DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
			FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
		
		set (CMAKE_XCODE_ATTRIBUTE_CC ${ccache_c_command})
		set (CMAKE_XCODE_ATTRIBUTE_CXX ${ccache_cxx_command})
		set (CMAKE_XCODE_ATTRIBUTE_LD ${ccache_c_command})
		set (CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS ${ccache_cxx_command})


		set (CMAKE_C_COMPILER_LAUNCHER "${ccache_command}")
		set (CMAKE_CXX_COMPILER_LAUNCHER "${ccache_command}")
		
	elseif (CMAKE_GENERATOR MATCHES "(Visual Studio)")
		cmake_path (NATIVE_PATH REPOSITORY_ROOT NORMALIZE repo_root)
		cmake_path (NATIVE_PATH CMAKE_CURRENT_BINARY_DIR NORMALIZE binary_dir)
		cmake_path (NATIVE_PATH CCACHE_CONFIG_FILE NORMALIZE config_file)
		cmake_path (NATIVE_PATH CCACHE NORMALIZE ccache_exe)
		cmake_path (NATIVE_PATH CMAKE_C_COMPILER NORMALIZE compiler_path)
		cmake_path (NATIVE_PATH ccache_ignore_path NORMALIZE ignore_path)
	
		set (ccache_command "${binary_dir}\\ccache.bat")
		set (ccache_compilertype msvc)

		file (WRITE "${ccache_command}" "
			@echo off
			@setlocal
			set \"CCACHE_BASEDIR=${repo_root}\"
			set \"CCACHE_CONFIGPATH=${config_file}\"
			set \"CCACHE_IGNOREHEADERS=${ignore_path}\"
			set CCACHE_COMPILERTYPE=${ccache_compilertype}
			\"${ccache_exe}\" \"${compiler_path}\" %*"
		)

		list (APPEND CMAKE_VS_GLOBALS "CLToolExe=ccache.bat" "CLToolPath=${binary_dir}" "TrackFileAccess=false" "UseMultiToolTask=true" "EnforceProcessCountAcrossBuilds=true") 
	endif ()
else ()
	message (WARNING "Could not find ccache.")
endif ()