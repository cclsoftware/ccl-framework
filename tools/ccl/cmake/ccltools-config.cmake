include_guard (DIRECTORY)

find_package (corelib REQUIRED)

set (ccltools "")
set (ccltools_FOUND ON)
foreach (ccltool IN ITEMS ${ccltools_FIND_COMPONENTS})
	list (APPEND ccltools ${ccltool})
	set (ccltools_${ccltool}_FOUND ON)
	ccl_find_program (${ccltool} NAMES ${ccltool} ccl${ccltool} HINTS "${CCL_TOOLS_BINDIR}/${VENDOR_HOST_PLATFORM}/ccl" "${CCL_TOOLS_BINDIR}" PATH_SUFFIXES "${CMAKE_HOST_SYSTEM_PROCESSOR}" NO_DEFAULT_PATH)
	set (ccl${ccltool} "${${ccltool}}")
	if (NOT ${ccltool})
		set (ccltools_${ccltool}_FOUND OFF)
		set (ccltools_FOUND OFF)
	endif ()
endforeach ()

if (NOT ccltools_FOUND AND NOT CCL_BUILDING_TOOLS)
	if (NOT CCL_TOOLS_DIR)
		message (FATAL_ERROR "CCL tools not found for the current host architecture")
	else ()
		
		if (CMAKE_CROSSCOMPILING)
			if (NOT VENDOR_HOST_PLATFORM)
				message (FATAL_ERROR "Cross-compiling, but VENDOR_HOST_PLATFORM not set")
			endif ()
			set (ccltool_install_directory "${CCL_TOOLS_BINDIR}/${VENDOR_HOST_PLATFORM}/ccl/${CMAKE_HOST_SYSTEM_PROCESSOR}")

			# fallback to locally built ccltools, build for host architecture
			set (ccltools_FOUND ON)
			foreach (ccltool IN ITEMS ${ccltools_FIND_COMPONENTS})
				set (ccltools_${ccltool}_FOUND ON)

				include (ExternalProject)

				string (SUBSTRING "${ccltool}" 0 3 nameprefix)
				if ("${nameprefix}" STREQUAL "ccl")
					set (toolname "${ccltool}")
					string (SUBSTRING "${ccltool}" 3 -1 tooldirectory)
				else ()
					set (toolname "ccl${ccltool}")
					set (tooldirectory "${ccltool}")
				endif ()
				set (toolfilename "${toolname}")
				if (VENDOR_PLATFORM STREQUAL "win")
					set (toolfilename "${toolname}.exe")
				endif ()

				set (ccltool_build_directory "${CMAKE_BINARY_DIR}/ext/${toolname}/out")

				get_property (is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
				if (is_multi_config)
					set (ccltool_artifacts_directory "${ccltool_build_directory}/$<CONFIG>")
				else ()
					set (ccltool_artifacts_directory "${ccltool_build_directory}")
				endif ()

				if (NOT TARGET ${toolname}_build)
					string (REPLACE ";" "|" tool_module_path "${CMAKE_MODULE_PATH}")
					string (REPLACE ";" "|" tool_include_dirs "${VENDOR_INCLUDE_DIRS}")

					ExternalProject_Add (${toolname}_build
						PREFIX "${CMAKE_BINARY_DIR}/ext/${toolname}"
						SOURCE_DIR "${CCL_TOOLS_DIR}/ccl/${tooldirectory}/cmake"
						LIST_SEPARATOR "|"
						CMAKE_ARGS --no-warn-unused-cli -Wno-dev -DCCL_BUILDING_TOOLS=ON -DVENDOR_ENABLE_DEBUG_SYMBOLS=OFF -DVENDOR_BUILDOUTPUT_DIRECTORY=${ccltool_build_directory} -DCMAKE_MODULE_PATH=${tool_module_path} -DREPOSITORY_ROOT=${REPOSITORY_ROOT} -DVENDOR_INCLUDE_DIRS=${tool_include_dirs} -DZLIB_USE_ZLIB_NG=OFF
						BUILD_BYPRODUCTS "${ccltool_artifacts_directory}/${toolfilename}"
						INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${ccltool_artifacts_directory} ${ccltool_install_directory}/
						INSTALL_BYPRODUCTS "${ccltool_install_directory}/${toolfilename}"
						LOG_CONFIGURE ON
						LOG_INSTALL ON
						LOG_OUTPUT_ON_FAILURE ON
					)
					set_target_properties (${toolname}_build PROPERTIES USE_FOLDERS ON FOLDER "ccl/tools")
				endif ()

				set (${toolname}_build ${toolname}_build)
				set (${ccltool} "${ccltool_install_directory}/${toolfilename}")
				set (ccl${ccltool} "${ccltool_install_directory}/${toolfilename}")
			endforeach ()

		else (CMAKE_CROSSCOMPILING)

			# fallback to locally built ccltools
			set (ccltools_FOUND ON)
			foreach (ccltool IN ITEMS ${ccltools_FIND_COMPONENTS})
				set (ccltools_${ccltool}_FOUND ON)
				add_subdirectory (${CCL_TOOLS_DIR}/ccl/${ccltool}/cmake ${ccltool})
				if (TARGET "ccl${ccltool}")
					set (${ccltool} "$<TARGET_FILE:ccl${ccltool}>")
					set (ccl${ccltool} "$<TARGET_FILE:ccl${ccltool}>")
				elseif (TARGET ${ccltool})
					set (${ccltool} "$<TARGET_FILE:${ccltool}>")
					set (ccl${ccltool} "$<TARGET_FILE:${ccltool}>")
				else ()
					message (FATAL_ERROR "Can't build ${ccltool} from source")
					set (ccltools_${ccltool}_FOUND OFF)
					set (ccltools_FOUND OFF)
				endif ()
			endforeach ()

		endif (CMAKE_CROSSCOMPILING)
	endif ()
endif ()

# Set result variables
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (ccltools
	FOUND_VAR ccltools_FOUND
	REQUIRED_VARS ccltools
	VERSION_VAR CCL_VERSION
	HANDLE_COMPONENTS
)
