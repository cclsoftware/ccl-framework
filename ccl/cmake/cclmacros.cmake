set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
get_filename_component (platformdir ${platformdir} ABSOLUTE)
ccl_find_file (ccl_PLATFORMMACROS_FILE NAMES "cclmacros.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional macros.")

if (EXISTS "${ccl_PLATFORMMACROS_FILE}")
	include ("${ccl_PLATFORMMACROS_FILE}")
else ()
	set (ccl_PLATFORMMACROS_FILE "")
	message (DEBUG "Could not find a vendor specific CCL macros file")
endif ()

find_package (ccltools REQUIRED COMPONENTS crypt package)

# Get the name of an isolated target, if it exists.
# Returns the plain target name if an isolated variant does not exist
# @param {STRING} target  Name of the target without isolation postfix.
# @param {STRING} result  Name of the result variable.
macro (ccl_get_isolated_target plain_target result)
	set (${result} "${plain_target}")
	if (NOT "${CCL_ISOLATION_POSTFIX}" STREQUAL "")
		if (TARGET "${plain_target}.${CCL_ISOLATION_POSTFIX}")
			string (APPEND ${result} ".${CCL_ISOLATION_POSTFIX}")
		endif ()
	endif ()
endmacro ()

# Link ccl framework, optionally using application- or version-dependent library names
macro (target_link_ccl_framework target)
	cmake_parse_arguments (ccl_link_params "" "ISOLATION_POSTFIX;APPEND_VERSION;APPEND_ABI" "PUBLIC;PRIVATE;INTERFACE" ${ARGN})

	if (NOT "${ccl_link_params_ISOLATION_POSTFIX}" STREQUAL "")
		set (CCL_ISOLATION_POSTFIX "${ccl_link_params_ISOLATION_POSTFIX}")
		set (CCL_EXPORT_POSTFIX "_${CCL_ISOLATION_POSTFIX}")
		find_package (ccl REQUIRED COMPONENTS ${ccl_link_params_PRIVATE} ${ccl_link_params_PUBLIC})
	endif ()
	
	foreach (linkage PRIVATE PUBLIC INTERFACE)
		foreach (component ${ccl_link_params_${linkage}})
			ccl_get_isolated_target ("${component}" current_component)
			target_link_libraries (${target} ${linkage} "${current_component}")
		endforeach ()
	endforeach ()
endmacro ()

# Add a plug-in library, preconfigured to match vendor guidelines.
# @param {STRING} target  Name of the target to add.
macro (ccl_add_plugin_library target)
	set (_current_plugin_target "${target}")
	if (CCL_ISOLATION_POSTFIX)
		string (APPEND _current_plugin_target ".${CCL_ISOLATION_POSTFIX}")
	endif ()
	set (${target}_target "${_current_plugin_target}")
	get_directory_property (has_parent PARENT_DIRECTORY)
	if (NOT has_parent STREQUAL "")
		set (${target}_target "${_current_plugin_target}" PARENT_SCOPE)
	endif ()
	ccl_add_library (${target} MODULE SUBDIR "Plugins" POSTFIX "${CCL_ISOLATION_POSTFIX}" ${ARGN})
	ccl_export_symbols (${_current_plugin_target}
		CCLModuleMain
		CCLGetClassFactory
	)
	target_include_directories (${_current_plugin_target} PRIVATE ${VENDOR_INCLUDE_DIRS})
endmacro ()

# File glob and filter for files starting with a "." (hidden on many OSs)
# @param {STRING} file_list Name of the variable to put the result into.
# @param {STRING} glob Glob expressions (path with wildcards)
macro (ccl_file_glob file_list glob)
	file (GLOB_RECURSE ${file_list} ${glob})
	list (FILTER ${file_list} EXCLUDE REGEX "/\\.[^\/]+$")
endmacro ()

# Add a package containing scripts, generated from a .in description file or a folder.
# @param {STRING} target  Name of the target to add the script package to.
# @param {FILEPATH} input  Script package .in file or folder for the packagetool.
macro (ccl_add_script_package target input)
	if (IS_DIRECTORY ${input})
		get_filename_component (packageName ${input} NAME)

		set (depfile "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/dep/${target}/${packageName}.scriptpackage.d")
		cmake_path (NATIVE_PATH depfile NORMALIZE nativeDepfile)

		if (VENDOR_ENABLE_PARALLEL_BUILDS)
			set (packagePath "${CMAKE_BINARY_DIR}/assets/${target}/scripts/${packageName}.package")
		else ()
			set (packagePath "${input}/../../assets/scripts/${packageName}.package")
		endif ()

		cmake_path (NATIVE_PATH packagePath NORMALIZE nativePackagePath)
		cmake_path (NATIVE_PATH input NORMALIZE nativeInput)

		add_custom_command (OUTPUT ${packagePath}
			COMMAND ${cclpackage} -c "${nativeInput}" "${nativePackagePath}" -depfile "${nativeDepfile}"
			COMMAND ${CMAKE_COMMAND} -E touch "${packagePath}"
			DEPENDS "${input}" ${cclpackage_build}
			DEPFILE "${depfile}"
			VERBATIM USES_TERMINAL
		)
	else ()
		get_filename_component (baseDir ${input} DIRECTORY)
		get_filename_component (inFile ${input} NAME)
		string (REPLACE ".package.in" "" packageName ${inFile})
		
		cmake_path (NATIVE_PATH inFile NORMALIZE nativeInput)
		
		set (depfile "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/dep/${target}/${packageName}.scriptpackage.d")
		cmake_path (NATIVE_PATH depfile NORMALIZE nativeDepfile)

		if (VENDOR_ENABLE_PARALLEL_BUILDS)
			set (outputDirectory "${CMAKE_BINARY_DIR}/assets/${target}")
			cmake_path (NATIVE_PATH outputDirectory NORMALIZE nativeOutputDirectory)
			set (packagePath "${outputDirectory}/scripts/${packageName}.package")
		else ()
			set (outputDirectory "")
			set (nativeOutputDirectory "")
			set (packagePath "${baseDir}/../assets/scripts/${packageName}.package")
		endif ()

		set (scriptSourcePath "${baseDir}/${packageName}")
		if (NOT TARGET scripts_${packageName})
			ccl_file_glob (dependencies "${scriptSourcePath}/*")
			add_custom_command (OUTPUT ${packagePath}
				COMMAND ${cclpackage} -dest "${nativeOutputDirectory}" -batch "${nativeInput}" -depfile "${nativeDepfile}"
				COMMAND ${CMAKE_COMMAND} -E touch "${packagePath}"
				DEPENDS ${dependencies} "${input}" ${cclpackage_build}
				DEPFILE "${depfile}"
				WORKING_DIRECTORY ${baseDir}
				VERBATIM USES_TERMINAL
			)
			source_group (TREE "${scriptSourcePath}" PREFIX "scripts" FILES ${dependencies})
		endif ()
	endif ()
	
	if (NOT TARGET scripts_${target}_${packageName})
		add_custom_target (scripts_${target}_${packageName} DEPENDS ${packagePath})
		target_sources (scripts_${target}_${packageName} PRIVATE ${dependencies} "${input}")
		source_group ("scripts" FILES "${input}")
		set_target_properties (scripts_${target}_${packageName} PROPERTIES USE_FOLDERS ON FOLDER scripts)
	endif ()
	
	set_source_files_properties (${packagePath} PROPERTIES GENERATED 1)

	ccl_add_assets ("${target}" FILES "${packagePath}" PATH "Scripts")
	add_dependencies (${target} scripts_${target}_${packageName})
endmacro ()

# Add a snapshot package, generated from a directory.
# @param {STRING} target  Name of the target to add the snapshot to.
# @param {FILEPATH} input  Directory containing a snapshot subdirectory for the packagetool.
macro (ccl_add_snapshot_package target input)
	get_filename_component (baseDir ${input} DIRECTORY)
	get_filename_component (packageName ${input} NAME)
	
	cmake_parse_arguments (snapshot_params "" "SUBDIR" "" ${ARGN})
	if (NOT snapshot_params_SUBDIR)
		set (snapshot_params_SUBDIR "snapshots")
	endif ()

	if (NOT TARGET snapshots_${target}_${packageName})

		if (VENDOR_ENABLE_PARALLEL_BUILDS)
			set (destinationDir "${CMAKE_BINARY_DIR}/assets/${target}/snapshots")
			set (depfile "${destinationDir}/${packageName}.package.d")
		else ()
			set (destinationDir "${CMAKE_CURRENT_LIST_DIR}/../assets/snapshots")
			set (depfile "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BUILD_TYPE}/dep/${target}/${packageName}.snapshotpackage.d")
		endif ()

		set (sourcePath "${input}/${snapshot_params_SUBDIR}")
		set (packagePath "${destinationDir}/${packageName}.package")
		
		set_source_files_properties (${packagePath} PROPERTIES GENERATED 1)

		cmake_path (NATIVE_PATH packagePath NORMALIZE nativePackagePath)
		cmake_path (NATIVE_PATH sourcePath NORMALIZE nativeSourcePath)
		cmake_path (NATIVE_PATH depfile NORMALIZE nativeDepfile)
		ccl_file_glob (dependencies "${sourcePath}")
		add_custom_command (OUTPUT "${packagePath}"
			COMMAND ${cclpackage} -z "${nativeSourcePath}" "${nativePackagePath}" -depfile "${nativeDepfile}"
			COMMAND ${CMAKE_COMMAND} -E touch "${packagePath}"
			DEPENDS ${dependencies} ${cclpackage_build}
			DEPFILE "${depfile}"
			WORKING_DIRECTORY ${baseDir}
			VERBATIM USES_TERMINAL
		)
		add_custom_target (snapshots_${target}_${packageName} DEPENDS ${packagePath})
		target_sources (snapshots_${target}_${packageName} PRIVATE ${sourcePath})
		source_group (TREE "${sourcePath}" PREFIX "${snapshot_params_SUBDIR}" FILES ${dependencies})
		set_target_properties (snapshots_${target}_${packageName} PROPERTIES USE_FOLDERS ON FOLDER snapshots)
	endif ()
	
	ccl_add_assets ("${target}" FILES "${packagePath}" PATH "Snapshots")
	add_dependencies (${target} snapshots_${target}_${packageName})

endmacro ()

# Add an extension package, generated from a .build description file.
# @param {STRING} target  Name of the target to add the extension to.
# @param {FILEPATH} input  Extension package .build file for the packagetool.
macro (ccl_add_extension_package target input)
	get_filename_component (baseDir ${input} DIRECTORY)
	get_filename_component (inFile ${input} NAME)
	
	cmake_parse_arguments (package_params "SKIP_DEPLOYMENT;DEPLOY_PACKAGE" "SUFFIX;OUTPUT;PATH" "DEPENDENCIES" ${ARGN})
	
	string (REPLACE "${package_params_SUFFIX}.build" "" packageName ${inFile})
	
	if (package_params_OUTPUT)
		set (packageFileName "${package_params_OUTPUT}")
	else ()
		set (packageFileName "${packageName}")
	endif ()
	
	if (package_params_OUTPUT AND IS_ABSOLUTE "${package_params_OUTPUT}")
		set (packagePath "${package_params_OUTPUT}")
		set (nativeDest "")
	elseif (package_params_DEPLOY_PACKAGE)
		if (VENDOR_ENABLE_PARALLEL_BUILDS)	
			set (dest "${CMAKE_BINARY_DIR}/assets/${target}/extensions/~install")
			set (packagePath "${dest}/${packageFileName}.package")
			cmake_path (NATIVE_PATH dest NORMALIZE nativeDest)
		else ()
			set (packagePath "${baseDir}/${packageFileName}.package")
			set (nativeDest "")
		endif ()
	else ()
		if (VENDOR_ENABLE_PARALLEL_BUILDS)	
			set (dest "${CMAKE_BINARY_DIR}/assets/${target}/extensions/~install")
			set (packagePath "${dest}/${packageFileName}")
			cmake_path (NATIVE_PATH dest NORMALIZE nativeDest)		
		else ()
			set (packagePath "${baseDir}/~install/${packageFileName}")
			set (nativeDest "")
		endif ()
	endif ()
	
	set_source_files_properties (${packagePath} PROPERTIES GENERATED 1)

	if (NOT package_params_PATH)
		set (package_params_PATH "Extensions")
	endif ()

	set (package_target package_${packageName}${package_params_SUFFIX})

	if (NOT TARGET ${package_target})

		if (package_params_OUTPUT)
			set (outputFile "${packagePath}")
			set (depfile "${outputFile}.d")
		elseif (package_params_DEPLOY_PACKAGE)
			set (outputFile "${packagePath}")
			set (depfile "${outputFile}.d")
		else ()
			set (outputFile "${packagePath}/metainfo.xml")
			set (depfile "") # depfile won't work if we don't know the primary output name
		endif ()
		
		cmake_path (NATIVE_PATH inFile NORMALIZE nativeInput)
		cmake_path (NATIVE_PATH depfile NORMALIZE nativeDepfile)

		set (sourcePath "${baseDir}/${packageName}")
		ccl_file_glob (dependencies "${sourcePath}/*")
		list (APPEND dependencies "${input}")
		add_custom_command (OUTPUT "${outputFile}"
			COMMAND ${cclpackage} -dest "${nativeDest}" -batch "${nativeInput}" -depfile "${nativeDepfile}"
			COMMAND ${CMAKE_COMMAND} -E touch "${outputFile}"
			DEPENDS ${dependencies} ${package_params_DEPENDENCIES} ${cclpackage_build}
			DEPFILE "${depfile}"
			WORKING_DIRECTORY ${baseDir}
			VERBATIM USES_TERMINAL
		)
		add_custom_target (${package_target} DEPENDS ${outputFile})

		target_sources (${package_target} PRIVATE ${dependencies})
		source_group (TREE "${sourcePath}" PREFIX "extension" FILES ${dependencies})
		set_target_properties (${package_target} PROPERTIES USE_FOLDERS ON FOLDER extensions)

	endif ()
	
	if (package_params_SKIP_DEPLOYMENT)
		add_dependencies (${target} ${package_target})
	elseif (package_params_DEPLOY_PACKAGE)
		ccl_add_assets ("${target}" FILES "${packagePath}" PATH "${package_params_PATH}")
		add_dependencies (${target} ${package_target})
	else ()
		ccl_add_assets ("${target}" DIRECTORY "${packagePath}" PATH "${package_params_PATH}")
		add_dependencies (${target} ${package_target})
	endif ()
endmacro ()

# Add an extension.
# @param {STRING} target  Name of the target to add the extension to.
# @param {FILEPATH} input  Extension project name.
macro (ccl_add_extension target input)
	cmake_parse_arguments (extension_params "SKIP_DEPLOYMENT" "PATH" "" ${ARGN})
	
	if (NOT package_params_PATH)
		set (package_params_PATH "Extensions")
	endif ()

	add_subdirectory ("${REPOSITORY_EXTENSIONS_DIR}/development/${input}/cmake" "extensions/${input}")
	set (packagePath "${REPOSITORY_EXTENSIONS_DIR}/deployment/${input}")
	set_target_properties (${input} PROPERTIES USE_FOLDERS ON FOLDER extensions)
	
	if (extension_params_SKIP_DEPLOYMENT)
		add_dependencies (${target} ${input})
	else ()
		ccl_add_assets ("${target}" DIRECTORY "${packagePath}" PATH "${package_params_PATH}")
		ccl_add_dependencies (${target} ${input})
	endif ()
endmacro ()

# Create an XML signature for an input file.
# @param {FILEPATH} INPUT  Path to the input file.
# @param {FILEPATH} OUTPUT  [optional] Path to the resulting xml file.
# @param {FILEPATH} KEY_FILE  Path to private key file.
# @param {STRING} ROOT_NAME  Name of the XML root node.
macro (ccl_sign)
	cmake_parse_arguments (sign_params "" "INPUT;OUTPUT;KEY_FILE;ROOT_NAME" "" ${ARGN})

	if (NOT sign_params_OUTPUT)
		set (sign_params_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sign/${sign_params_INPUT}.signed.xml")
	endif ()
	
	cmake_path (NATIVE_PATH sign_params_OUTPUT NORMALIZE native_output)
	cmake_path (NATIVE_PATH sign_params_INPUT NORMALIZE native_input)
	cmake_path (NATIVE_PATH sign_params_KEY_FILE NORMALIZE native_keyfile)

	add_custom_command (
		COMMAND "${cclcrypt}" -sign "${native_output}" "${native_input}" "${sign_params_ROOT_NAME}" "${native_keyfile}"
		OUTPUT "${sign_params_OUTPUT}"
		DEPENDS "${sign_params_INPUT}" ${cclcrypt_build}
	)
endmacro ()

# Encrypt a file.
# @param {FILEPATH} INPUT  Path to the input file.
# @param {FILEPATH} OUTPUT  [optional] Path to the resulting xml file.
# @param {FILEPATH} CIPHER  Path to a cipher xml file.
macro (ccl_encrypt)
	cmake_parse_arguments (encrypt_params "" "INPUT;OUTPUT;CIPHER" "" ${ARGN})

	if (NOT encrypt_params_OUTPUT)
		set (encrypt_params_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/encrypt/${encrypt_params_INPUT}.encrypted")
	endif ()
	
	cmake_path (NATIVE_PATH encrypt_params_OUTPUT NORMALIZE native_output)
	cmake_path (NATIVE_PATH encrypt_params_INPUT NORMALIZE native_input)
	cmake_path (NATIVE_PATH encrypt_params_CIPHER NORMALIZE native_cipher)

	add_custom_command (
		COMMAND "${cclcrypt}" -encrypt "${native_output}" "${native_input}" "${native_cipher}"
		OUTPUT "${encrypt_params_OUTPUT}"
		DEPENDS "${encrypt_params_INPUT}" ${cclcrypt_build}
	)
endmacro ()

# Sign and encrypt an authorization policy xml file and write it to a cpp file.
# @param {FILEPATH} INPUT  Path to the input file.
# @param {FILEPATH} OUTPUT  [optional] Path to the resulting cpp file.
# @param {FILEPATH} SIGNED  [optional] Path to the intermediate signed file.
# @param {FILEPATH} ENCRYPTED  [optional] Path to the intermediate encrypted file.
# @param {STRING} ROOT_NAME  [optional] Name of the XML root node.
# @param {FILEPATH} KEY_FILE  [optional] Path to private key file.
# @param {FILEPATH} CIPHER  [optional] Path to a cipher xml file.
# @param {STRING} PREFIX  [optional] Variable name prefix used in the generated cpp file.
macro (ccl_make_authpolicy)
	cmake_parse_arguments (auth_params "" "INPUT;OUTPUT;SIGNED;ENCRYPTED;ROOT_NAME;KEY_FILE;CIPHER;PREFIX" "" ${ARGN})

	if (NOT auth_params_OUTPUT)
		set (auth_params_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bincpp/${auth_params_INPUT}.signed.encrypted.cpp")
	endif ()

	get_filename_component (input_filename ${auth_params_INPUT} NAME)

	if (NOT auth_params_SIGNED)
		set (auth_params_SIGNED "${CMAKE_CURRENT_BINARY_DIR}/authpolicy/${input_filename}.signed.xml")
	endif ()
	
	if (NOT auth_params_ENCRYPTED)
		set (auth_params_ENCRYPTED "${CMAKE_CURRENT_BINARY_DIR}/authpolicy/${input_filename}.signed.encrypted")
	endif ()
	
	if (NOT auth_params_PREFIX)
		set (auth_params_PREFIX "SignedAuthorizationPolicyEncrypted")
	endif ()
	
	if (NOT auth_params_ROOT_NAME)
		set (auth_params_ROOT_NAME "SignedAuthorizationPolicy")
	endif ()
	
	ccl_sign (
		INPUT "${auth_params_INPUT}"
		OUTPUT "${auth_params_SIGNED}"
		KEY_FILE "${auth_params_KEY_FILE}"
		ROOT_NAME "${auth_params_ROOT_NAME}"
	)
	
	ccl_encrypt (
		INPUT "${auth_params_SIGNED}"
		OUTPUT "${auth_params_ENCRYPTED}"
		CIPHER "${auth_params_CIPHER}"
	)
	
	ccl_makebin (
		INPUT "${auth_params_ENCRYPTED}"
		OUTPUT "${auth_params_OUTPUT}"
		PREFIX "${auth_params_PREFIX}"
	)
endmacro ()

# Sign and encrypt a public key file and an authorization policy.
# @param {PATH} SECURITY_DIR  [optional] Directory containing cipher, key files and auth policy.
# @param {FILEPATH} CIPHER  [optional] Path to a cipher xml file.
# @param {FILEPATH} KEY_FILE  [optional] Path to private key file.
# @param {FILEPATH} PUBLIC_KEY_FILE  [optional] Path to a public key file.
# @param {FILEPATH} AUTH_POLICY  [optional] Path to an authorization policy xml file.
macro (ccl_make_appsecurity)
	cmake_parse_arguments (auth_params "" "SECURITY_DIR;CIPHER;KEY_FILE;PUBLIC_KEY_FILE;AUTH_POLICY" "" ${ARGN})
	
	if (NOT auth_params_KEY_FILE)
		set (auth_params_KEY_FILE "${auth_params_SECURITY_DIR}/authorizer.privatekey")
	endif ()
	
	if (NOT auth_params_CIPHER)
		set (auth_params_CIPHER "${auth_params_SECURITY_DIR}/cipher.xml")
	endif ()
	
	if (NOT auth_params_PUBLIC_KEY_FILE)
		set (auth_params_PUBLIC_KEY_FILE "${auth_params_SECURITY_DIR}/authorizer.publickey")
	endif ()
	
	if (NOT auth_params_AUTH_POLICY)
		set (auth_params_AUTH_POLICY "${auth_params_SECURITY_DIR}/authpolicy.xml")
	endif ()
	
	ccl_encrypt (
		INPUT ${auth_params_PUBLIC_KEY_FILE}
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/encrypt/authorizer.publickey.encrypted
		CIPHER ${auth_params_CIPHER}
	)
	ccl_makebin (
		INPUT ${CMAKE_CURRENT_BINARY_DIR}/encrypt/authorizer.publickey.encrypted
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bincpp/authorizerkey.encrypted.cpp
		PREFIX AuthorizerPublicKeyEncrypted
	)
	
	ccl_make_authpolicy (
		INPUT ${auth_params_AUTH_POLICY}
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bincpp/authpolicy.signed.encrypted.cpp
		CIPHER ${auth_params_CIPHER}
		KEY_FILE ${auth_params_KEY_FILE}
	)

	set (appsecurity_bincpp_files
		${CMAKE_CURRENT_BINARY_DIR}/bincpp/authorizerkey.encrypted.cpp
		${CMAKE_CURRENT_BINARY_DIR}/bincpp/authpolicy.signed.encrypted.cpp
	)
endmacro ()

# Add a target containing unit tests. This will add a custom target in order to run the tests.
# For generators XCode and Visual Studio, the debugger command for the provided target is replaced
# by the testrunner in order to directly debug tests.
# @param {STRING} target  Name of the target to add the extension to.
function (ccl_add_test target)
	if (NOT CCL_ENABLE_TESTING)
		return ()
	endif ()

	find_package (testrunner)
	
	if (NOT TARGET testrunner)
		return ()
	endif ()
	
	add_dependencies (${target} testrunner)

	if (APPLE)
		get_target_property (testrunner_directory testrunner RUNTIME_OUTPUT_DIRECTORY)
		cmake_path (SET testrunner_directory NORMALIZE ${testrunner_directory})

		# Debug executables can't use generator expressions and must be defined during configuration
		string (REPLACE "$<CONFIG>" "Debug" testrunner_directory ${testrunner_directory})
		
		get_target_property (testrunner_output_name testrunner OUTPUT_NAME)
		
		set (testrunner_application "${testrunner_directory}/${testrunner_output_name}.app/Contents/MacOS/${testrunner_output_name}")
		set (plugin_debug_path "$(TARGET_BUILD_DIR)/$(TARGET_NAME).$(WRAPPER_EXTENSION)")
		set (plugin_path "$<TARGET_BUNDLE_DIR:${target}>")
	else ()
		set (testrunner_application "$<SHELL_PATH:$<TARGET_FILE:testrunner>>")
		set (plugin_debug_path "$<SHELL_PATH:$<TARGET_FILE:${target}>>")
		set (plugin_path "${plugin_debug_path}")
	endif ()

	add_custom_target (${target}_unittest
		COMMAND "${testrunner_application}" "${plugin_path}"
	)
	set_target_properties (${target}_unittest PROPERTIES USE_FOLDERS ON FOLDER cmake/tests)	
	add_dependencies (${target}_unittest ${target})
	
	if (COMMAND ccl_set_debug_command)
		ccl_set_debug_command (${target}
			DEBUG_EXECUTABLE "${testrunner_application}"
			DEBUG_ARGUMENTS "${plugin_debug_path}"	
		)
	endif ()
endfunction ()
