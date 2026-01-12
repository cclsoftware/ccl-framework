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
# Filename    : coremacros.linux.cmake
# Description : Linux CMake Macros
#
#************************************************************************************************

# user identities
set (user_documents_dir "$ENV{XDG_DOCUMENTS_DIR}")
if (NOT user_documents_dir)
	get_filename_component (user_documents_dir "~/Documents" ABSOLUTE)
endif ()
list (APPEND VENDOR_IDENTITY_DIRS "${user_documents_dir}/CCL/identities")

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "linux_x64" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(i386)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "linux_x86" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "linux_arm64" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "linux_arm" CACHE STRING "")
endif ()

# Install assets
# @group linux
# @param {STRING} target  Name of the target to add asset files to.
# @param {PATH} PATH  [optional] Destination subdirectory.
# @param {FILEPATH} FILES  [variadic] Asset files.
# @param {PATH} DIRECTORY  [variadic] Asset directories.
macro (ccl_process_assets target)
	if (NOT VENDOR_APPLICATION_RUNTIME_DIRECTORY)
		message (FATAL_ERROR "VENDOR_APPLICATION_RUNTIME_DIRECTORY not set")
	endif ()

	cmake_parse_arguments (asset_params "" "PATH" "DIRECTORY;FILES" ${ARGN})

	if (asset_params_DIRECTORY)
		cmake_path (CONVERT "${asset_params_DIRECTORY}" TO_CMAKE_PATH_LIST assets NORMALIZE)
		install (DIRECTORY ${assets} DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/${asset_params_PATH}")
	else ()
		cmake_path (CONVERT "${asset_params_FILES}" TO_CMAKE_PATH_LIST assets NORMALIZE)
		install (FILES ${assets} DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/${asset_params_PATH}")
	endif ()
endmacro ()

# Link resource files to a binary.
# @group linux
# @param {STRING} target  Name of a target to add resources to.
# @param {STRING} path  Subdirectory to place the resources in.
# @param args  Resource files.
macro (ccl_process_resources target config path)
	set (resources ${ARGN})
	cmake_path (CONVERT "${resources}" TO_CMAKE_PATH_LIST resources NORMALIZE)
	
	if (VENDOR_ENABLE_PARALLEL_BUILDS)
		set (rc_base_dir "${CMAKE_BINARY_DIR}/rc/${target}")
	else ()
		set (rc_base_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/rc/${target}")
	endif ()
	set (rc_dir "${rc_base_dir}/Resource")
	set (rc_script "${rc_base_dir}/rc_${target}.cmake")
	set (rc_zip "${rc_base_dir}/rc.zip")
	set (rc_cpp_script "${rc_base_dir}/rc_${target}_c.cmake")
	set (rc_cpp "${rc_base_dir}/rc.cpp")
	
	file (MAKE_DIRECTORY "${rc_dir}/${path}")
	
	string (TOLOWER "${config}" lowerconfig)
	string (TOLOWER "${CMAKE_BUILD_TYPE}" buildtype)

	get_property (${target}_processed_resource_files GLOBAL PROPERTY ${target}_processed_resource_files)

	set (new_resources "")
	foreach (resource_entry IN ITEMS ${resources})
		if (NOT "${resource_entry}" IN_LIST  ${target}_processed_resource_files)
			list (APPEND new_resources "${resource_entry}")
		endif ()
	endforeach ()

	set (rc_file_created OFF)
	ccl_check_file_created ("${rc_script}" rc_file_created)
	if (new_resources AND NOT rc_file_created AND ("${config}" STREQUAL "" OR "${lowerconfig}" STREQUAL "${buildtype}"))
		ccl_create_file_once ("${rc_script}" CONTENT "")
	endif ()

	if ("${config}" STREQUAL "" OR "${lowerconfig}" STREQUAL "${buildtype}")
		foreach (resource_entry IN ITEMS ${new_resources})
			file (APPEND "${rc_script}" "file (COPY \"${resource_entry}\" DESTINATION \"${rc_dir}/${path}\")\n")
		endforeach ()
	else ()
		message (VERBOSE "Skipping resources for configuration ${config}: ${new_resources}")
	endif ()	

	if (new_resources AND ("${config}" STREQUAL "" OR "${lowerconfig}" STREQUAL "${buildtype}"))
		if (NOT rc_file_created)
			find_program (zip NAMES zip)

			add_custom_command (
				OUTPUT "${rc_zip}"
				COMMAND ${CMAKE_COMMAND} -E rm -Rf "${rc_dir}"
				COMMAND ${CMAKE_COMMAND} -E make_directory "${rc_dir}"
				COMMAND ${CMAKE_COMMAND} -P "${rc_script}"
				COMMAND ${CMAKE_COMMAND} -E rm -f "${rc_zip}"
				COMMAND cd ${rc_dir} && ${zip} -rq0 "${rc_zip}" "./*" && cd ..
				DEPENDS ${new_resources}
				WORKING_DIRECTORY "${rc_base_dir}"
			)

			ccl_makebin_export (
				INPUT "${rc_zip}"
				OUTPUT "${rc_cpp}"
				SCRIPT_FILE "${rc_cpp_script}"
				PREFIX "CCLGetResource"
				VARIABLE_NAME "kResourceData"
				"${rc_cpp}" "${rc_zip}" "${rc_cpp_script}" "Resource"
			)

			target_sources (${target} PRIVATE "${rc_zip}" "${rc_cpp}")
			source_group ("cmake\\resource" FILES "${rc_zip}" "${rc_cpp}")
		else ()
			add_custom_command (
				OUTPUT "${rc_zip}" APPEND
				DEPENDS ${new_resources}
			)
		endif ()

		list (APPEND ${target}_processed_resource_files ${new_resources})
	endif ()

	set_property (GLOBAL PROPERTY ${target}_processed_resource_files "${${target}_processed_resource_files}")

endmacro ()

# Install scalable icon.
# @group linux
# @param {STRING} target  Name of a target to add resources to.
# @param {FILEPATH} icon  Path to an SVG icon.
macro (ccl_install_icon target icon)

	cmake_parse_arguments (icon_params "" "RENAME" "" ${ARGN})
	set (icon_name "${target}")
	if (icon_params_RENAME)
		set (icon_name "${icon_params_RENAME}")
	endif ()
	install (FILES "${icon}" DESTINATION "${VENDOR_ICONS_DIRECTORY}/scalable/apps/" RENAME "${icon_name}.svg")
endmacro ()

# Install icons from xcassets directory.
# @group linux
# @param {STRING} target  Name of a target to add resources to.
# @param {FILEPATH} iconset  Path to an iconset directory.
macro (ccl_install_icons_from_xcassets target iconset)
	cmake_parse_arguments (icon_params "" "RENAME" "" ${ARGN})
	set (icon_name "${target}")
	if (icon_params_RENAME)
		set (icon_name "${icon_params_RENAME}")
	endif ()

	file(READ "${iconset}/AppIcon.appiconset/Contents.json" contents_string)
	string (JSON images_string ERROR_VARIABLE error GET ${contents_string} images)
	if(error)
		message (WARNING "${error}")
	endif ()

	set (icon_sizes "")
	set (icon_names "")

	# Loop through each element of the JSON array (indices 0 though 1).
	foreach (index RANGE 999)
		string (JSON current_size ERROR_VARIABLE error GET ${images_string} ${index} size)
		if(error)
			continue ()
		endif ()
		string (JSON current_scale ERROR_VARIABLE error GET ${images_string} ${index} scale)
		if(error)
			continue ()
		endif ()
		string (JSON current_name ERROR_VARIABLE error GET ${images_string} ${index} filename)
		if(error)
			continue ()
		endif ()

		if(NOT "${current_scale}" STREQUAL "1x")
			message (DEBUG "Skipping icon \"${current_name}\" with scale ${current_scale} in ${iconset}.")
			continue ()
		endif ()

		string (REPLACE "x" ";" current_resolution ${current_size})
		list (GET current_resolution 0 current_resolution)
		if (current_resolution GREATER_EQUAL 1024) # Note: 1024x1024 exceeds the maximum icon size for some packaging formats
			message (DEBUG "Skipping icon \"${current_name}\" with resolution ${current_size}")
		endif ()

		list (APPEND icon_sizes "${current_size}")
		list (APPEND icon_names "${current_name}")

	endforeach ()

	foreach (item IN ZIP_LISTS icon_sizes icon_names)
		if (EXISTS "${iconset}/AppIcon.appiconset/${item_1}")
			message (DEBUG "Found icon \"${item_1}\" with size ${item_0} in ${iconset}.")
			install (FILES "${iconset}/AppIcon.appiconset/${item_1}" DESTINATION "${VENDOR_ICONS_DIRECTORY}/${item_0}/apps/" RENAME "${icon_name}.png")
		else ()
			message (WARNING "Couldn't find icon \"${item_1}\" in ${iconset}.")
		endif ()
	endforeach ()
endmacro ()

# Install desktop file.
# @group linux
# @param {FILEPATH} in  Path to a desktop file template.
# @param {STRING} targetid  Package ID of a target to install a desktop file for.
macro (ccl_install_desktop_file in targetid)
	set (output_file "${CMAKE_CURRENT_BINARY_DIR}/${targetid}.desktop")
	configure_file ("${in}" "${output_file}")
	install (FILES ${output_file} DESTINATION ${VENDOR_DESKTOPFILE_DIRECTORY})
endmacro ()

# Install appstream data file.
# @group linux
# @param {STRING} target  target to install a mime types definition file for.
# @param {FILEPATH} in  Path to a appstream data file template.
# @param {STRING} targetid  Package ID of a target to install an appstream data file for.
macro (ccl_install_appstream_file target in targetid)
	set (appstream_mimetypes "")
	foreach (mimetype IN ITEMS ${${target}_mimetypes})
		string (APPEND appstream_mimetypes "<mediatype>${mimetype}</mediatype>\n")
	endforeach ()
	string (TIMESTAMP appstream_date "%Y-%m-%d")
	set (output_file "${CMAKE_CURRENT_BINARY_DIR}/${targetid}.appdata.xml")
	configure_file ("${in}" "${output_file}")
	install (FILES ${output_file} DESTINATION ${VENDOR_METAINFO_DIRECTORY})
endmacro ()

# Install mime types definition file.
# @group linux
# @param {STRING} target  target to install a mime types definition file for.
# @param {FILEPATH} in  Path to a mime types definition file.
# @param {STRING} targetid  Package ID of a target to install a mime types definition file for.
macro (ccl_install_mimetypes target in targetid)
	set (output_file "${CMAKE_CURRENT_BINARY_DIR}/${targetid}.xml")
	string (REPLACE "/" "-" VENDOR_MIME_ICON_PREFIX "${VENDOR_MIME_PREFIX}${VENDOR_MIME_TYPE}")
	string (REPLACE "/" "-" ${target}_MIME_ICON_PREFIX "${VENDOR_MIME_PREFIX}${${target}_MIME_TYPE}")
	string (REPLACE "/" "-" CCL_MIME_ICON_PREFIX "${VENDOR_MIME_PREFIX}${CCL_MIME_TYPE}")
	configure_file ("${in}" "${output_file}")
	install (FILES "${output_file}" DESTINATION ${VENDOR_MIMETYPES_DIRECTORY})
endmacro ()

# Configure the project vendor.
# @group linux
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Installation directories
	if ("${VENDOR_PRODUCT_NAME}" STREQUAL "")
		set (VENDOR_PRODUCT_NAME "${CMAKE_PROJECT_NAME}")
	endif ()
	if (PROJECT_APPLICATION_RUNTIME_DIRECTORY)
		set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "${PROJECT_APPLICATION_RUNTIME_DIRECTORY}")
	else ()
		if (PROJECT_VENDOR_INSTALL_SUBDIR)
			set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "opt/${PROJECT_VENDOR_INSTALL_SUBDIR}/${VENDOR_PRODUCT_NAME}")
		else ()
			set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "opt/${VENDOR_PRODUCT_NAME}")
		endif ()
	endif ()

	if (VENDOR_APPLICATION_RUNTIME_DIRECTORY AND NOT "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}" STREQUAL ".")
		set (VENDOR_PLUGINS_RUNTIME_DIRECTORY "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/Plugins")
		set (VENDOR_PLATFORMINTEGRATION_DIRECTORY "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/PlatformIntegration")
	else ()
		set (VENDOR_PLUGINS_RUNTIME_DIRECTORY "Plugins")
		set (VENDOR_PLATFORMINTEGRATION_DIRECTORY "PlatformIntegration")		
	endif ()

	set (VENDOR_DESKTOPFILE_DIRECTORY "usr/share/applications")
	set (VENDOR_METAINFO_DIRECTORY "usr/share/metainfo")
	set (VENDOR_MIMETYPES_DIRECTORY "usr/share/mime/packages")
	set (VENDOR_ICONS_DIRECTORY "usr/share/icons/hicolor")

	set (VENDOR_LIBRARY_DESTINATION "usr/lib/${PROJECT_VERSION_SUBDIRECTORY}/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")

	string (REPLACE "/" "-" cmake_subdir "${PROJECT_VERSION_SUBDIRECTORY}")
	set (VENDOR_CMAKE_FILES_DESTINATION "usr/lib/cmake/${cmake_subdir}")

	set (VENDOR_BINARY_DESTINATION "usr/bin/${PROJECT_VERSION_SUBDIRECTORY}/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
	set (VENDOR_PUBLIC_HEADERS_DESTINATION "usr/include/${PROJECT_VERSION_SUBDIRECTORY}")
	set (VENDOR_APPSUPPORT_DESTINATION "usr/share/${PROJECT_VERSION_SUBDIRECTORY}")
	set (VENDOR_LICENSE_DESTINATION "usr/share/licenses/${PROJECT_VERSION_SUBDIRECTORY}")
endmacro ()

# Setup platform-specific options to pass to CMake for external projects
# @group linux
# @param {STRING} target  Name of target to setup platform options for.
macro (ccl_setup_external_project_platform_options target)
	string (REPLACE ";" "|" ${target}_find_root_path "${CMAKE_FIND_ROOT_PATH}")
	string (REPLACE ";" "|" ${target}_library_path "${CMAKE_LIBRARY_PATH}")
	string (REPLACE ";" "|" ${target}_program_path "${CMAKE_PROGRAM_PATH}")
	string (REPLACE ";" "|" ${target}_prefix_path "${CMAKE_PREFIX_PATH}")

	list (APPEND ${target}_platform_options
		-DCMAKE_CROSSCOMPILING=${CMAKE_CROSSCOMPILING}
		-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
		-DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
		-DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
		-DCMAKE_CXX_COMPILER_TARGET=${CMAKE_CXX_COMPILER_TARGET}
		-DCMAKE_ASM_COMPILER_TARGET=${CMAKE_ASM_COMPILER_TARGET}
		-DCMAKE_AR=${CMAKE_AR}
		-DCMAKE_RANLIB=${CMAKE_RANLIB}
		-DCMAKE_C_COMPILER_AR=${CMAKE_C_COMPILER_AR}
		-DCMAKE_C_COMPILER_RANLIB=${CMAKE_C_COMPILER_RANLIB}
		-DCMAKE_CXX_COMPILER_AR=${CMAKE_CXX_COMPILER_AR}
		-DCMAKE_CXX_COMPILER_RANLIB=${CMAKE_CXX_COMPILER_RANLIB}
		-DCMAKE_FIND_ROOT_PATH=${${target}_find_root_path}
		-DCMAKE_SYSROOT=${CMAKE_SYSROOT}
		-DCMAKE_LIBRARY_PATH=${${target}_library_path}
		-DCMAKE_PROGRAM_PATH=${${target}_program_path}
		-DCMAKE_PREFIX_PATH=${${target}_prefix_path}
		-DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
		-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM}
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY}
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=${CMAKE_TRY_COMPILE_TARGET_TYPE}
		-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON
		-DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
	)
endmacro ()
