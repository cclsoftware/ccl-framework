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
	cmake_parse_arguments (params "" "PATH" "" ${ARGN})

	set (shaders_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/$<CONFIG>/shaders/${target}")
	get_filename_component (source_name "${source}" NAME_WE)
	set (compiled_shader "${shaders_dir}/${source_name}.metallib")

    set (METAL_DEBUG_FLAGS
        "$<$<CONFIG:Debug>:-gline-tables-only>"
        "$<$<CONFIG:Debug>:-frecord-sources>"
    )
    set (METAL_RELEASE_FLAGS
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:Release>:-ffast-math>"
    )

	add_custom_command (
		OUTPUT "${compiled_shader}"
		DEPENDS "${source}"
		COMMAND xcrun -sdk macosx metal ${METAL_DEBUG_FLAGS} ${METAL_RELEASE_FLAGS} -I "${CCL_DIR}/.." -c ${source} -o ${source}.air
		COMMAND xcrun -sdk macosx metallib ${source}.air -o  "${compiled_shader}"
		COMMAND_EXPAND_LISTS
        VERBATIM
	)

	add_custom_command (TARGET ${target} PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/${params_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${compiled_shader}"
                "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/${params_PATH}/${source_name}.metallib"
        VERBATIM
    )

	target_sources (${target} PRIVATE ${source})
	source_group ("shaders" FILES ${source})

	# Phony target hides the genex path from add_dependencies.
	set (shader_tgt "${target}_shader_${source_name}")
	add_custom_target (${shader_tgt} DEPENDS "${compiled_shader}")
	set_target_properties (${shader_tgt} PROPERTIES FOLDER shaders)
	add_dependencies (${target} ${shader_tgt})

endmacro ()
