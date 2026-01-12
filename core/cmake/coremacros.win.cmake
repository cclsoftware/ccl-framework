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
# Filename    : coremacros.win.cmake
# Description : Windows CMake Macros
#
#************************************************************************************************

# user identities
get_filename_component (user_documents_dir "[HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders;Personal]" ABSOLUTE)
list (APPEND VENDOR_IDENTITY_DIRS "${user_documents_dir}/CCL/identities")

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "win_x64" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(i386)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "win_x86" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64ec)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "win_arm64ec" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "win_arm64" CACHE STRING "")
endif ()

# Generate an .rc file for the resource compiler
# @group win
# @param {STRING} target  Name of a target to add resources to.
# @param {STRING} path  Subdirectory to place the resources in.
# @param args  Resource files.
macro (ccl_process_resources target config path)
	set (resources ${ARGN})
	cmake_path (CONVERT "${resources}" TO_CMAKE_PATH_LIST resources NORMALIZE)
	
	set (rc_base_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/rc/${target}")
	set (rc_file "${rc_base_dir}/${target}.rc")
	
	file (MAKE_DIRECTORY "${rc_base_dir}")

	set (rc_file_created OFF)
	ccl_check_file_created (${rc_file} rc_file_created)
	if (NOT rc_file_created)
		
		ccl_create_file_once ("${rc_file}" CONTENT "#define APSTUDIO_READONLY_SYMBOLS
			#include \"windows.h\"
						
			VS_VERSION_INFO VERSIONINFO
			 FILEVERSION ${${target}_VERSION_MAJOR}, ${${target}_VERSION_MINOR}, ${${target}_VERSION_REVISION}, 0
			 PRODUCTVERSION ${${target}_VERSION_MAJOR}, ${${target}_VERSION_MINOR}, ${${target}_VERSION_REVISION}, 0
			 FILEFLAGSMASK 0x3fL
			#ifdef _DEBUG
			 FILEFLAGS 0x1L
			#else
			 FILEFLAGS 0x0L
			#endif
			 FILEOS 0x40004L
			 FILETYPE 0x1L
			 FILESUBTYPE 0x0L
			BEGIN
				BLOCK \"StringFileInfo\"
				BEGIN
					BLOCK \"040004e4\"
					BEGIN
						VALUE \"CompanyName\", \"${${target}_COMPANY_NAME}\"
						VALUE \"FileDescription\", \"${${target}_FILE_DESCRIPTION}\"
						VALUE \"FileVersion\", \"${${target}_VERSION}\"
						VALUE \"InternalName\", \"${${target}_PACKAGE_ID}\"
						VALUE \"LegalCopyright\", \"${${target}_COPYRIGHT}\"
						VALUE \"ProductName\", \"${${target}_NAME}\"
						VALUE \"ProductVersion\", \"${${target}_VERSION}\"
					END
				END
				BLOCK \"VarFileInfo\"
				BEGIN
					VALUE \"Translation\", 0x400, 1252
				END
			END
		")
		
		foreach (item IN ITEMS ${${target}_icons})
			if ("${icon_id}" STREQUAL "")
				set (icon_id "${item}")
				continue ()
			endif ()
			set (icon_path "${item}")

			file (APPEND "${rc_file}"
				"${icon_id} ICON DISCARDABLE \"${icon_path}\"
			")
			
			set (icon_id "")
		endforeach ()
		
		target_sources (${target} PRIVATE "${rc_file}")
		source_group ("resource" FILES "${rc_file}")
	endif ()

	string (TOLOWER "${config}" lowerconfig)
	if ("${lowerconfig}" STREQUAL "release")
		file (APPEND "${rc_file}" 
			"#ifndef _DEBUG
		")
	elseif ("${lowerconfig}" STREQUAL "debug")
		file (APPEND "${rc_file}" 
			"#ifdef _DEBUG
		")
	elseif (NOT "${config}" STREQUAL "")
		message (FATAL_ERROR "Unknown configuration ${config}")
	endif ()

	get_property (${target}_processed_resource_files GLOBAL PROPERTY ${target}_processed_resource_files)

	list (REMOVE_DUPLICATES resources)
	foreach (resource_entry IN ITEMS ${resources})

		if (EXISTS "${resource_entry}")
			file (GLOB_RECURSE resource_files LIST_DIRECTORIES false "${resource_entry}" "${resource_entry}/*")
		else ()
			set (resource_files "${resource_entry}")
		endif ()
		
		foreach (resource IN ITEMS ${resource_files})
			cmake_path (GET resource FILENAME resource_file)

			if ("${resource}" IN_LIST  ${target}_processed_resource_files)
				continue ()
			endif ()
			list (APPEND ${target}_processed_resource_files "${resource}")
			
			cmake_path (GET resource_entry PARENT_PATH resource_basepath)
			cmake_path (RELATIVE_PATH resource BASE_DIRECTORY "${resource_basepath}" OUTPUT_VARIABLE resource_path)
			cmake_path (GET resource_path PARENT_PATH resource_path)
			set (prefix "")
			if (NOT "${path}" STREQUAL "")
				set (prefix "${prefix}${path}/")
			endif ()
			if (NOT "${resource_path}" STREQUAL "" AND NOT "${resource_path}" STREQUAL "./")
				set (prefix "${prefix}${resource_path}/")
			endif ()
			string (REPLACE "@" "&#40;" full_resource_path "${prefix}${resource_file}")
			string (REPLACE "'" "&#27;" full_resource_path "${full_resource_path}")
			string (REPLACE " " "&#20;" full_resource_path "${full_resource_path}")
			file (APPEND "${rc_file}" 
				"${full_resource_path} RCDATA DISCARDABLE \"${resource}\"
			")
		endforeach ()
	endforeach ()
	
	set_property (GLOBAL PROPERTY ${target}_processed_resource_files "${${target}_processed_resource_files}")

	if (NOT "${config}" STREQUAL "")
		file (APPEND "${rc_file}" 
			"#endif
		")
	endif ()
endmacro ()

# Add a manifest file to a target
# @group win
# @param {STRING} target  Name of a target to add resources to.
# @param {FILEPATH} file  Path to a manifest file.
macro (ccl_embed_manifest target file)
	set (filepath "${file}")
	cmake_path (GET filepath EXTENSION LAST_ONLY file_extension)
	if (NOT "${file_extension}" STREQUAL "manifest")
		set (rc_base_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/rc/${target}")
		file (MAKE_DIRECTORY "${rc_base_dir}")
		set (manifest_file "${rc_base_dir}/${target}.manifest")
		configure_file ("${file}" "${manifest_file}" COPYONLY)
		target_sources (${target} PRIVATE "${manifest_file}")
		source_group ("resource" FILES "${manifest_file}")
	else ()
		target_sources (${target} PRIVATE "${file}")
		source_group ("resource" FILES "${file}")
	endif ()
endmacro ()

# Generate an export definition for a target.
# @group win
# @param {STRING} target  Name of the target to add resource files to.
# @param args  [variadic] List of functions to export.
macro (ccl_generate_export_file target)
	cmake_parse_arguments (export_params "PRIVATE" "" "" ${ARGN})

	set (exports ${export_params_UNPARSED_ARGUMENTS})
	
	set (export_prefix "")
	if (export_params_PRIVATE)
		set (export_suffix " PRIVATE")
	endif ()
	
	set (export_base_dir "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/exports")
	set (export_file "${export_base_dir}/${target}.def")
	
	file (MAKE_DIRECTORY "${export_base_dir}")
	file (WRITE "${export_file}" "EXPORTS
")
	
	foreach (exported_function IN ITEMS ${exports})
		file (APPEND "${export_file}"
			"	${exported_function}${export_suffix}
")
	endforeach ()
	
	set_source_files_properties ("${export_file}" PROPERTIES GENERATED ON)
	target_sources (${target} PRIVATE "${export_file}")
	source_group ("resource" FILES "${export_file}")
endmacro ()

# Set the debug command for a target.
# @group win
# @param {STRING} target  Name of the target to set the debug command for.
# @param {STRING} DEBUG_EXECUTABLE  Path to the executable to run when debugging.
# @param {STRING} DEBUG_ARGUMENTS  [optional] Additional arguments passed to the executable when debugging.
# @param args  [variadic] List of functions to export.
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
        VS_DEBUGGER_COMMAND "${ARG_DEBUG_EXECUTABLE}"
		VS_DEBUGGER_COMMAND_ARGUMENTS "${ARG_DEBUG_ARGUMENTS}"
	)
endmacro ()

# Get arguments to be passed to a sign script
# @group win
# @param {STRING} result Name of the result variable.
# @param {STRING} vendor Vendor name.
macro (ccl_get_sign_args result vendor)
	if ("${vendor}" STREQUAL "")
		message (FATAL_ERROR "No vendor set.")
	endif ()
	
	find_program (signtool_binary NAMES signtool 
		PATHS 
		"[HKLM/SOFTWARE/Microsoft/Microsoft SDKs/Windows/v10.0;InstallationFolder]/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
		"[HKLM/SOFTWARE/WOW6432Node/Microsoft/Microsoft SDKs/Windows/v10.0;InstallationFolder]/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
		"$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
		"${CCL_TOOLS_BINDIR}/win" 
		REGISTRY_VIEW BOTH
		PATH_SUFFIXES signtools signtool x64 x86 arm64
	)

	set (${result} "")
	if (signtool_binary)
		cmake_path (NATIVE_PATH signtool_binary NORMALIZE signtool_binary_native)
		list (APPEND ${result} "/signtool" "${signtool_binary_native}")		
	endif ()
	if (${vendor}_SIGNING_CERTIFICATE_THUMBPRINT)
		list (APPEND ${result} "/thumbprint" "${${vendor}_SIGNING_CERTIFICATE_THUMBPRINT}")		
	else ()
		list (APPEND ${result} "/cert" "${${vendor}_SIGNING_CERTIFICATE}")
	endif ()
	if (${vendor}_SIGNING_TIMESTAMP_SERVER)
		list (APPEND ${result} "/timestampserver" "${${vendor}_SIGNING_TIMESTAMP_SERVER}")
	endif ()
	if (${vendor}_SIGNING_AZURE_CLIENTID AND ${vendor}_SIGNING_AZURE_TENANTID AND ${vendor}_SIGNING_AZURE_KEYVAULT)
		list (APPEND ${result} "/client" "${${vendor}_SIGNING_AZURE_CLIENTID}")
		list (APPEND ${result} "/tenant" "${${vendor}_SIGNING_AZURE_TENANTID}")
		list (APPEND ${result} "/keyvault" "${${vendor}_SIGNING_AZURE_KEYVAULT}")
		list (APPEND ${result} "/secret" "$ENV{${vendor}_SIGNING_AZURE_SECRET}")
	endif ()
endmacro ()

# Reset vendor variables before configuring a new vendor.
# @group win
macro (ccl_reset_project_vendor_variables)
	set (SIGNING_CERTIFICATE_WIN "")
	set (SIGNING_CERTIFICATE_THUMBPRINT_WIN "")
	set (SIGNING_TIMESTAMP_SERVER_WIN "")
	set (SIGNING_AZURE_CLIENTID_WIN "")
	set (SIGNING_AZURE_TENANTID_WIN "")
	set (SIGNING_AZURE_KEYVAULT_WIN "")
	set (SIGNING_PRE_COMMAND_WIN "")
	set (SIGNING_POST_COMMAND_WIN "")
endmacro ()

# Configure the project vendor.
# @group win
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Include file for NSIS packaging scripts
	configure_file ("${CCL_IDENTITIES_DIR}/shared/vendor.nsh.in" "${vendor_include_directory}/vendor.nsh")
	cmake_path (NATIVE_PATH vendor_include_directory NORMALIZE vendor_include_directory_native)
	set (VENDOR_NSIS_INCLUDE_DIR "${vendor_include_directory_native}")

	# Code Signing
	option (VENDOR_ENABLE_CODESIGNING "Enable code signing for build outputs and generated istallers" OFF)

	set (${vendor}_SIGNING_CERTIFICATE "${SIGNING_CERTIFICATE_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_CERTIFICATE_THUMBPRINT "${SIGNING_CERTIFICATE_THUMBPRINT_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_TIMESTAMP_SERVER "${SIGNING_TIMESTAMP_SERVER_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_AZURE_CLIENTID "${SIGNING_AZURE_CLIENTID_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_AZURE_TENANTID "${SIGNING_AZURE_TENANTID_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_AZURE_KEYVAULT "${SIGNING_AZURE_KEYVAULT_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_PRE_COMMAND "${SIGNING_PRE_COMMAND_WIN}" CACHE STRING "" FORCE)
	set (${vendor}_SIGNING_POST_COMMAND "${SIGNING_POST_COMMAND_WIN}" CACHE STRING "" FORCE)
	
	set (sign_target_name "sign_${vendor}_binaries")
	if (CCL_ISOLATION_POSTFIX)
		string (APPEND sign_target_name ".${CCL_ISOLATION_POSTFIX}")
	endif ()
	
	if (VENDOR_ENABLE_CODESIGNING AND NOT TARGET "${sign_target_name}")
		ccl_get_sign_args (sign_args "${vendor}")
		
		find_program (CMD REQUIRED NAMES cmd)
		find_file (sign_script REQUIRED NAMES "sign_binaries.bat" PATHS "${REPOSITORY_ROOT}/build/win" "${CCL_REPOSITORY_ROOT}/build/win")
		cmake_path (NATIVE_PATH sign_script NORMALIZE sign_script_native)
		find_file (sign_extensions_script NAMES "sign_extensions.bat" PATHS "${REPOSITORY_ROOT}/build/win")
		cmake_path (NATIVE_PATH sign_extensions_script NORMALIZE sign_extensions_script_native)
		
		ccl_get_build_output_directory (artifacts_directory)
	
		if (sign_script AND sign_extensions_script)
		
			add_custom_command (OUTPUT ${sign_target_name}_run_always
				COMMENT "Signing binaries"
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_PRE_COMMAND}"
				COMMAND "${CMD}" /c "${sign_script_native}" ${sign_args} "${artifacts_directory}"
				COMMAND "${CMD}" /c "${sign_extensions_script_native}" ${sign_args} ""
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_POST_COMMAND}"
				VERBATIM
			)
		
		elseif (sign_script)
			
			add_custom_command (OUTPUT ${sign_target_name}_run_always
				COMMENT "Signing binaries"
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_PRE_COMMAND}"
				COMMAND "${CMD}" /c "${sign_script_native}" ${sign_args} "${artifacts_directory}"
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_POST_COMMAND}"
				VERBATIM
			)
			
		else ()
		
			find_file (sign_file_script REQUIRED NAMES "sign_file.bat" PATHS "${REPOSITORY_ROOT}/build/win")
			cmake_path (NATIVE_PATH sign_file_script NORMALIZE sign_file_script_native)	
			
			add_custom_command (OUTPUT ${sign_target_name}_run_always
				COMMENT "Signing binaries"
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_PRE_COMMAND}"
				COMMAND "${CMD}" /c "${sign_script_native}" ${sign_args} "${artifacts_directory}/*.exe"
				COMMAND "${CMD}" /c "${sign_script_native}" ${sign_args} "${artifacts_directory}/*.dll"
				COMMAND "${CMD}" /c "${sign_script_native}" ${sign_args} "${artifacts_directory}/Plugins/*.dll"
				COMMAND "${CMD}" /c "${${vendor}_SIGNING_POST_COMMAND}"
				VERBATIM
			)
			
		endif ()
		
		add_custom_target ("${sign_target_name}" DEPENDS ${sign_target_name}_run_always)
		set_target_properties ("${sign_target_name}" PROPERTIES USE_FOLDERS ON FOLDER cmake EXCLUDE_FROM_DEFAULT_BUILD_DEBUG ON)		
	endif ()
	
	# Installation directories (if using CPack)
	if ("${VENDOR_PRODUCT_NAME}" STREQUAL "")
		set (VENDOR_PRODUCT_NAME "${CMAKE_PROJECT_NAME}")
	endif ()
	if (PROJECT_APPLICATION_RUNTIME_DIRECTORY)
		set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "${PROJECT_APPLICATION_RUNTIME_DIRECTORY}")
	else ()
		if (PROJECT_VENDOR_INSTALL_SUBDIR)
			set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "${PROJECT_VENDOR_INSTALL_SUBDIR}")
		else ()
			set (VENDOR_APPLICATION_RUNTIME_DIRECTORY "")
		endif ()
	endif ()
	if (VENDOR_APPLICATION_RUNTIME_DIRECTORY AND NOT "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}" STREQUAL ".")
		set (VENDOR_PLUGINS_RUNTIME_DIRECTORY "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/Plugins")
		set (VENDOR_LIBRARY_DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
		set (VENDOR_BINARY_DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
	else ()
		set (VENDOR_PLUGINS_RUNTIME_DIRECTORY "Plugins")
		set (VENDOR_LIBRARY_DESTINATION "${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
		set (VENDOR_BINARY_DESTINATION "${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
	endif ()
	
	set (VENDOR_STATIC_LIBRARY_DESTINATION "lib/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
	set (VENDOR_IMPORT_LIBRARY_DESTINATION "lib/${PROJECT_ARCHITECTURE_SUBDIRECTORY}")
	set (VENDOR_PUBLIC_HEADERS_DESTINATION "include")
	set (VENDOR_CMAKE_FILES_DESTINATION "cmake")
	if (PROJECT_APPSUPPORT_DESTINATION)
		set (VENDOR_APPSUPPORT_DESTINATION "${PROJECT_APPSUPPORT_DESTINATION}")
	else ()
		set (VENDOR_APPSUPPORT_DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
	endif ()
endmacro ()

# Disable Arm64X build for given DLL target.
# @group win
# @param {STRING} target  DLL target name.
macro (ccl_disable_arm64x target)
	set_target_properties (${target} PROPERTIES ARM64X_HANDLED ON)
endmacro ()

# Process target dependencies.
# @group win
# @param {STRING} target  Name of the target to process dependencies for.
# @param {STRING} ITEMS  [variadic] List of dependency target names.
macro (ccl_process_dependencies target)
	cmake_parse_arguments (dependency_params "" "" "ITEMS" ${ARGN})

	set (arm64ReproDir "${CMAKE_BINARY_DIR}/repros")
	string (REPLACE "windows-arm64x" "windows-arm64" arm64ReproDir "${arm64ReproDir}")

	get_target_property (dependencies ${target} LINK_LIBRARIES)
	list (APPEND dependencies ${dependency_params_ITEMS})

	foreach (item IN ITEMS ${dependencies})
		if (NOT TARGET ${item})
			continue ()
		endif ()

		get_target_property (target_type ${item} TYPE)
		if (NOT ${target_type} STREQUAL "SHARED_LIBRARY" AND NOT ${target_type} STREQUAL "MODULE_LIBRARY")
			continue ()
		endif ()

		get_target_property (target_imported ${item} IMPORTED)
		if (${target_imported})
			continue ()
		endif ()

		get_target_property (arm64x_handled ${item} ARM64X_HANDLED)
		if (${arm64x_handled})
			continue ()
		endif ()

		set (arm64ReproFile "${arm64ReproDir}/${item}.rsp")

		if (VENDOR_BUILD_AS_ARM64X)
			if (NOT EXISTS "${arm64ReproFile}")
				message (FATAL_ERROR "Arm64 link repro file not found at ${arm64ReproFile}.  Please configure Arm64 project first.")
			endif ()

			get_property (configure_depends DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS)
			if ("${arm64ReproFile}" IN_LIST configure_depends)
				continue ()
			endif ()

			file (STRINGS "${arm64ReproFile}" arm64Objs REGEX obj\"$)
			file (STRINGS "${arm64ReproFile}" arm64Libs REGEX lib\"$)
			file (STRINGS "${arm64ReproFile}" arm64Def REGEX def\"$)

			string (REPLACE "\"" ";" arm64Objs "${arm64Objs}")
			string (REPLACE "\"" ";" arm64Libs "${arm64Libs}")
			string (REPLACE "\"" ";" arm64Def "${arm64Def}")
			string (REPLACE "/def:" "/defArm64Native:" arm64Def "${arm64Def}")

			target_sources (${item} PRIVATE ${arm64Objs})
			target_link_options (${item} PRIVATE /machine:arm64x "${arm64Def}" "${arm64Libs}")

			set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${arm64ReproFile}")
		elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
			make_directory (${arm64ReproDir})
			file (TOUCH ${arm64ReproFile})

			target_link_options (${item} PRIVATE "/LINKREPROFULLPATHRSP:${arm64ReproFile}")
		endif ()
		
		set_target_properties (${item} PROPERTIES ARM64X_HANDLED ON)
	endforeach ()
endmacro ()

# Sign files.
# @group win
# @param {STRING} target  Name of a new siging target.
# @param {STRING} vendor Vendor name.
# @param {FILEPATH} script  Path to a file to sign, might contain wildcards.
macro (ccl_sign_files target vendor file)
	get_filename_component (file_directory "${file}" DIRECTORY)
	get_filename_component (file_name "${file}" NAME)
	cmake_path (NATIVE_PATH file_directory file_directory_native)
	set (file_path_native "${file_directory_native}\\${file_name}")

	get_filename_component (filename "${file}" NAME)
	
	ccl_get_sign_args (sign_args "${vendor}")
		
	if(VENDOR_ENABLE_CODESIGNING)
		find_program (CMD cmd REQUIRED NAMES cmd)
		find_file (sign_file_script REQUIRED NAMES "sign_file.bat" PATHS "${REPOSITORY_ROOT}/build/win" "${CCL_REPOSITORY_ROOT}/build/win")
		cmake_path (NATIVE_PATH sign_file_script NORMALIZE sign_file_script_native)

		add_custom_command (OUTPUT sign_${target}
			COMMENT "Signing ${filename}"
			COMMAND "${CMD}" /c "${${vendor}_SIGNING_PRE_COMMAND}"
			COMMAND "${CMD}" /c "${sign_file_script_native}" ${sign_args} "${file_path_native}"
			COMMAND "${CMD}" /c "${${vendor}_SIGNING_POST_COMMAND}"
			VERBATIM
		)
	else ()
		add_custom_command (OUTPUT sign_${target}
			COMMENT "Signing ${filename}"
			COMMAND ${CMAKE_COMMAND} -E echo "Not signing ${file_name}. VENDOR_ENABLE_CODESIGNING is set to ${VENDOR_ENABLE_CODESIGNING}."
			VERBATIM
		)
	endif ()
	
	add_custom_target (${target} DEPENDS sign_${target})
	set_target_properties (${target} PROPERTIES USE_FOLDERS ON FOLDER cmake EXCLUDE_FROM_DEFAULT_BUILD_DEBUG ON)		
endmacro ()

# Add a package target using an NSIS installer script.
# Creates a target names ${target}_installer.
# @group win
# @param {STRING} target  Name of the target to add a package target for.
# @param {FILEPATH} script  NSIS script file path.
# @param args  [variadic] Additional arguments are passed to NSIS.
macro (ccl_nsis_package target script)
	if (NOT VENDOR_NSIS_INCLUDE_DIR)
		message (FATAL_ERROR "VENDOR_NSIS_INCLUDE_DIR not set. Make sure to set PROJECT_VENDOR in your CMakeLists.txt")
	endif ()

	find_program (NSIS NAMES makensis PATHS "C:/Program Files (x86)/NSIS")
	if (NOT NSIS)
		message (NOTICE "Skipping ${target}_installer. Missing dependency: NSIS")
	endif ()
	
	if (NSIS AND NOT TARGET ${target}_installer)
		set (nsis_arch "")
		if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "x86_64")
			set (nsis_arch "/DX64")
		elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
			set (nsis_arch "/DARM64")
		elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64ec" AND NOT VENDOR_BUILD_AS_ARM64X)
			set (nsis_arch "/DARM64EC")
		elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64ec")
			set (nsis_arch "/DARM64X")
		endif ()
		
		set (nsis_definitions "")
		foreach (definition ${CCL_NSIS_DEFINITIONS})
			list (APPEND nsis_definitions "/D${definition}")
		endforeach ()

		if (REPOSITORY_ROOT)
			list (APPEND nsis_definitions "/DBASEDIR=${REPOSITORY_ROOT}")
		endif ()
		
		if (NOT EXISTS "${REPOSITORY_ROOT}/buildnumber.h")
			list (APPEND nsis_definitions "/DBUILD_REVISION=0")
		endif ()

		if (CCL_REPOSITORY_ROOT)
			list (APPEND nsis_definitions "/DCCL_BASEDIR=${CCL_REPOSITORY_ROOT}")
		elseif (CCL_DIR)
			list (APPEND nsis_definitions "/DCCL_BASEDIR=${CCL_DIR}")
		endif ()

		if (CCL_IDENTITIES_DIR)
			list (APPEND nsis_definitions "/DCCL_IDENTITIES_DIR=${CCL_IDENTITIES_DIR}")
		endif ()

		if (NOT VENDOR_BUILD_AS_ARM64X)
			list (APPEND nsis_definitions "/DBUILDDIR=${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/$<CONFIG>")
		else ()
			list (APPEND nsis_definitions "/DBUILDDIR=${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/arm64/$<CONFIG>")
			list (APPEND nsis_definitions "/DBUILDDIR_MULTIARCH=${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${VENDOR_PLATFORM_SUBDIR}/$<CONFIG>")
		endif ()

		add_custom_command (OUTPUT ${target}_nsis_run_always
			COMMENT "Calling NSIS"
			COMMAND "${NSIS}" ${nsis_definitions} "/DVENDOR_INCLUDE_DIR=${VENDOR_NSIS_INCLUDE_DIR}" ${nsis_arch} ${ARGN} "${script}"
			VERBATIM
		)
		# Only add the dependency when codesigning is disabled. Otherwise we might overwrite previously signed binaries.
		if (NOT VENDOR_ENABLE_CODESIGNING)
			add_custom_command (OUTPUT ${target}_nsis_run_always APPEND DEPENDS ${target})
		endif ()
		
		get_filename_component (script_directory "${script}" DIRECTORY)
		ccl_sign_files (${target}_installer "${PROJECT_VENDOR}" "${script_directory}/*.exe")
		target_sources (${target}_installer PRIVATE "${script}")

		add_custom_command (OUTPUT sign_${target}_installer APPEND DEPENDS ${target}_nsis_run_always)
		
		find_program (CMD REQUIRED NAMES cmd)
		cmake_path (NATIVE_PATH CMD NORMALIZE cmd_native)
		cmake_path (NATIVE_PATH script_directory NORMALIZE script_directory_native)
		string (REPLACE "\\" "\\\\" cmd_native "${cmd_native}")
		string (REPLACE "\\" "\\\\" script_directory_native "${script_directory_native}")
		ccl_set_debug_command (${target}_installer 
			DEBUG_EXECUTABLE "${cmd_native}"
			DEBUG_ARGUMENTS "/c for %B in (\"${script_directory_native}\\\\*.exe\") do call \"%B\""
		)
	endif ()
endmacro ()
