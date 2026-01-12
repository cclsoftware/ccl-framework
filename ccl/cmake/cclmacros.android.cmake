if (${VENDOR_HOST_PLATFORM} STREQUAL win)
	set (shader_tools_path "${CMAKE_ANDROID_NDK}/shader-tools/windows-x86_64")
elseif (${VENDOR_HOST_PLATFORM} STREQUAL mac)
	set (shader_tools_path "${CMAKE_ANDROID_NDK}/shader-tools/darwin-x86_64")
elseif (${VENDOR_HOST_PLATFORM} STREQUAL linux)
	set (shader_tools_path "${CMAKE_ANDROID_NDK}/shader-tools/linux-x86_64")
endif ()

find_program (glslc_executable NAMES glslc HINTS "${shader_tools_path}")
find_program (spirv_opt_executable NAMES spirv-opt HINTS "${shader_tools_path}")
find_program (spirv_cross_executable NAMES spirv-cross HINTS "${CCL_TOOLS_DIR}/bin/${VENDOR_HOST_PLATFORM}/vulkan" "${CCL_SPIRV_CROSS_DIR}")

# Add a 3D shader resource.
# @group android
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
macro (ccl_add_shader_resource target source)
	if (spirv_cross_executable)
		set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/shaders/${target}")
		if ("${VENDOR_OUTPUT_DIRECTORY}" MATCHES ".*/\.cxx/.*")
			set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/../../../../../${VENDOR_PLATFORM}/shaders/${target}")
		endif ()

		get_filename_component (source_name "${source}" NAME_WE)
		set (compiled_shader "${shaders_dir}/${source_name}.spv")
		set (shader_reflection "${shaders_dir}/${source_name}.json")
		set (dependency_file "${compiled_shader}.d")

		# compile and optimize spir-v shader and add it as a resource
		add_custom_command (
			OUTPUT "${compiled_shader}"
			DEPENDS "${source}"
			DEPFILE "${dependency_file}"
			COMMAND "${glslc_executable}" -I "${CCL_DIR}/.." -MD -MF "${dependency_file}" -DVULKAN_SHADER=1 -o "${compiled_shader}" "${source}"
			COMMAND "${spirv_opt_executable}" -O "${compiled_shader}" -o "${compiled_shader}"
		)
		ccl_add_resources (${target} "${compiled_shader}" ${ARGN})

		# compile reflection data and add it as a resource
		add_custom_command (
			OUTPUT "${shader_reflection}"
			DEPENDS "${compiled_shader}"
			COMMAND "${spirv_cross_executable}" "${compiled_shader}" "--reflect" "--output" "${shader_reflection}"
		)
		ccl_add_resources (${target} "${shader_reflection}" ${ARGN})

		target_sources (${target} PRIVATE ${source})
		source_group ("resource\\shaders" FILES ${source} ${compiled_shader})
	elseif (NOT spirv_cross_warning_issued)
		message (WARNING
			"The SPIRV-Cross shader compiler could not be found. "
			"In order to use custom shaders, please provide the path to a folder containing spirv-cross in the CCL_SPIRV_CROSS_DIR variable.")

		set (spirv_cross_warning_issued ON)
	endif ()
endmacro ()
