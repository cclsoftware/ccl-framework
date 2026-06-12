# This macro configures the Apple entitlements file and (optionally) Info.plist file using an @ONLY configure step.
# Configuration requires an ${app_name}.entitlements.in and an Info.plist.in file in /packaging/${VENDOR_PLATFORM}/
#
# Keychain access:
#
# Uniformly configure Apple's keychain access group feature. It will create a keychain group named (TEAMID.com.VENDOR.shared) 
# and copy that name to the Info.plist so it is accessible at runtime when reading or storing credentials.
#
# Use this in the entitlements.in file
#
#	<key>keychain-access-groups</key>
#	<array>
#		<string>$(AppIdentifierPrefix)@entitlements_access_group@</string>
#	</array>
#
# Use this in the Info.plist.in file
#
#	<key>KeychainAccessGroup</key>
#	<string>@entitlements_team_id@.@entitlements_access_group@</string>
#
# Sandboxing:
#
# Set an option named ${${app_name}_ENABLE_SANDBOX} to configure sandbox entitlements for this target.
#
# Use this in the entitlements.in file:
#
#	<key>com.apple.security.app-sandbox</key>
#	<@entitlements_enable_sandbo@x/>
# 
include_guard (DIRECTORY)

macro (ccl_configure_entitlements app_name)

	if(${${app_name}_ENABLE_SANDBOX})
		set(entitlements_enable_sandbox "true")
	else()
		set(entitlements_enable_sandbox "false")
	endif()

	set (entitlements_access_group "com.${PROJECT_VENDOR}.shared")
	set (entitlements_team_id ${APP_SIGNING_TEAMID})
	set (entitlements_input_file "${CMAKE_CURRENT_LIST_DIR}/../packaging/${VENDOR_PLATFORM}/${app_name}.entitlements.in")
	set (entitlements_output_file "${CMAKE_CURRENT_BINARY_DIR}/packaging/${app_name}.entitlements")

	set (info_plist_input_file "${CMAKE_CURRENT_LIST_DIR}/../packaging/${VENDOR_PLATFORM}/Info.plist.in")
	set (info_plist_output_file "${CMAKE_CURRENT_BINARY_DIR}/packaging/Info.plist")

	if (EXISTS ${entitlements_input_file})
		message ("Generating entitlements for ${app_name} in: ${CMAKE_CURRENT_BINARY_DIR}/packaging/")
		configure_file (${entitlements_input_file} ${entitlements_output_file} @ONLY)
		set_target_properties (${app_name} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${entitlements_output_file})
	endif ()
	if (EXISTS ${info_plist_input_file})
		configure_file (${info_plist_input_file} ${info_plist_output_file} @ONLY)
		set_target_properties (${app_name} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${info_plist_output_file})
	endif ()

	# Clear variables
	set (entitlements_enable_sandbox "")
	set (entitlements_access_group "")
	set (entitlements_team_id "")
	set (entitlements_input_file "")
	set (entitlements_output_file "")

	set (info_plist_input_file "")
	set (info_plist_output_file "")
endmacro ()