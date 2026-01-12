include_guard (DIRECTORY)

find_package (ccltools REQUIRED COMPONENTS package)
ccl_find_program (msgfmt NAMES msgfmt HINTS "${CCL_TOOLS_BINDIR}/${VENDOR_HOST_PLATFORM}/gettext")
ccl_find_program (SHELL sh REQUIRED NAMES sh bash PATHS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer/Git/usr/bin" "C:/Program Files/Git/bin")

# use a translation package
macro (ccl_use_translation_packages apptarget)
	if(APPLE)
		core_add_bundle_localizations (${apptarget} ${ARGN})
	endif ()
	
	cmake_parse_arguments (translation_params "" "PROJECT" "" ${ARGN})
	
	set (app "${apptarget}")
	if (translation_params_PROJECT)
		set (app "${translation_params_PROJECT}")
	endif ()
	
	set (mo_tmp "${CMAKE_CURRENT_BINARY_DIR}/tmp/make_mo.sh")
	set (mo_command "${CMAKE_CURRENT_BINARY_DIR}/make_mo.sh")
	
	file (WRITE "${mo_tmp}" "#!${SHELL}
		"
	)

	set (all_mo_files "")
	set (all_po_files "")
	foreach (translations_DIR ${VENDOR_TRANSLATIONS_DIRS})
		file (GLOB_RECURSE po_files "${translations_DIR}/modules/**/*.po")
		string (REPLACE ".po" ".mo" mo_files "${po_files}")
		foreach (po_file ${po_files})
			string (REGEX REPLACE "\\.po" ".mo" mo_file "${po_file}")
			file (APPEND "${mo_tmp}" "
				${msgfmt} ${po_file} -o ${mo_file}"
			)
		endforeach ()
		list (APPEND all_mo_files ${mo_files})
		list (APPEND all_po_files ${po_files})
	endforeach()
	
	if (NOT TARGET makemo)
		file (GENERATE OUTPUT "${mo_command}" INPUT "${mo_tmp}"
			NEWLINE_STYLE UNIX
			FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)

		add_custom_command (OUTPUT ${all_mo_files}
			WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
			DEPENDS ${all_po_files}
			COMMENT "Generating .mo files"
			COMMAND "${SHELL}" "${mo_command}"
			VERBATIM
		)
		
		add_custom_target (makemo DEPENDS ${all_mo_files})
		set_target_properties (makemo PROPERTIES USE_FOLDERS ON FOLDER translations)
	endif ()

	set (targets "${translation_params_UNPARSED_ARGUMENTS}")
	foreach (target IN LISTS targets)
		foreach (translations_DIR ${VENDOR_TRANSLATIONS_DIRS})
			set (translation "${translations_DIR}/projects/${app}/${app}-${target}.langbuild")
			get_filename_component (translation "${translation}" ABSOLUTE)
			if (EXISTS "${translation}" AND NOT translations_${app}_${target}_command_generated)
				set (translations_${app}_${target}_command_generated ON)
				file (READ "${translation}" script_content)
				string (REGEX MATCH "\\(LanguagePack\\)\\=([^\r\n]*)" _ "${script_content}")
				set (langpack "${CMAKE_MATCH_1}")
				if (NOT langpack)
					continue ()
				endif ()
				
				set (translations_output_dir "${translations_DIR}/projects/${app}")
				if (VENDOR_ENABLE_PARALLEL_BUILDS)
					set (translations_output_dir "${CMAKE_BINARY_DIR}/translations/projects/${app}")
				endif ()
				file (MAKE_DIRECTORY ${translations_output_dir})

				set (translations_${app}_${target}_OUTPUT "${translations_output_dir}/languages/${langpack}.langpack")
				get_filename_component (translations_${app}_${target}_OUTPUT "${translations_${app}_${target}_OUTPUT}" ABSOLUTE)
			
				set (depfile "${translations_${app}_${target}_OUTPUT}.d")
			
				cmake_path (NATIVE_PATH depfile NORMALIZE nativeDepfile)
				cmake_path (NATIVE_PATH translation NORMALIZE translation)
				cmake_path (NATIVE_PATH translations_output_dir NORMALIZE translations_output_dir)
			
				add_custom_command (OUTPUT ${translations_${app}_${target}_OUTPUT}
					COMMAND ${cclpackage} -dest "${translations_output_dir}" -batch "${translation}" -depfile "${nativeDepfile}"
					DEPENDS makemo
					DEPFILE "${depfile}"
					WORKING_DIRECTORY "${translations_DIR}/projects/${app}"
					VERBATIM USES_TERMINAL
				)
				list (APPEND translations_${app} translations_${app}_${target})
				list (APPEND translations_${app}_OUTPUTS ${translations_${app}_${target}_OUTPUT})

				list (APPEND ${apptarget}_sources ${translations_${app}_OUTPUTS})
				source_group ("cmake\\translation" FILES ${translations_${app}_OUTPUTS})

				break ()

			endif ()
		endforeach ()
		if (NOT translations_${app}_${target}_command_generated)
			message (FATAL_ERROR "Couldn't find translation project for ${app} ${target}")
		endif ()
	endforeach ()

	ccl_add_assets (${apptarget} FILES ${translations_${app}_OUTPUTS} PATH "Languages")
endmacro ()
