#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : cclmacros.win.cmake
# Description : Win CMake Macros
#
#************************************************************************************************

# Add a 3D shader resource.
# @group win
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
# @param {STRING} shader_type  Shader type, usually "Vertex" or "Pixel".
macro (ccl_add_shader_resource target source shader_type)
	
	string (MAKE_C_IDENTIFIER "${source}" source_id)
	get_property (compiled_shader GLOBAL PROPERTY ccl_compiled_shader_${source_id})

	if (compiled_shader)
		message (DEBUG "Shader resource '${source}' already has an output file defined: ${compiled_shader}. Re-using path.")
		get_filename_component (shaders_dir "${compiled_shader}" DIRECTORY)
	else ()
		set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/shaders/${target}")
		get_filename_component (source_name "${source}" NAME_WE)
		set (compiled_shader "${shaders_dir}/${source_name}.cso")
		set_property (GLOBAL PROPERTY ccl_compiled_shader_${source_id} "${compiled_shader}")
	endif ()
	
    set_source_files_properties (${compiled_shader} PROPERTIES GENERATED ON)
	ccl_add_resources (${target} "${compiled_shader}" ${ARGN})
	target_sources (${target} PRIVATE ${source})
	set_source_files_properties (${source} PROPERTIES VS_SHADER_TYPE ${shader_type} VS_SHADER_FLAGS "/I\"${CCL_DIR}/..\"" VS_SHADER_MODEL "5.0" VS_SHADER_OBJECT_FILE_NAME "${compiled_shader}")
	source_group ("source/graphics/3d/shader/d3d" FILES ${source})
	
endmacro ()

# Add a 3D vertex shader resource.
# @group win
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
# @param {STRING} shader_type  Shader type.
macro (ccl_add_vertexshader target source)
    ccl_add_shader_resource (${target} ${source} "Vertex" ${ARGN})
endmacro ()

# Add a 3D pixel shader resource.
# @group win
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
macro (ccl_add_pixelshader target source)
    ccl_add_shader_resource (${target} ${source} "Pixel" ${ARGN})
endmacro ()
