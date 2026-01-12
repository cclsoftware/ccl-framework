option (CCL_ENABLE_VULKAN "Enable Vulkan support for cclgui" ON)
option (CCL_ENABLE_OPENGLES2 "Enable OpenGL ES 2 support for cclgui" ON)

set (Vulkan_FIND_QUIETLY ON)
find_package (Vulkan COMPONENTS glslc SPIRV-Tools)
find_program (glslc_executable NAMES glslc HINTS Vulkan::glslc "${REPOSITORY_TOOLS_DIR}/bin/linux")
find_program (spirv_cross_executable NAMES spirv-cross HINTS Vulkan::SPIRV-Tools "${REPOSITORY_TOOLS_DIR}/bin/linux")
find_program (sed NAMES sed)

# Add a 3D shader resource.
# @group linux
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
		if (CCL_ENABLE_VULKAN)
			# add the precompiled spir-v shader as a resource
			ccl_add_resources (${target} "${compiled_shader}" ${ARGN})
		endif ()
		if (CCL_ENABLE_OPENGLES2)
			# add a preprocessed glsl source file as a resource
			add_custom_command (
				OUTPUT "${preprocessed_shader}"
				DEPENDS "${source}"
				DEPFILE "${preprocessed_dependency_file}"
				COMMAND "${glslc_executable}" -E -I "${CCL_DIR}/.." -MD -MF "${preprocessed_dependency_file}" -MT "${preprocessed_shader}" -DVULKAN_SHADER=0 -o "${preprocessed_shader}" "${source}"
				COMMAND ${sed} -i "'/#line.*/d'" ${preprocessed_shader}
				COMMAND ${sed} -i "'/#extension GL_GOOGLE_include_directive.*/d'" ${preprocessed_shader}
			)
			ccl_add_resources (${target} "${preprocessed_shader}" ${ARGN})
		endif ()	
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

# Use a D-Bus interface.
# Generates source files and headers from an xml interface description.
# @group linux
# @param {STRING} target  Name of the target using the interface
# @param {STRING} interface  Name of an interface or absolute path to an interface xml description
macro (ccl_use_dbus_interface target interface)
	cmake_parse_arguments (dbus_params "SERVER;OPTIONAL" "" "" ${ARGN})

	unset (interface_file CACHE)
	set (interface_file "${interface}")
	if (NOT EXISTS "${interface_file}")
		unset (interface_file)
		find_file (interface_file NAMES "${interface}" "${interface}.xml" HINTS "/usr/share/dbus-1/interfaces" NO_CACHE)
	endif ()

	if (NOT EXISTS "${interface_file}")
		set (dbus_interface_FOUND OFF)
		if (NOT dbus_params_OPTIONAL)
			message (FATAL_ERROR "Couldn't find D-Bus interface file ${interface} (${interface_file})")
		else ()
			message (NOTICE "Couldn't find D-Bus interface file ${interface} (${interface_file})")
		endif ()
	else ()
		set (dbus_interface_FOUND ON)

		set (dbus_dir "${CMAKE_BINARY_DIR}/dbus")
		get_filename_component (interface_name "${interface_file}" NAME_WLE)
		string (REPLACE "." "-" generated_name "${interface_name}")
		string (TOLOWER "${generated_name}" generated_name)

		if (dbus_params_SERVER)
			set (dbus_output "${dbus_dir}/${generated_name}-server.h")
			set (dbus_target_name "dbus_${generated_name}-server")
			set (dbus_arguments "--adaptor=${dbus_output}")
		else ()
			set (dbus_output "${dbus_dir}/${generated_name}-client.h")
			set (dbus_target_name "dbus_${generated_name}-client")
			set (dbus_arguments "--proxy=${dbus_output}")
		endif ()

		if (NOT TARGET ${dbus_target_name})
			find_package (sdbus-c++ 1...<2 REQUIRED)
			find_package (sdbus-c++-tools 1...<2 REQUIRED)

			find_program (xml2cpp NAMES sdbus-c++-xml2cpp HINTS "/usr/bin" SDBusCpp::sdbus-c++-xml2cpp "${REPOSITORY_TOOLS_DIR}/bin/linux")

			add_custom_command (
				OUTPUT "${dbus_output}"
				COMMAND ${xml2cpp} "${interface_file}" "${dbus_arguments}"
				DEPENDS "${interface_file}"
				COMMENT "Generating D-Bus wrappers for ${interface_name}."
			)

			add_custom_target (${dbus_target_name} DEPENDS ${dbus_output})
			set_target_properties (${dbus_target_name} PROPERTIES USE_FOLDERS ON FOLDER dbus)
		endif ()
		target_include_directories (${target} PRIVATE "${dbus_dir}")
		target_link_libraries (${target} PRIVATE SDBusCpp::sdbus-c++)
		add_dependencies (${target} ${dbus_target_name})
	endif ()
endmacro ()
