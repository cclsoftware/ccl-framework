find_program (glslc_executable NAMES glslc HINTS Vulkan::glslc "${REPOSITORY_TOOLS_DIR}/bin/linux")
find_program (spirv_cross_executable NAMES spirv-cross HINTS Vulkan::SPIRV-Tools "${REPOSITORY_TOOLS_DIR}/bin/linux")
find_program (sed NAMES sed)

# Configure a plug-in library target
# @group wasm
# @param {STRING} target  Name of the target to configure.
macro (ccl_configure_plugin target)
	ccl_apply_wasm_flags (${target})
endmacro ()

# Add a 3D shader resource.
# @group wasm
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
macro (ccl_add_shader_resource target source)
	if (glslc_executable)
		set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/shaders/${target}")
		get_filename_component (source_name "${source}" NAME_WE)
		set (preprocessed_shader "${shaders_dir}/${source_name}.glsl")
		set (compiled_shader "${shaders_dir}/${source_name}.spv")
		set (shader_reflection "${shaders_dir}/${source_name}.json")
		set (dependency_file "${compiled_shader}.d")
		set (preprocessed_dependency_file "${compiled_shader}_preprocessed.d")
		add_custom_command (
			OUTPUT "${compiled_shader}"
			DEPENDS "${source}"
			DEPFILE "${dependency_file}"
			COMMAND "${glslc_executable}" -I "${CCL_DIR}/.." -MD -MF "${dependency_file}" -DVULKAN_SHADER=1 -o "${compiled_shader}" "${source}"
		)		
		# add a preprocessed glsl source file as a resource

		if (VENDOR_HOST_PLATFORM STREQUAL "win")
			set (sed_quotes "")
		else ()
			set (sed_quotes "'")
		endif ()

		add_custom_command (
			OUTPUT "${preprocessed_shader}"
			DEPENDS "${source}"
			DEPFILE "${preprocessed_dependency_file}"
			COMMAND "${glslc_executable}" -E -I "${CCL_DIR}/.." -MD -MF "${preprocessed_dependency_file}" -MT "${preprocessed_shader}" -DVULKAN_SHADER=0 -o "${preprocessed_shader}" "${source}"
			COMMAND ${sed} -i "${sed_quotes}s/#version 310 es/#version 300 es/g${sed_quotes}" ${preprocessed_shader}
			COMMAND ${sed} -i "${sed_quotes}s/layout *(location = .)//g${sed_quotes}" ${preprocessed_shader}
			COMMAND ${sed} -i "${sed_quotes}/#line.*/d${sed_quotes}" ${preprocessed_shader}
			COMMAND ${sed} -i "${sed_quotes}/#extension GL_GOOGLE_include_directive.*/d${sed_quotes}" ${preprocessed_shader}
		)
		ccl_add_resources (${target} "${preprocessed_shader}" ${ARGN})
		if (spirv_cross_executable)
			add_custom_command (
				OUTPUT "${shader_reflection}"
				DEPENDS "${compiled_shader}"
				COMMAND "${spirv_cross_executable}" "${compiled_shader}" "--reflect" "--output" "${shader_reflection}"
			)
			ccl_add_resources (${target} "${shader_reflection}" ${ARGN})
		else ()
			message (WARNING "Spirv-cross not found. 3D shader reflection won't work.")
		endif ()

		target_sources (${target} PRIVATE ${source})
		source_group ("resource\\shaders" FILES ${source} ${compiled_shader} ${shader_reflection})
	else ()
		message (WARNING "glslc not found. 3D shader support is disabled.")
	endif ()
endmacro ()

# Download font files.
# @group wasm
function (download_webfonts)
	set (webfonts_dir "${CMAKE_BINARY_DIR}/assets/webfonts")
	set (webfonts_filename "ccl-web-fonts-v1.0.zip")
	set (webfonts_archive "${webfonts_dir}/${webfonts_filename}")
	set (webfonts_output_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/fonts")
	if (NOT EXISTS "${webfonts_output_dir}/CCLFallbackGeneric.ttf")
		ccl_restore_from_cache (webfonts FILE_NAME "${webfonts_filename}" DESTINATION "${webfonts_dir}")
		if (NOT EXISTS "${webfonts_archive}")
			download_from_github_release (cclsoftware/ccl-web-fonts v1.0 "${webfonts_filename}" "${webfonts_dir}" PUBLIC SOURCE)
			if (NOT EXISTS "${webfonts_archive}")
				message (WARNING "Failed to download web fonts.")
			else ()
				set (webfonts_downloaded ON)
			endif ()
		endif ()
		if (EXISTS "${webfonts_archive}")
			if (webfonts_downloaded)
				ccl_add_to_cache (webfonts "${webfonts_archive}")
			endif ()
			file (ARCHIVE_EXTRACT INPUT "${webfonts_archive}" DESTINATION "${webfonts_dir}" TOUCH)
			file (REMOVE "${webfonts_archive}")
		endif ()

		if (EXISTS "${webfonts_dir}")
			file (MAKE_DIRECTORY "${webfonts_output_dir}")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans/static/NotoSans-Regular.ttf" "${webfonts_output_dir}/CCLFallbackGeneric.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Emoji/static/NotoEmoji-Regular.ttf" "${webfonts_output_dir}/CCLFallbackEmoji.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_JP/static/NotoSansJP-Regular.ttf" "${webfonts_output_dir}/CCLFallbackJP.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_SC/static/NotoSansSC-Regular.ttf" "${webfonts_output_dir}/CCLFallbackSC.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_TC/static/NotoSansTC-Regular.ttf" "${webfonts_output_dir}/CCLFallbackTC.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_KR/static/NotoSansKR-Regular.ttf" "${webfonts_output_dir}/CCLFallbackKR.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_Symbols/static/NotoSansSymbols-Regular.ttf" "${webfonts_output_dir}/CCLFallbackSymbols.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/Noto_Sans_Symbols_2/static/NotoSansSymbols2-Regular.ttf" "${webfonts_output_dir}/CCLFallbackSymbols2.ttf")
			file (RENAME "${webfonts_dir}/ccl-web-fonts-1.0/LICENSE.txt" "${webfonts_output_dir}/LICENSE.txt")
		endif ()
	endif ()
	
	get_property (ccl_fallback_fonts_installed GLOBAL PROPERTY ccl_fallback_fonts_installed)
	if (NOT ccl_fallback_fonts_installed AND VENDOR_APPLICATION_RUNTIME_DIRECTORY AND EXISTS "${webfonts_output_dir}")
		install (DIRECTORY "${webfonts_output_dir}" DESTINATION ${VENDOR_APPLICATION_RUNTIME_DIRECTORY})
		set_property (GLOBAL PROPERTY ccl_fallback_fonts_installed ON)
	endif ()
endfunction ()
