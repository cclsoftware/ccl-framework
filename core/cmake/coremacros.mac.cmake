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
# Filename    : coremacros.mac.cmake
# Description : Mac CMake Macros
#
#************************************************************************************************

# user identities
get_filename_component (user_documents_dir "~/Documents" ABSOLUTE)
list (APPEND VENDOR_IDENTITY_DIRS "${user_documents_dir}/CCL/identities")

# Add assets to a bundle
# @group mac
# @param {STRING} target_raw  Name of the target to add asset files to.
# @param {PATH} PATH  [optional] Destination subdirectory.
# @param {FILEPATH} FILES  [variadic] Asset files.
# @param {PATH} DIRECTORY  [variadic] Asset directories.
macro (ccl_process_assets target_raw)
	ccl_get_isolated_target ("${target_raw}" target)
	cmake_parse_arguments (asset_params "" "PATH" "DIRECTORY;FILES" ${ARGN})

	if (asset_params_PATH)
		set (path "${asset_params_PATH}")
	else ()
		set (path ".")
	endif ()

	if (asset_params_DIRECTORY)
		cmake_path (CONVERT "${asset_params_DIRECTORY}" TO_CMAKE_PATH_LIST assets NORMALIZE)
	else ()
		cmake_path (CONVERT "${asset_params_FILES}" TO_CMAKE_PATH_LIST assets NORMALIZE)
	endif ()
	target_sources (${target} PRIVATE ${assets})
	set_source_files_properties (${assets} PROPERTIES MACOSX_PACKAGE_LOCATION "${path}")
endmacro ()

# Copy resource files to a bundle.
# @group mac
# @param {STRING} target_raw  Name of a target to add resources to.
# @param {STRING} path  Subdirectory to place the resources in.
# @param args  Resource files.
macro (ccl_process_resources target_raw config path)
	ccl_get_isolated_target ("${target_raw}" target)
	set (resources ${ARGN})
	cmake_path (CONVERT "${resources}" TO_CMAKE_PATH_LIST resources NORMALIZE)
	foreach (resource IN ITEMS ${resources})
		cmake_path (GET resource EXTENSION ext)
		if (ext STREQUAL ".cmake")
			message (WARNING ".cmake file '${resource}' included in resources, this might be by error.")
		endif ()
		set_source_files_properties (${resource} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${path})
	endforeach ()
endmacro ()

# Process file path to find a bundle in the parant path of a file
# @group mac
# @param {STRING} bundle_dir  On succes contains path to the bundle which contains the binary
# @param {STRING} binary_path  Path to file within bundle, typically the binary.

macro (ccl_get_bundle_dir bundle_dir binary_path)
	set(path "${binary_path}")
	while (NOT path STREQUAL "/")
		cmake_path (GET path EXTENSION ext)
		if (ext STREQUAL ".framework" OR ext STREQUAL ".bundle" OR ext STREQUAL ".plugin")
			set (${bundle_dir} ${path})
			break ()
		endif ()
		cmake_path (GET path PARENT_PATH path)
	endwhile ()
endmacro ()

# Copy libs and bundles to a bundle.
# @group mac
# @param {STRING} target_raw  Name of a target to add the modules to, without isolation
# evaluates ${target}_FRAMEWORKS, ${target}_DYLIBS and ${target}_PLUGINS, etc. lists
macro (ccl_process_plugins target_raw)
	ccl_get_isolated_target ("${target_raw}" target)
	if (NOT TARGET ${target})
		return ()
	endif ()

	get_target_property (target_type ${target} TYPE)
	if (${target_type} STREQUAL "EXECUTABLE" OR ${target_type} STREQUAL "MODULE_LIBRARY")
		set (frameworks "")
		foreach (item IN ITEMS ${${target}_FRAMEWORKS})
			set (target_isimported FALSE)
			if (TARGET ${item})
				get_target_property (target_isimported ${item} IMPORTED)
			endif()
			if (${target_isimported})
				get_target_property (imported_location_debug ${item} IMPORTED_LOCATION_DEBUG)
				ccl_get_bundle_dir (bundle_dir_debug ${imported_location_debug})
				get_target_property (imported_location_release ${item} IMPORTED_LOCATION_RELEASE)
				ccl_get_bundle_dir (bundle_dir_release ${imported_location_release})
				list (APPEND frameworks "$<IF:$<CONFIG:Release,RelWithDebInfo>,${bundle_dir_release},${bundle_dir_debug}>")
			else ()
				list (APPEND frameworks $<TARGET_BUNDLE_DIR:${item}>)
			endif ()
		endforeach ()
		set (dylibs "")
		foreach (item IN ITEMS ${${target}_DYLIBS})
			list (APPEND dylibs $<TARGET_FILE:${item}>)
		endforeach ()
		set (plugins "")
		foreach (item IN ITEMS ${${target}_PLUGINS})
			if (${target_isimported})
				get_target_property (imported_location_debug ${item} IMPORTED_LOCATION_DEBUG)
				ccl_get_bundle_dir (bundle_dir_debug ${imported_location_debug})
				get_target_property (imported_location_release ${item} IMPORTED_LOCATION_RELEASE)
				ccl_get_bundle_dir (bundle_dir_release ${imported_location_release})
				list (APPEND plugins "$<IF:$<CONFIG:Release,RelWithDebInfo>,${bundle_dir_release},${bundle_dir_debug}>")
			else ()
				list (APPEND plugins $<TARGET_BUNDLE_DIR:${item}>)
			endif ()
			if(IOS)
				set (plugin_file "${item}.plugin")
				file (WRITE "${plugin_file}" "Lib=${item}.framework\n")
				ccl_add_assets (${target} FILES "${plugin_file}" PATH "$BUNDLE_PLUGINS_FOLDER_PATH")
			endif()
		endforeach ()
		set (sharedsupport "")
		foreach (item IN ITEMS ${${target}_SHAREDSUPPORT})
			list (APPEND sharedsupport $<TARGET_BUNDLE_DIR:${item}>)
		endforeach ()

		if(frameworks)
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Copying frameworks"
				COMMAND ${CMAKE_COMMAND} -E make_directory \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\"
				COMMAND rsync -rl ${frameworks} \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\"
			)
		endif ()
		if(dylibs)
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Copying dylibs"
				COMMAND ${CMAKE_COMMAND} -E make_directory \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\"
				COMMAND rsync ${dylibs} \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\"
			)
		endif()
		if(frameworks OR dylibs)
			add_custom_command (TARGET ${target} PRE_LINK
			COMMENT "Signing frameworks/dylibs"
			COMMAND xcrun codesign --sign \"$EXPANDED_CODE_SIGN_IDENTITY\" --force -o runtime -v \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\"/*
		)
		endif()
		if(plugins)
			if(IOS)
				set (plugin_target_dir \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\")
			else()
				set (plugin_target_dir \"$CODESIGNING_FOLDER_PATH/$BUNDLE_PLUGINS_FOLDER_PATH\")
			endif()
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Copying plugins"
				COMMAND ${CMAKE_COMMAND} -E make_directory ${plugin_target_dir}
				COMMAND rsync -rl ${plugins} ${plugin_target_dir}
			)
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Signing plugins"
				COMMAND xcrun codesign --sign \"$EXPANDED_CODE_SIGN_IDENTITY\" --force -o runtime --preserve-metadata=entitlements -v ${plugin_target_dir}/*
			)
		endif ()
		if(sharedsupport)
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Copying shared support bundles"
				COMMAND ${CMAKE_COMMAND} -E make_directory \"$CODESIGNING_FOLDER_PATH/$BUNDLE_CONTENTS_FOLDER_PATH_deep/SharedSupport/Bundles\"
				COMMAND rsync -rl ${sharedsupport} \"$CODESIGNING_FOLDER_PATH/$BUNDLE_CONTENTS_FOLDER_PATH_deep/SharedSupport/Bundles\"
			)
			add_custom_command (TARGET ${target} PRE_LINK
				COMMENT "Signing shared support bundles"
				COMMAND xcrun codesign --sign \"$EXPANDED_CODE_SIGN_IDENTITY\" --force -o runtime --preserve-metadata=entitlements -v \"$CODESIGNING_FOLDER_PATH/$BUNDLE_CONTENTS_FOLDER_PATH_deep/SharedSupport/Bundles\"/*
			)
		endif()
	endif()

endmacro ()

macro (ccl_process_plugins_type target_raw type subdir plugins)
	ccl_get_isolated_target ("${target_raw}" target)
	set (files "")
	if(IOS)
		set (target_subdir \"$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH\")
	else()
		set (target_subdir \"$CODESIGNING_FOLDER_PATH/$BUNDLE_CONTENTS_FOLDER_PATH_deep/${subdir}\")
	endif()
	foreach (item IN ITEMS ${plugins})
		list (APPEND files $<${type}:${item}>)
		if(IOS)
			set (plugin_file "${item}.plugin")
			file (WRITE "${plugin_file}" "Lib=${item}.framework\n")
			ccl_add_assets (${target} FILES "${plugin_file}" PATH "${subdir}")
		endif()
	endforeach ()
	add_custom_command (TARGET ${target} PRE_LINK
		COMMENT "Copying ${type} files"
		COMMAND ${CMAKE_COMMAND} -E make_directory ${target_subdir}
		COMMAND rsync -rl ${files} ${target_subdir}
	)
	add_custom_command (TARGET ${target} PRE_LINK
		COMMENT "Signing ${type} files"
		COMMAND xcrun codesign --sign \"$EXPANDED_CODE_SIGN_IDENTITY\" --force -o runtime -v  ${target_subdir}/*
	)
endmacro ()

# Create localized dummy files in Mac or iOS Bundles.
# @group mac
# @param {STRING} target_raw  Name of a target.
macro (core_add_bundle_localizations target_raw unused1 unused2)
	ccl_get_isolated_target ("${target_raw}" target)
	set (locales en ${ARGN})
	foreach (loc IN ITEMS ${locales})
		list (APPEND target_dirs "\"$BUILT_PRODUCTS_DIR/$UNLOCALIZED_RESOURCES_FOLDER_PATH/${loc}.lproj\"")
	endforeach ()

	add_custom_command (TARGET ${target} POST_BUILD
		COMMENT "Generating .lproj directories"
		COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dirs}
	)

endmacro ()

# Generate an export definition for a target.
# @group mac
# @param {STRING} target_raw  Name of the target to add resource files to.
# @param args  [variadic] List of functions to export.
macro (ccl_generate_export_file target_raw)
	ccl_get_isolated_target ("${target_raw}" target)
	cmake_parse_arguments (export_params "PRIVATE" "" "" ${ARGN})

	set (exports ${export_params_UNPARSED_ARGUMENTS})
	
	set (export_base_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/exports")
	set (export_file "${export_base_dir}/${target}.exp")
	
	file (MAKE_DIRECTORY "${export_base_dir}")
	file (WRITE "${export_file}" "")
	
	foreach (exported_function IN ITEMS ${exports})
		file (APPEND "${export_file}"
			"_${exported_function}
")
	endforeach ()
	
	set_source_files_properties ("${export_file}" PROPERTIES GENERATED ON)
	target_sources (${target} PRIVATE "${export_file}")
	
	get_target_property (target_type ${target} TYPE)
	if (target_type STREQUAL "SHARED")
		set_property (TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-exported_symbols_list,${export_file}")
	else ()
		target_link_options (${target} PRIVATE -exported_symbols_list "${export_file}")
	endif ()

endmacro ()

# Configure the target vendor.
# @group mac
# @param {STRING} vendor Vendor name.
macro (ccl_configure_vendor target vendor)
	if (DEFINED FORCE_SIGNING_TEAMID)
		set_target_properties (${target} PROPERTIES
			XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${FORCE_SIGNING_TEAMID}")
	endif()
endmacro ()

# Configure the project vendor.
# @group mac
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Code Signing
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/signing.cmake)
		include (${CMAKE_CURRENT_SOURCE_DIR}/signing.cmake)
	endif ()
	if (NOT DEFINED FORCE_SIGNING_TEAMID)
		set (CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${SIGNING_TEAMID_MAC}")
	else()
		set (CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${FORCE_SIGNING_TEAMID}")
	endif()

	# Installation directories (if using CPack)
	set (VENDOR_LIBRARY_DESTINATION "Frameworks/$<PLATFORM_ID>")
	set (VENDOR_PLUGINS_DESTINATION "${VENDOR_LIBRARY_DESTINATION}/Plugins")
	set (VENDOR_STATIC_LIBRARY_DESTINATION "lib/$<PLATFORM_ID>")
	set (VENDOR_CMAKE_FILES_DESTINATION "Frameworks/cmake/ccl")
	set (VENDOR_BINARY_DESTINATION "bin")
	set (VENDOR_PUBLIC_HEADERS_DESTINATION "include")
	set (VENDOR_APPSUPPORT_DESTINATION "share")
endmacro ()

# Copy plugins to a bundle in debug builds only.
# @group mac
# @param {STRING} target  Name of a target to add the plugins to.
# @param {STRING} pluginTarget  Name of a plugin target to be added.
macro(add_plugin_to_mac_debug_bundle target_raw pluginTarget)
	ccl_get_isolated_target ("${target_raw}" target)
	set (pluginBundlePath "$<TARGET_BUNDLE_DIR:${pluginTarget}>")
	set (pluginBundleName "$<TARGET_FILE_NAME:${pluginTarget}>.bundle")
	set (pluginDestination "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/PlugIns/${pluginBundleName}")

	set (copyCommand "$<$<CONFIG:Debug>:${CMAKE_COMMAND};-E;copy_directory_if_different;${pluginBundlePath};${pluginDestination}>") 
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND "${copyCommand}"
		COMMAND_EXPAND_LISTS
	)
endmacro()

# Setup platform-specific options to pass to CMake for external projects
# @group mac
# @param {STRING} target  Name of target to setup platform options for.
macro (ccl_setup_external_project_platform_options target)
	string (REPLACE ";" "|" ${target}_osx_architectures "${CMAKE_OSX_ARCHITECTURES}")

	list (APPEND ${target}_platform_options
		-DCMAKE_OSX_ARCHITECTURES:STRING=${${target}_osx_architectures}
	)
endmacro ()
