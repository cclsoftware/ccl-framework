include_guard (DIRECTORY)

set (skins_config_file ${CMAKE_CURRENT_LIST_FILE})

find_package (ccltools REQUIRED COMPONENTS package)

list (APPEND VENDOR_SKIN_DIRS "${CMAKE_CURRENT_LIST_DIR}/..")

option (CCL_ADD_SKIN_RESOURCES_DEBUG "Add skin resources to binaries in debug builds" "${CMAKE_CROSSCOMPILING}")

# use a skin package
macro (ccl_use_skin_packages target)
	set (packages "${ARGN}")
	foreach (package IN LISTS packages)
		foreach (skins_DIR ${VENDOR_SKIN_DIRS})
			set (skin "${skins_DIR}/${package}")
			get_filename_component (skin "${skin}" ABSOLUTE)
			if (EXISTS "${skin}")
				set (skins_output_dir "${CMAKE_BINARY_DIR}/skins")
				if (ANDROID AND "${CMAKE_BINARY_DIR}" MATCHES ".*/\.cxx/.*")
					set (skins_output_dir "${CMAKE_BINARY_DIR}/../../../../../skins")
				endif ()
				file (MAKE_DIRECTORY ${skins_output_dir})

				set (skins_${package}_OUTPUT "${skins_output_dir}/${package}.skin")
				if(NOT TARGET skins_${package})
					ccl_file_glob (dependencies "${skin}/*")
					get_filename_component (skins_${package}_OUTPUT "${skins_${package}_OUTPUT}" ABSOLUTE)
					cmake_path (NATIVE_PATH skin NORMALIZE skin)
					cmake_path (NATIVE_PATH skins_${package}_OUTPUT NORMALIZE output)
					add_custom_command (OUTPUT ${skins_${package}_OUTPUT}
						COMMAND "${cclpackage}" -c "${skin}" "${output}" "pdn,psd" -depfile "${output}.d"
						# DEPENDS ${dependencies} # no need to explicitly add dependencies. Dependencies are defined by the DEPFILE
						DEPFILE "${skins_${package}_OUTPUT}.d"
						WORKING_DIRECTORY ${skins_DIR}
						COMMAND_EXPAND_LISTS
						VERBATIM USES_TERMINAL
					)
					
					if (APPLE)
						foreach (dependency IN LISTS dependencies)
							set_source_files_properties (${dependency} PROPERTIES XCODE_EXPLICIT_FILE_TYPE default)
						endforeach ()
					endif ()
					
					add_custom_target (skins_${package} DEPENDS ${skins_${package}_OUTPUT})
					target_sources (skins_${package} PRIVATE ${dependencies} ${skins_config_file})
					set_source_files_properties (${dependencies} PROPERTIES HEADER_FILE_ONLY ON)
					source_group (TREE "${skin}" PREFIX "skin" FILES ${dependencies})
					source_group ("cmake" FILES ${skins_config_file})
					set_target_properties (skins_${package} PROPERTIES USE_FOLDERS ON FOLDER skins)
					ccl_add_dependencies (skins_${package} ${cclpackage_build})
				endif ()
				break ()
			endif ()
		endforeach ()
		ccl_add_dependencies (${target} skins_${package})
		if (CCL_ADD_SKIN_RESOURCES_DEBUG)
			ccl_add_resources (${target} ${skins_${package}_OUTPUT})
		else ()
			ccl_add_resources (${target} ${skins_${package}_OUTPUT} CONFIG Release)
		endif ()
	endforeach ()
endmacro ()

# adds a custom application skin package
macro (ccl_add_skin_package target from)
	set (name "default")
	if (${ARGC} GREATER 3)
		set (name "${ARGV3}")
	endif ()

	set (skins_output_dir "${CMAKE_BINARY_DIR}/skins")
	if (ANDROID AND "${CMAKE_BINARY_DIR}" MATCHES ".*/\.cxx/.*")
		set (skins_output_dir "${CMAKE_BINARY_DIR}/../../../../../skins")
	endif ()
	file (MAKE_DIRECTORY ${skins_output_dir})

	set (skins_${target}_OUTPUT "${skins_output_dir}/${target}/${name}.skin")
	if (${ARGC} GREATER 2 AND "${ARGV2}")
		set (skins_${target}_OUTPUT "${ARGV2}/${name}.skin")
	endif ()

	if (NOT TARGET skins_${target})
		get_filename_component (skin "${from}" ABSOLUTE)
		ccl_file_glob (dependencies "${skin}/*")
		cmake_path (NATIVE_PATH skin NORMALIZE skin)
		cmake_path (NATIVE_PATH skins_${target}_OUTPUT NORMALIZE output)
		add_custom_command (OUTPUT ${skins_${target}_OUTPUT}
			COMMAND "${cclpackage}" -c "${skin}" "${output}" "pdn,psd" -depfile "${output}.d"
			# DEPENDS ${dependencies} # no need to explicitly add dependencies. Dependencies are defined by the DEPFILE
			DEPFILE "${skins_${target}_OUTPUT}.d"
			WORKING_DIRECTORY "${from}/.."
			COMMAND_EXPAND_LISTS
			VERBATIM USES_TERMINAL
		)
		
		if (APPLE)
			foreach (dependency IN LISTS dependencies)
				set_source_files_properties (${dependency} PROPERTIES XCODE_EXPLICIT_FILE_TYPE default)
			endforeach ()
		endif ()
		
		target_sources (${target} PRIVATE ${dependencies})
		set_source_files_properties (${dependencies} PROPERTIES HEADER_FILE_ONLY ON)
		source_group (TREE "${skin}" PREFIX "skin" FILES ${dependencies})
		ccl_add_dependencies (${target} ${cclpackage_build})
	endif ()
	if (CCL_ADD_SKIN_RESOURCES_DEBUG)
		ccl_add_resources (${target} ${skins_${target}_OUTPUT})
	else ()
		ccl_add_resources (${target} ${skins_${target}_OUTPUT} CONFIG Release)
	endif ()
endmacro ()
