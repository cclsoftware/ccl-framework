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
# Filename    : cclmacros.mac.cmake
# Description : Mac CMake Macros
#
#************************************************************************************************

macro (ccl_set_debug_command target)
	set (cmake_arguments
		DEBUG_EXECUTABLE
		DEBUG_ARGUMENTS
	)
	cmake_parse_arguments ("ARG" "" "${cmake_arguments}" "" ${ARGN})

	if (NOT ARG_DEBUG_EXECUTABLE)
		message (FATAL_ERROR "ccl_set_debug_command called without argument DEBUG_EXECUTABLE")
	endif ()

	set_target_properties (${target} PROPERTIES 
		XCODE_SCHEME_EXECUTABLE "${ARG_DEBUG_EXECUTABLE}"
		XCODE_SCHEME_ARGUMENTS "${ARG_DEBUG_ARGUMENTS}"
	)
endmacro ()

# Add a 3D shader resource.
# @group mac
# @param {STRING} target  Name of the target to add a shader resource to.
# @param {STRING} source  Shader source file.
macro (ccl_add_shader_resource target source)
	set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/shaders/${target}")
	get_filename_component (source_name "${source}" NAME_WE)
	set (compiled_shader "${shaders_dir}/${source_name}.metallib")
	add_custom_command (
		OUTPUT "${compiled_shader}"
		DEPENDS "${source}"
		COMMAND xcrun -sdk macosx metal -gline-tables-only -frecord-sources -I "${CCL_DIR}/.." -c ${source} -o ${source}.air
		COMMAND xcrun -sdk macosx metallib ${source}.air -o  "${compiled_shader}" 
	)
	ccl_add_resources (${target} "${compiled_shader}" ${ARGN})
	target_sources (${target} PRIVATE ${source})
	source_group ("resource/shaders" FILES ${source} ${compiled_shader})
	ccl_add_resources (${target}  "${compiled_shader}"  PATH shaders)
endmacro ()
