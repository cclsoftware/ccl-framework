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
# Filename    : coremacros.android.cmake
# Description : Android CMake Macros
#
#************************************************************************************************

if (NOT CCL_FRAMEWORK_ROOT)
	set (CCL_FRAMEWORK_ROOT "${CCL_REPOSITORY_ROOT}")
endif ()

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "android_x64" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(i386)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "android_x86" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "android_arm64" CACHE STRING "")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(armv7)")
	set (EXTENSIONS_BINARY_SUBDIRECTORY "android_arm" CACHE STRING "")
endif ()

# Generate a fallback code signing key.
# @group android
# @param {STRING} vendor Vendor name.
macro (ccl_generate_signing_key vendor)
	find_program (keytool NAMES keytool PATHS ENV JAVA_HOME PATH_SUFFIXES bin)

	set (signing_dir "${CMAKE_BINARY_DIR}/vendor/${vendor}")

	if (NOT EXISTS "${signing_dir}/vendor.jks")
		message (STATUS "Generating fallback code signing key for vendor: ${vendor}")

		execute_process (COMMAND ${keytool} -genkey -keyalg RSA -keysize 2048 -dname "CN=${VENDOR_NAME}" -keystore "${signing_dir}/vendor.jks" -storetype pkcs12 -alias "${vendor}" -storepass password -keypass password)
	else ()
		message (STATUS "Using fallback code signing key for vendor: ${vendor}")
	endif ()

	set (SIGNING_STORE_ANDROID "${signing_dir}/vendor.jks")
	set (SIGNING_ALIAS_ANDROID "${vendor}")
	set (SIGNING_STORE_PASSWORD_ANDROID "password")
	set (SIGNING_ALIAS_PASSWORD_ANDROID "password")
endmacro ()

# Generate signing.properties file.
# @group android
macro (ccl_generate_signing_properties)
	set (properties_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle/properties")

	file (MAKE_DIRECTORY "${properties_dir}")

	file (WRITE "${properties_dir}/signing.properties"
		"key.store=${SIGNING_STORE_ANDROID}\n"
		"key.alias=${SIGNING_ALIAS_ANDROID}\n"
		"key.store.password=${SIGNING_STORE_PASSWORD_ANDROID}\n"
		"key.alias.password=${SIGNING_ALIAS_PASSWORD_ANDROID}\n")
endmacro ()

# Configure the project vendor.
# @group android
# @param {STRING} vendor Vendor name.
macro (ccl_configure_project_vendor vendor)
	# Code Signing
	if ("${CCL_FRAMEWORK_ROOT}" STREQUAL "${CCL_REPOSITORY_ROOT}")
		if (NOT SIGNING_STORE_ANDROID)
			ccl_generate_signing_key ("${vendor}")
		endif ()

		ccl_generate_signing_properties ()
	elseif (CCL_SIGNING_PROPERTIES AND EXISTS ${CCL_SIGNING_PROPERTIES})
		file (GENERATE OUTPUT "${signing_properties}" INPUT "${CCL_SIGNING_PROPERTIES}")
	elseif (EXISTS "${CMAKE_CURRENT_LIST_DIR}/signing.properties")
		file (GENERATE OUTPUT "${signing_properties}" INPUT "${CMAKE_CURRENT_LIST_DIR}/signing.properties")
	else ()
		find_program (keytool NAMES keytool PATHS ENV JAVA_HOME PATH_SUFFIXES bin)

		set (sign_warning "To use a custom signing key, please provide a signing.properties file with the following structure:\n")
		string (APPEND sign_warning
			"    key.store=<path to key store file>\n"
			"    key.alias=<alias of key to use>\n"
			"    key.store.password=<key store password>\n"
			"    key.alias.password=<key password>\n"
			"Place the signing.properties file in the same directory as the top level CMakeLists.txt or provide the path to it in the CCL_SIGNING_PROPERTIES variable. "
			"The CCL_SIGNING_PROPERTIES variable takes precedence if set."
		)

		if (EXISTS "${keytool}")
			message (WARNING
				"No code signing properties found; a fallback signing key will be generated. "
				"${sign_warning}")

			ccl_generate_signing_key ("${vendor}")
			ccl_generate_signing_properties ()
		else ()
			message (WARNING
				"No code signing properties and no Java keytool found; cannot generate signing information. "
				"${sign_warning}")
		endif ()
	endif ()
	
	# Installation directories (if using CPack)
	if ("${VENDOR_PRODUCT_NAME}" STREQUAL "")
		set (VENDOR_PRODUCT_NAME "${CMAKE_PROJECT_NAME}")
	endif ()
	set (VENDOR_LIBRARY_DESTINATION "lib/${PROJECT_ARCHITECTURE_SUBDIRECTORY}" CACHE STRING "Default system directory for shared libraries" FORCE)
	set (VENDOR_PUBLIC_HEADERS_DESTINATION "include" CACHE STRING "Default system directory for public headers" FORCE)
	set (VENDOR_CMAKE_FILES_DESTINATION "cmake" CACHE STRING "Default system directory for cmake files" FORCE)
endmacro ()

# Generate version.properties file.
# @group android
# @param {STRING} target Target name.
macro (ccl_generate_version_properties target)
	set (properties_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle/properties")

	file (MAKE_DIRECTORY "${properties_dir}")

	file (WRITE "${properties_dir}/version.properties"
		"versionCode=${${target}_VERSION_BUILD}\n"
		"versionName=${${target}_VERSION_MAJOR}.${${target}_VERSION_MINOR}.${${target}_VERSION_REVISION}.${${target}_VERSION_BUILD}\n")
endmacro ()

# Set a prefix to use for Gradle includes for the specified target.
# @group android
# @param {STRING} target  Target name.
# @param {STRING} prefix  Prefix to use for target.
macro (ccl_set_target_gradle_prefix target prefix)
	get_target_property (${target}_GRADLE_DIR ${target} GRADLE_DIR)

	if (${target}_GRADLE_DIR)
		message (FATAL_ERROR "When using a Gradle prefix, it must be set before any content (assets, resources, dependencies etc.) is added.")
	endif ()

	set_target_properties (${target} PROPERTIES GRADLE_PREFIX ${prefix})
endmacro ()

# Get the directory to store generated Gradle files for target.
# @group android
# @param {STRING} output  Output variable to store result in.
# @param {STRING} target  Name of the target to operate on.
macro (ccl_get_target_gradle_dir output target)
	get_target_property (${output} ${target} GRADLE_DIR)

	if (NOT ${output})
		get_target_property (${target}_GRADLE_PREFIX ${target} GRADLE_PREFIX)

		if (${target}_GRADLE_PREFIX)
			set (${output} "${CMAKE_CURRENT_BINARY_DIR}/gradle/${${target}_GRADLE_PREFIX}")
		else ()
			set (${output} "${CMAKE_CURRENT_BINARY_DIR}/gradle/${target}")
		endif ()

		set_target_properties (${target} PROPERTIES GRADLE_DIR "${${output}}")
	endif ()
endmacro ()

# Add an external Gradle project to include in the project
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} name  Name of the Gradle project to include.
# @param {STRING} path  Path to the Gradle project.
# @param {BOOL} TRANSITIVE  Pass the dependency on to depending projects.
macro (ccl_add_gradle_project target name path)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_settings_file "${gradle_base_dir}/settings.gradle")
	set (gradle_dependencies_file "${gradle_base_dir}/dependencies.gradle")

	ccl_create_file_once (${gradle_settings_file})
	ccl_create_file_once (${gradle_dependencies_file})

	file (APPEND "${gradle_settings_file}"
		"include ':${name}'\n"
		"project(':${name}').projectDir = new File(\"${path}\")\n")

	file (APPEND "${gradle_dependencies_file}"
		"dependencies { implementation project(':${name}') }\n")

	if (params_TRANSITIVE)
		set (gradle_base_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
		set (gradle_settings_file "${gradle_base_dir}/settings.gradle")
		set (gradle_dependencies_file "${gradle_base_dir}/dependencies.gradle")

		ccl_create_file_once (${gradle_dependencies_file})

		file (APPEND "${gradle_settings_file}"
			"include ':${name}'\n"
			"project(':${name}').projectDir = new File(\"${path}\")\n")

		file (APPEND "${gradle_dependencies_file}"
			"dependencies { implementation project(':${name}') }\n")
	endif ()
endmacro ()

# Set a property on an external Gradle project
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} project Name of the external Gradle project.
# @param {STRING} name    Name of the property to add.
# @param {STRING} value   Initial value of the property.
macro (ccl_add_gradle_project_property target project name value)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_properties_file "${gradle_base_dir}/properties.gradle")

	ccl_create_file_once (${gradle_properties_file})

	file (APPEND "${gradle_properties_file}" "project(':${project}').ext.${name} = \"${value}\"\n")
endmacro ()

# Add an external dependency to the project
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {LIST} packages  Package identifiers of the dependencies to add.
# @param {BOOL} COMPILE_ONLY  Don't add the dependency to the resulting package.
# @param {BOOL} TRANSITIVE  Pass the dependency on to depending projects.
macro (ccl_add_gradle_dependency target packages)
	cmake_parse_arguments (params "COMPILE_ONLY;TRANSITIVE" "" "" ${ARGN})

	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_dependencies_file "${gradle_base_dir}/dependencies.gradle")

	ccl_create_file_once (${gradle_dependencies_file})

	if (params_COMPILE_ONLY)
		set (dependency_type "compileOnly")
	else ()
		set (dependency_type "implementation")
	endif ()

	foreach (package IN ITEMS ${packages})
		if ("${package}" MATCHES "\.aar$")
			string(REPLACE "%" "\$" gradle_package "${package}")

			file (APPEND "${gradle_dependencies_file}"
				"\tdependencies { ${dependency_type} files(\"${gradle_package}\") }\n")

			# Add compile-only AAR project dependencies to top level project libraries
			if (NOT TARGET ${target}.aar OR params_COMPILE_ONLY AND NOT "${package}" IN_LIST PROJECT_AAR_LIBRARIES)
				list (APPEND PROJECT_AAR_LIBRARIES "${package}")
			endif ()
		else ()
			file (APPEND "${gradle_dependencies_file}"
				"dependencies { ${dependency_type} \"${package}\" }\n")
		endif ()
	endforeach ()

	if (params_TRANSITIVE)
		set (gradle_base_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
		set (gradle_dependencies_file "${gradle_base_dir}/dependencies.gradle")

		ccl_create_file_once (${gradle_dependencies_file})

		foreach (package IN ITEMS ${packages})
			file (APPEND "${gradle_dependencies_file}"
				"dependencies { implementation \"${package}\" }\n")
		endforeach ()
	endif ()
endmacro ()

# Add a property that will be made available in .gradle scripts
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} name  Name of the property to add.
# @param {STRING} value  Initial value of the property.
macro (ccl_add_gradle_property target name value)
	set (gradle_base_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
	set (gradle_properties_file "${gradle_base_dir}/properties.gradle")

	ccl_create_file_once (${gradle_properties_file})

	file (APPEND "${gradle_properties_file}"
		"if (!ext.has('${name}')) ext.${name} = \"${value}\"\n")
endmacro ()

# Add a .gradle file to include in the project build
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} path  Path to the .gradle file to include.
macro (ccl_add_gradle_include target path)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_includes_file "${gradle_base_dir}/includes.gradle")

	ccl_create_file_once (${gradle_includes_file})

	file (APPEND "${gradle_includes_file}" "apply from: \"${path}\"\n")
endmacro ()

# Add a .settings.gradle file to include in the project settings
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} path  Path to the .settings.gradle file to include.
macro (ccl_add_gradle_settings target path)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_settings_file "${gradle_base_dir}/settings.gradle")

	ccl_create_file_once (${gradle_settings_file})

	file (APPEND "${gradle_settings_file}" "apply from: \"${path}\"\n")
endmacro ()

# Add a Java source dir to include in the project build
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} path  Path to the Java source dir to add.
macro (ccl_add_java_sourcedir target path)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_sources_file "${gradle_base_dir}/sources.gradle")

	ccl_create_file_once (${gradle_sources_file})

	file (APPEND "${gradle_sources_file}" "android.sourceSets.main.java.srcDirs += \"${path}\"\n")
endmacro ()

# Add a ProGuard file to include in the project build
# @group android
# @param {STRING} target  Name of the target to operate on.
# @param {STRING} path  Path to the ProGuard file to include.
macro (ccl_add_proguard_file target path)
	ccl_get_target_gradle_dir (gradle_base_dir ${target})
	set (gradle_proguard_file "${gradle_base_dir}/proguard.gradle")

	ccl_create_file_once (${gradle_proguard_file})

	file (APPEND "${gradle_proguard_file}" "android.buildTypes.release.proguardFiles += \"${path}\"\n")

	# Also add ProGuard file to the main project
	set (gradle_base_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
	set (gradle_proguard_file "${gradle_base_dir}/proguard.gradle")

	ccl_create_file_once (${gradle_proguard_file})

	file (APPEND "${gradle_proguard_file}" "android.buildTypes.release.proguardFiles += \"${path}\"\n")
endmacro ()

# Generate a .gradle file referencing project assets
# @group android
# @param {STRING} target  Name of the target to add asset files to.
# @param {PATH} PATH  [optional] Destination subdirectory.
# @param {FILEPATH} FILES  [variadic] Asset files.
# @param {PATH} DIRECTORY  [variadic] Asset directories.
macro (ccl_process_assets target)
	cmake_parse_arguments (asset_params "" "PATH" "DIRECTORY;FILES" ${ARGN})

	if (asset_params_FILES)
		cmake_path (CONVERT "${asset_params_FILES}" TO_CMAKE_PATH_LIST assets NORMALIZE)
	elseif (asset_params_DIRECTORY)
		cmake_path (CONVERT "${asset_params_DIRECTORY}" TO_CMAKE_PATH_LIST assets NORMALIZE)
	endif ()

	get_target_property (${target}_ASSETS_PROCESSED ${target} GRADLE_ASSETS_PROCESSED)

	if (NOT ${target}_ASSETS_PROCESSED OR gradle_assets_${target}_count)
		ccl_get_target_gradle_dir (gradle_base_dir ${target})
		set (gradle_assets_file "${gradle_base_dir}/assets.gradle")

		ccl_create_file_once (${gradle_assets_file})

		file (APPEND "${gradle_assets_file}"
			"if (!project.ext.buildConfiguration.equals(\"clean\") && !tasks.findByName('copyAssets_${target}${gradle_assets_${target}_count}')) {\n"
			"	task copyAssets_${target}${gradle_assets_${target}_count}(type: Copy) {\n")

		list (REMOVE_DUPLICATES assets)
		foreach (asset IN ITEMS ${assets})
			cmake_path (GET asset FILENAME asset_file)

			unset (prefix)
			if (asset_params_PATH)
				if (asset_params_DIRECTORY)
					set (prefix "${asset_params_PATH}/${asset_file}")
				else ()
					set (prefix "${asset_params_PATH}")
				endif ()
			elseif (asset_params_DIRECTORY)
				set (prefix "${asset_file}")
			endif ()

			file (APPEND "${gradle_assets_file}" "\t\tfrom(\"${asset}\")")
			if (prefix)
				file (APPEND "${gradle_assets_file}" " { into \"${prefix}\" }")
			endif ()
			file (APPEND "${gradle_assets_file}" "\n")
		endforeach ()

		file (APPEND "${gradle_assets_file}"
			"\t\tinto \"build/assets\"\n"
			"\t}\n\n"
			"\tif (!project.ext.buildConfiguration.equals(\"sync\"))\n"
			"\t\tcopyAssets_${target}${gradle_assets_${target}_count}.dependsOn(\":native:buildCMake\${project.ext.cmakeConfiguration}[\${project.ext.buildABI}]\")\n\n"
			"\tpreBuild.dependsOn(copyAssets_${target}${gradle_assets_${target}_count})\n"
			"}\n")

		if (NOT gradle_assets_${target}_count)
			set (gradle_assets_${target}_count 1)
		else ()
			math (EXPR gradle_assets_${target}_count "${gradle_assets_${target}_count} + 1")
		endif ()

		set_target_properties (${target} PROPERTIES GRADLE_ASSETS_PROCESSED ON)
	endif ()
endmacro ()

# Generate a .gradle file referencing project resources
# @group android
# @param {STRING} target  Name of a target to add resources to.
# @param {STRING} path  Subdirectory to place the resources in.
# @param args  Resource files.
macro (ccl_process_resources target config path)
	set (resources ${ARGN})
	get_target_property (${target}_RESOURCES_PROCESSED ${target} GRADLE_RESOURCES_PROCESSED)

	if (resources AND (NOT ${target}_RESOURCES_PROCESSED OR gradle_resources_${target}_count))
		cmake_path (CONVERT "${resources}" TO_CMAKE_PATH_LIST resources NORMALIZE)

		ccl_get_target_gradle_dir (gradle_base_dir ${target})
		set (gradle_resources_file "${gradle_base_dir}/resources.gradle")

		ccl_create_file_once (${gradle_resources_file})

		file (APPEND "${gradle_resources_file}"
			"if (!project.ext.buildConfiguration.equals(\"clean\") && !tasks.findByName('copyResources_${target}${gradle_resources_${target}_count}')) {\n"
			"	task copyResources_${target}${gradle_resources_${target}_count}(type: Copy) {\n")

		list (REMOVE_DUPLICATES resources)
		foreach (resource IN ITEMS ${resources})
			cmake_path (GET resource FILENAME resource_file)

			set (prefix "${target}")
			if (NOT "${path}" STREQUAL "")
				set (prefix "${prefix}/${path}")
			elseif (IS_DIRECTORY "${resource}")
				set (prefix "${prefix}/${resource_file}")
			endif ()

			file (APPEND "${gradle_resources_file}"
				"\t\tfrom(\"${resource}\") { into \"${prefix}\" }\n")
		endforeach ()

		file (APPEND "${gradle_resources_file}"
			"\t\tinto \"build/assets/resources\"\n"
			"\t}\n\n"
			"\tif (!project.ext.buildConfiguration.equals(\"sync\"))\n"
			"\t\tcopyResources_${target}${gradle_resources_${target}_count}.dependsOn(\":native:buildCMake\${project.ext.cmakeConfiguration}[\${project.ext.buildABI}]\")\n\n"
			"\tpreBuild.dependsOn(copyResources_${target}${gradle_resources_${target}_count})\n"
			"}\n")

		if (NOT gradle_resources_${target}_count)
			set (gradle_resources_${target}_count 1)
		else ()
			math (EXPR gradle_resources_${target}_count "${gradle_resources_${target}_count} + 1")
		endif ()

		set_target_properties (${target} PROPERTIES GRADLE_RESOURCES_PROCESSED ON)
	endif ()
endmacro ()

# Process target dependencies.
# @group android
# @param {STRING} target  Name of the target to process dependencies for.
# @param {STRING} ITEMS  [variadic] List of dependency target names.
macro (ccl_process_dependencies target)
	cmake_parse_arguments (dependency_params "" "" "ITEMS" ${ARGN})

	foreach (item IN ITEMS ${dependency_params_ITEMS})
		if (NOT TARGET ${item})
			continue ()
		endif ()

		get_target_property (target_type ${item} TYPE)
		if (${target_type} STREQUAL "MODULE_LIBRARY")
			# Generate .plugin files and add them to project resources
			ccl_get_build_output_directory (build_output)
			get_target_property (library_output ${item} LIBRARY_OUTPUT_DIRECTORY)

			if (NOT "${build_output}" STREQUAL "${library_output}")
				get_target_property (imported_location ${item} IMPORTED_LOCATION)

				if (imported_location)
					get_filename_component (module_folder "${imported_location}" DIRECTORY)
					get_filename_component (module_folder "${module_folder}" NAME)
				else ()
					cmake_path (RELATIVE_PATH library_output BASE_DIRECTORY ${build_output} OUTPUT_VARIABLE module_folder)
				endif ()

				set (modules_dir "${CMAKE_CURRENT_BINARY_DIR}/modules")
				set (plugin_file "${modules_dir}/${item}.plugin")

				file (WRITE "${plugin_file}" "Lib=lib${item}.so\n")
				ccl_add_assets (${target} FILES "${plugin_file}" PATH "${module_folder}")
			endif ()

			# Include sub-project .gradle files in parent project
			set (gradle_project_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
			get_target_property (gradle_item_dir ${item} GRADLE_DIR)

			if (gradle_item_dir AND NOT "${gradle_project_dir}" STREQUAL "${gradle_item_dir}")
				get_property (processed_dependency_dirs GLOBAL PROPERTY processed_dependency_dirs)

				if (NOT "${gradle_project_dir}:${gradle_item_dir}" IN_LIST processed_dependency_dirs)
					set (files proguard)

					if (NOT TARGET ${item}.aar)
						list (APPEND files properties;assets;resources;settings;dependencies;includes;sources)
					endif ()

					foreach (file ${files})
						set (gradle_project_file "${gradle_project_dir}/${file}.gradle")
						set (gradle_item_file "${gradle_item_dir}/${file}.gradle")

						if (EXISTS "${gradle_item_file}")
							ccl_create_file_once (${gradle_project_file})

							file (APPEND "${gradle_project_file}"
								"apply from: \"${gradle_item_file}\"\n")
						endif ()
					endforeach ()

					list (APPEND processed_dependency_dirs "${gradle_project_dir}:${gradle_item_dir}")
					set_property (GLOBAL PROPERTY processed_dependency_dirs "${processed_dependency_dirs}")
				endif ()
			endif ()
		endif ()
	endforeach ()
endmacro ()

# Setup platform-specific options to pass to CMake for external projects
# @group android
# @param {STRING} target  Name of target to setup platform options for.
macro (ccl_setup_external_project_platform_options target)
	cmake_path (SET CMAKE_TOOLCHAIN_FILE NORMALIZE "${CMAKE_TOOLCHAIN_FILE}")

	list (APPEND ${target}_platform_options
		-DOPERATING_SYSTEM=Android
		-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
		-DCMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK}
		-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
		-DANDROID_PLATFORM=${ANDROID_PLATFORM}
		-DANDROID_API_LEVEL=${ANDROID_API_LEVEL}
		-DANDROID_STL=${ANDROID_STL}
		-DANDROID_ABI=${ANDROID_ABI}
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY}
	)
endmacro ()

# Add libraries to preload for calling JNI_OnLoad
# @group android
# @param {STRING} target  Name of target to add preload libraries to.
# @param {STRING} LIBRARIES  [variadic] List of library names.
macro (ccl_add_preload_libraries target)
	cmake_parse_arguments (params "" "" "LIBRARIES" ${ARGN})

	get_target_property (${target}_PRELOAD_LIBRARIES ${target} PRELOAD_LIBRARIES)

	if (NOT ${target}_PRELOAD_LIBRARIES)
		set (${target}_PRELOAD_LIBRARIES)
	endif ()

	list (APPEND ${target}_PRELOAD_LIBRARIES ${params_LIBRARIES})

	set_target_properties (${target} PROPERTIES PRELOAD_LIBRARIES "${${target}_PRELOAD_LIBRARIES}")
endmacro ()

# Get the jnilibs directory for native builds
# @group android
# @param {STRING} output  Output variable name.
macro (ccl_get_jnilibs_directory output)
	if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
		set (subfolder "arm64-v8a")
	elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "armv7")
		set (subfolder "armeabi-v7a")
	elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "i386")
		set (subfolder "x86")
	elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "x86_64")
		set (subfolder "x86_64")
	endif ()

	if ("${CMAKE_BINARY_DIR}" MATCHES ".*/\.cxx/.*")
		set (${output} "${CMAKE_CURRENT_BINARY_DIR}/../../../../jniLibs/${subfolder}")
	else ()
		set (${output} "${CMAKE_CURRENT_BINARY_DIR}/native/jniLibs/${subfolder}")
	endif ()
endmacro ()

# Copy imported libs of target to jnilibs directory
# @group android
# @param {STRING} target  Name of target to copy imported lib from.
macro (ccl_copy_imported_jnilib target)
	get_target_property (imported_location ${target} IMPORTED_LOCATION)
	ccl_get_jnilibs_directory (jnilibs_dir)

	if ("${CMAKE_BINARY_DIR}" MATCHES ".*/\.cxx/.*")
		# Exclude Linux here as the Android Gradle Plugin automatically
		# pulls in imported CMake libs on Linux
		if (NOT "${VENDOR_HOST_PLATFORM}" STREQUAL "linux")
			file (COPY "${imported_location}" DESTINATION "${jnilibs_dir}")
		endif ()
	endif ()
endmacro ()

# Add an existing APK deployment project.
# @group android
# @param {STRING} project  Parent project target name.
# @param {STRING} appid  Application ID, e.g. dev.ccl.ccldemo.
# @param {STRING} CMAKE_ARGS  [variadic] List of arguments to pass to native CMake subprojects.
macro (ccl_add_deployment_project project appid)
	cmake_parse_arguments (params "" "" "CMAKE_ARGS" ${ARGN})

	# Add sub-project AAR dependencies to deployment project
	ccl_add_gradle_dependency (${project} "${PROJECT_AAR_LIBRARIES}")

	# Move parent-project .gradle files to top-level
	set (gradle_project_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle")
	get_target_property (gradle_target_dir ${project} GRADLE_DIR)

	if (gradle_target_dir AND NOT "${gradle_project_dir}" STREQUAL "${gradle_target_dir}")
		set (files proguard;properties;assets;resources;settings;dependencies;includes;sources)

		foreach (file ${files})
			set (gradle_project_file "${gradle_project_dir}/${file}.gradle")
			set (gradle_target_file "${gradle_target_dir}/${file}.gradle")

			if (EXISTS "${gradle_target_file}")
				ccl_create_file_once (${gradle_project_file})

				file (READ "${gradle_target_file}" target_file_content)
				file (APPEND "${gradle_project_file}" "${target_file_content}")
			endif ()
		endforeach ()

		file (REMOVE_RECURSE "${gradle_target_dir}")
	endif ()

	set_target_properties (${project} PROPERTIES GRADLE_DIR "${gradle_project_dir}")

	# Create Gradle project
	set (gradle_project_dir "${CMAKE_CURRENT_BINARY_DIR}")

	set (gradle_build_file "${gradle_project_dir}/build.gradle")
	set (gradle_properties_file "${gradle_project_dir}/gradle.properties")
	set (gradle_local_properties_file "${gradle_project_dir}/local.properties")
	set (gradle_settings_file "${gradle_project_dir}/settings.gradle")
	set (gradle_toolchain_file "${gradle_project_dir}/gradle/toolchain.gradle")
	set (gradle_wrapper_properties_file "${gradle_project_dir}/gradle/wrapper/gradle-wrapper.properties")
	set (gradle_libs_versions_file "${gradle_project_dir}/gradle/libs.versions.toml")
	set (gradle_app_build_file "${gradle_project_dir}/${project}/build.gradle")
	set (gradle_native_build_file "${gradle_project_dir}/native/build.gradle")

	set (java_loader_file "${gradle_project_dir}/${project}/src/main/MainActivity.java")

	ccl_create_file_once (${gradle_build_file})

	file (APPEND "${gradle_build_file}"
		"plugins {\n"
		"\talias(libs.plugins.android.application) apply false\n"
		"\talias(libs.plugins.android.library) apply false\n"
		"}\n")

	ccl_create_file_once (${gradle_properties_file})

	file (APPEND "${gradle_properties_file}"
		"org.gradle.jvmargs=-Xmx2048m -Dfile.encoding=UTF-8\n"
		"org.gradle.caching=true\n"
		"org.gradle.configuration-cache=true\n\n"
		"android.useAndroidX=true\n"
		"android.javaCompile.suppressSourceTargetDeprecationWarning=true\n")

	ccl_create_file_once (${gradle_local_properties_file})

	set (sdk_path "${CMAKE_ANDROID_NDK}/../..")
	cmake_path (NORMAL_PATH sdk_path)

	file (APPEND "${gradle_local_properties_file}"
		"sdk.dir=${sdk_path}\n")

	ccl_create_file_once (${gradle_settings_file})

	file (APPEND "${gradle_settings_file}"
		"pluginManagement {\n"
		"\trepositories {\n"
		"\t\tgoogle {\n"
		"\t\t\tcontent {\n"
		"\t\t\t\tincludeGroupByRegex(\"com\\\\.android.*\")\n"
		"\t\t\t\tincludeGroupByRegex(\"com\\\\.google.*\")\n"
		"\t\t\t\tincludeGroupByRegex(\"androidx.*\")\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t\tmavenCentral()\n"
		"\t\tgradlePluginPortal()\n"
		"\t}\n"
		"}\n"
		"dependencyResolutionManagement {\n"
		"\trepositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)\n"
		"\trepositories {\n"
		"\t\tgoogle()\n"
		"\t\tmavenCentral()\n"
		"\t}\n"
		"}\n\n"
		"if(file(\"${gradle_project_dir}/gradle/settings.gradle\").exists())\n"
		"\tapply from: \"${gradle_project_dir}/gradle/settings.gradle\"\n\n"
		"rootProject.name = \"${CMAKE_PROJECT_NAME}\"\n"
		"include ':${project}'\n\n"
		"include ':native'\n"
		"project(':native').projectDir = new File(\"${CMAKE_CURRENT_BINARY_DIR}/native\")\n")

	foreach (aar_item IN ITEMS ${PROJECT_AAR_PROJECTS})
		string (REGEX MATCH "^[^|]*" aar_project "${aar_item}")
		string (REGEX MATCH "[^|]*$" aar_project_file "${aar_item}")

		file (APPEND "${gradle_settings_file}"
			"\n"
			"include ':${aar_project_file}'\n"
			"project(':${aar_project_file}').projectDir = new File(\"${CMAKE_CURRENT_BINARY_DIR}/${aar_project}\")\n")

		file (APPEND "${gradle_project_dir}/gradle/dependencies.gradle"
			"dependencies { implementation project(':${aar_project_file}') }\n")
	endforeach ()

	ccl_create_file_once (${gradle_toolchain_file})

	file (APPEND "${gradle_toolchain_file}"
		"android {\n"
		"\tcompileSdkVersion = ${VENDOR_ANDROID_BUILD_PLATFORM_VERSION}\n"
		"\tbuildToolsVersion = '${VENDOR_ANDROID_BUILD_TOOLS_VERSION}'\n\n"
		"\tndkVersion = '${VENDOR_ANDROID_NDK_VERSION}'\n\n"
		"\tdefaultConfig {\n"
		"\t\tminSdkVersion = ${VENDOR_ANDROID_MINIMUM_PLATFORM_VERSION}\n"
		"\t\ttargetSdkVersion = ${VENDOR_ANDROID_TARGET_PLATFORM_VERSION}\n"
		"\t}\n"
		"}\n")

	ccl_create_file_once (${gradle_wrapper_properties_file})

	file (APPEND "${gradle_wrapper_properties_file}"
		"distributionBase=GRADLE_USER_HOME\n"
		"distributionPath=wrapper/dists\n"
		"distributionUrl=https\://services.gradle.org/distributions/gradle-${VENDOR_ANDROID_GRADLE_VERSION}-bin.zip\n"
		"zipStoreBase=GRADLE_USER_HOME\n"
		"zipStorePath=wrapper/dists\n")

	ccl_create_file_once (${gradle_libs_versions_file})

	file (APPEND "${gradle_libs_versions_file}"
		"[versions]\n"
		"agp = \"${VENDOR_ANDROID_GRADLE_PLUGIN_VERSION}\"\n"
		"core = \"${VENDOR_ANDROIDX_CORE_LIBRARY_VERSION}\"\n"
		"appcompat = \"${VENDOR_ANDROIDX_APPCOMPAT_LIBRARY_VERSION}\"\n"
		"constraintlayout = \"${VENDOR_ANDROIDX_CONSTRAINTLAYOUT_LIBRARY_VERSION}\"\n"
		"material = \"${VENDOR_ANDROID_MATERIAL_DESIGN_LIBRARY_VERSION}\"\n\n"
		"[libraries]\n"
		"core = { group = \"androidx.core\", name = \"core\", version.ref = \"core\" }\n"
		"appcompat = { group = \"androidx.appcompat\", name = \"appcompat\", version.ref = \"appcompat\" }\n"
		"constraintlayout = { group = \"androidx.constraintlayout\", name = \"constraintlayout\", version.ref = \"constraintlayout\" }\n"
		"material = { group = \"com.google.android.material\", name = \"material\", version.ref = \"material\" }\n\n"
		"[plugins]\n"
		"android-application = { id = \"com.android.application\", version.ref = \"agp\" }\n"
		"android-library = { id = \"com.android.library\", version.ref = \"agp\" }\n")

	ccl_create_file_once (${gradle_app_build_file})

	if (CCL_BUILD_SUPPORT_DIR)
		set (build_support_dir "${CCL_BUILD_SUPPORT_DIR}")
	else ()
		set (build_support_dir "${CCL_FRAMEWORK_ROOT}/build")
	endif ()

	file (APPEND "${gradle_app_build_file}"
		"plugins {\n"
		"\talias(libs.plugins.android.application)\n"
		"}\n\n"
		"apply from: \"${gradle_project_dir}/gradle/toolchain.gradle\"\n\n"
		"if(rootProject.file(\"${gradle_project_dir}/gradle/properties.gradle\").exists())\n"
		"\tapply from: \"${gradle_project_dir}/gradle/properties.gradle\"\n\n"
		"apply from: \"${build_support_dir}/android/gradle/common.gradle\"\n\n"
		"android {\n"
		"\tnamespace '${appid}'\n\n"
		"\tdefaultConfig {\n"
		"\t\tapplicationId \"${appid}\"\n"
		"\t}\n\n"
		"\tsourceSets {\n"
		"\t\tmain {\n"
		"\t\t\tmanifest.srcFile '${CMAKE_CURRENT_LIST_DIR}/../packaging/android/AndroidManifest.xml'\n"
		"\t\t\tjava.srcDirs = ['src']\n"
		"\t\t\tjniLibs.srcDirs = ['../native/jniLibs']\n"
		"\t\t\tres.srcDirs = ['${CMAKE_CURRENT_LIST_DIR}/../packaging/android/resource']\n"
		"\t\t\tassets.srcDirs = ['build/assets']\n"
		"\t\t}\n"
		"\t}\n"
		"}\n\n"
		"dependencies {\n"
		"\timplementation libs.core\n"
		"\timplementation libs.appcompat\n"
		"\timplementation libs.constraintlayout\n"
		"\timplementation libs.material\n\n"
		"\timplementation project(':native')\n"
		"}\n")

	set (files includes;assets;resources;sources;dependencies;proguard)

	foreach (file ${files})
		file (APPEND "${gradle_app_build_file}"
			"\n"
			"if(rootProject.file(\"${gradle_project_dir}/gradle/${file}.gradle\").exists())\n"
			"\tapply from: \"${gradle_project_dir}/gradle/${file}.gradle\"\n")
	endforeach ()

	ccl_create_file_once (${java_loader_file})

	file (APPEND "${java_loader_file}"
		"package ${appid};\n\n"
		"import dev.ccl.cclgui.FrameworkActivity;\n\n"
		"import android.os.Bundle;\n\n"
		"public class MainActivity extends FrameworkActivity\n"
		"{\n"
		"	static\n"
		"	{\n"
		"		FrameworkActivity.loadNativeLibraries (\"${project}\");\n")

	get_target_property (${project}_PRELOAD_LIBRARIES ${project} PRELOAD_LIBRARIES)

	if (${project}_PRELOAD_LIBRARIES)
		foreach (library IN ITEMS ${${project}_PRELOAD_LIBRARIES})
			file (APPEND "${java_loader_file}"
				"		FrameworkActivity.loadNativeLibraries (\"${library}\");\n")
		endforeach ()
	endif ()

	file (APPEND "${java_loader_file}"
		"	}\n\n"
		"	@Override\n"
		"	public void onCreate (Bundle savedInstanceState)\n"
		"	{\n"
		"		setAppName (getString (R.string.app_name));\n\n"
		"		super.onCreate (savedInstanceState);\n"
		"	}\n"
		"}\n")

	ccl_create_file_once (${gradle_native_build_file})

	file (APPEND "${gradle_native_build_file}"
		"plugins {\n"
		"\talias(libs.plugins.android.library)\n"
		"}\n\n"
		"apply from: \"${gradle_project_dir}/gradle/toolchain.gradle\"\n\n"
		"if(rootProject.file(\"${gradle_project_dir}/gradle/properties.gradle\").exists())\n"
		"\tapply from: \"${gradle_project_dir}/gradle/properties.gradle\"\n\n"
		"apply from: \"${build_support_dir}/android/gradle/common.gradle\"\n\n"
		"android {\n"
		"\tnamespace '${appid}.nativelibs'\n\n"
		"\tdefaultConfig {\n"
		"\t\texternalNativeBuild {\n"
		"\t\t\tcmake {\n"
		"\t\t\t\targuments '--preset=android', '-DENABLE_ASAN=${ENABLE_ASAN}', '-DENABLE_HWASAN=${ENABLE_HWASAN}'")

	if (CCL_SPIRV_CROSS_DIR)
		cmake_path (NORMAL_PATH CCL_SPIRV_CROSS_DIR)
		file (APPEND "${gradle_native_build_file}"
			", \"-DCCL_SPIRV_CROSS_DIR='${CCL_SPIRV_CROSS_DIR}'\"")
	endif ()

	foreach (arg ${params_CMAKE_ARGS})
		file (APPEND "${gradle_native_build_file}"
			", '${arg}'")
	endforeach ()

	file (APPEND "${gradle_native_build_file}"
		"\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t}\n\n"
		"\texternalNativeBuild {\n"
		"\t\tcmake {\n"
		"\t\t\tpath file('${CMAKE_PARENT_LIST_FILE}')\n"
		"\t\t\tversion '${VENDOR_ANDROID_NATIVE_CMAKE_VERSION}'\n"
		"\t\t}\n"
		"\t}\n\n"
		"\tsourceSets {\n"
		"\t\tmain {\n"
		"\t\t\tjniLibs.srcDirs = ['jniLibs']\n"
		"\t\t}\n"
		"\t}\n\n"
		"}\n")

	# Copy BundleConfig.json
	if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../packaging/android/BundleConfig.json")
		file (COPY "${CMAKE_CURRENT_LIST_DIR}/../packaging/android/BundleConfig.json" DESTINATION "${gradle_project_dir}")
	endif ()

	# Copy libc++_shared.so to jniLibs folder
	set (arches armeabi-v7a|arm-linux-androideabi arm64-v8a|aarch64-linux-android x86|i686-linux-android x86_64|x86_64-linux-android)

	foreach (arch ${arches})
		string (REGEX MATCH "^[^|]*" folder "${arch}")
		string (REGEX MATCH "[^|]*$" triple "${arch}")

		file (COPY "${CMAKE_SYSROOT}/usr/lib/${triple}/libc++_shared.so" DESTINATION "${gradle_project_dir}/native/jniLibs/${folder}")
	endforeach ()

	# Copy ASan libraries to jniLibs folder
	string(REGEX MATCH "^[0-9]+" clang_major_version ${CMAKE_CXX_COMPILER_VERSION})

	set (arches armeabi-v7a|arm arm64-v8a|aarch64 x86|i686 x86_64|x86_64)

	foreach (arch ${arches})
		string (REGEX MATCH "^[^|]*" folder "${arch}")
		string (REGEX MATCH "[^|]*$" arch "${arch}")

		file (REMOVE
			"${gradle_project_dir}/native/jniLibs/${folder}/libclang_rt.asan-${arch}-android.so"
			"${gradle_project_dir}/native/jniLibs/${folder}/libclang_rt.hwasan-${arch}-android.so"
			"${gradle_project_dir}/native/src/main/resources/lib/${folder}/wrap.sh"
		)

		if (ENABLE_ASAN)
			file (COPY "${CMAKE_SYSROOT}/../lib/clang/${clang_major_version}/lib/linux/libclang_rt.asan-${arch}-android.so" DESTINATION "${gradle_project_dir}/native/jniLibs/${folder}")

			file (GENERATE OUTPUT "${gradle_project_dir}/native/src/main/resources/lib/${folder}/wrap.sh"
				CONTENT "#!/system/bin/sh\nHERE=$(cd \"$(dirname \"$0\")\" && pwd)\nexport \"LD_PRELOAD=$HERE/libclang_rt.asan-${arch}-android.so $HERE/libc++_shared.so\"\nexec \"$@\"\n"
				NEWLINE_STYLE LF
			)
		elseif (ENABLE_HWASAN AND "${arch}" MATCHES "(aarch64)|(x86_64)")
			file (COPY "${CMAKE_SYSROOT}/../lib/clang/${clang_major_version}/lib/linux/libclang_rt.hwasan-${arch}-android.so" DESTINATION "${gradle_project_dir}/native/jniLibs/${folder}")

			file (GENERATE OUTPUT "${gradle_project_dir}/native/src/main/resources/lib/${folder}/wrap.sh"
				CONTENT "#!/system/bin/sh\nLD_HWASAN=1 exec \"$@\"\n"
				NEWLINE_STYLE LF
			)
		endif ()
	endforeach ()
	
	# Clean up per-architecture projects when generating fresh
	if (VENDOR_GENERATE_FRESH)
		file (REMOVE_RECURSE "${gradle_project_dir}/native/.cxx")
	endif ()
endmacro ()

# Generate an AAR packaging project for Java parts and resources.
# @group android
# @param {STRING} project  Gradle project name.
# @param {STRING} NAMESPACE  Java namespace for AAR.
# @param {STRING} DEPENDS  [variadic] List of dependency target names.
macro (ccl_add_aar_project project)
	if (NOT TARGET ${project}.aar)
		cmake_parse_arguments (params "" "NAMESPACE" "DEPENDS" ${ARGN})

		string (REPLACE "-" "" projectname "${project}")

		if (TARGET ${project})
			ccl_set_target_gradle_prefix (${project} "${projectname}")

			list (APPEND params_DEPENDS ${project})
		endif ()

		set (project_dir "${CMAKE_CURRENT_BINARY_DIR}/${project}.aar")
		set (gradle_dir "${CMAKE_CURRENT_BINARY_DIR}/gradle/${projectname}")

		file (MAKE_DIRECTORY "${project_dir}")

		set (proguard_file "${project_dir}/rules.proguard")

		file (WRITE "${proguard_file}"
			"-keeppackagenames\n")

		set (build_file "${project_dir}/build.gradle")

		if (CCL_BUILD_SUPPORT_DIR)
			set (build_support_dir "${CCL_BUILD_SUPPORT_DIR}")
		else ()
			set (build_support_dir "${CCL_FRAMEWORK_ROOT}/build")
		endif ()

		file (WRITE "${build_file}"
			"plugins {\n"
			"\talias(libs.plugins.android.library)\n"
			"}\n\n"
			"apply from: \"${CMAKE_CURRENT_BINARY_DIR}/gradle/toolchain.gradle\"\n\n"
			"if(rootProject.file(\"${gradle_dir}/properties.gradle\").exists())\n"
			"\tapply from: \"${gradle_dir}/properties.gradle\"\n\n"
			"apply from: \"${build_support_dir}/android/gradle/common.gradle\"\n\n"
			"android {\n"
			"\tnamespace = 'com.example.${projectname}'\n\n"
			"\tsourceSets {\n"
			"\t\tmain {\n"
			"\t\t\tres.srcDirs = ['build/res']\n"
			"\t\t\tassets.srcDirs = ['build/assets']\n"
			"\t\t}\n"
			"\t}\n\n"
			"\tbuildTypes {\n"
			"\t\trelease {\n"
			"\t\t\tproguardFiles += 'rules.proguard'\n"
			"\t\t}\n"
			"\t}\n"
			"}\n\n"
			"dependencies {\n"
			"\timplementation libs.core\n"
			"}\n")

		set (files includes;assets;resources;sources;dependencies;proguard)

		foreach (file ${files})
			file (APPEND "${build_file}"
				"\n"
				"if(rootProject.file(\"${gradle_dir}/${file}.gradle\").exists())\n"
				"\tapply from: \"${gradle_dir}/${file}.gradle\"\n")
		endforeach ()

		foreach (dependency IN ITEMS ${params_DEPENDS})
			if (NOT "${project}" STREQUAL "${dependency}")
				ccl_create_file_once (${gradle_dir}/dependencies.gradle)
				file (APPEND "${gradle_dir}/dependencies.gradle"
					"dependencies { implementation project(':${dependency}') }\n")
			endif ()
		endforeach ()

		# Create custom target
		add_custom_target (${project}.aar)

		set_target_properties (${project}.aar PROPERTIES AAR_PROJECT "${project}")
	endif ()

	get_target_property (${project}_AAR_PROJECT ${project}.aar AAR_PROJECT)

	if (NOT "${project}.aar|${${project}_AAR_PROJECT}" IN_LIST PROJECT_AAR_PROJECTS)
		list (APPEND PROJECT_AAR_PROJECTS "${project}.aar|${${project}_AAR_PROJECT}")
	endif ()
endmacro ()

# Install an AAR generated by a packaging project.
# @group android
# @param {STRING} project  Gradle project name.
# @param {STRING} COMPONENT  Install component.
macro (ccl_install_aar project)
	cmake_parse_arguments (params "" "COMPONENT" "" ${ARGN})

	string (REPLACE "-" "" projectname "${project}")

	set (arch "${VENDOR_PLATFORM_SUBDIR}")
	if ("${VENDOR_PLATFORM_SUBDIR}" STREQUAL "arm")
		set (arch "arm7")
	elseif ("${VENDOR_PLATFORM_SUBDIR}" STREQUAL "arm64")
		set (arch "arm8")
	elseif ("${VENDOR_PLATFORM_SUBDIR}" STREQUAL "x64")
		set (arch "x86_64")
	endif ()

	if (CCL_SYSTEM_INSTALL)
		install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${project}.aar/build/outputs/aar/${project}-${arch}-$<IF:$<CONFIG:DEBUG>,debug,release>.aar RENAME ${projectname}.aar DESTINATION "lib/android-java" COMPONENT ${params_COMPONENT})
	elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
		install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${project}.aar/build/outputs/aar/${project}-${arch}-$<IF:$<CONFIG:DEBUG>,debug,release>.aar RENAME ${projectname}.aar DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")
	endif ()
endmacro ()
