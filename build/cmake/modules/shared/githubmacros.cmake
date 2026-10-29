# download file from GitHub Release
#
# authentication for non-PUBLIC downloads is taken from the GITHUB_TOKEN CMake variable
# (e.g. passed as -DGITHUB_TOKEN=... so it can be forwarded as a flatpak-builder secret
# config-opt, since secret-env is not visible to the CMake configure step), falling back
# to the GH_TOKEN environment variable for local/interactive builds.
function (download_from_github_release repository release_tag file_name destination)
	cmake_parse_arguments (github_download_params "PUBLIC;SOURCE" "RENAME" "" ${ARGN})

	if (NOT GITHUB_TOKEN AND DEFINED ENV{GH_TOKEN})
		set (GITHUB_TOKEN "$ENV{GH_TOKEN}")
	endif ()
	if (github_download_params_RENAME)
		set (output_file "${destination}/${github_download_params_RENAME}")
	else ()
		set (output_file "${destination}/${file_name}")
	endif ()

	if(github_download_params_PUBLIC)
		file (MAKE_DIRECTORY "${destination}")
		if (github_download_params_SOURCE)
			file (DOWNLOAD "https://github.com/${repository}/archive/refs/tags/${release_tag}.zip" "${output_file}" STATUS file_download_result)
		else ()
			file (DOWNLOAD "https://github.com/${repository}/releases/download/${release_tag}/${file_name}" "${output_file}" STATUS file_download_result)
		endif ()
		list (GET file_download_result 0 file_download_result_code)
		if (NOT ${file_download_result_code} EQUAL 0)
			list (GET file_download_result 1 file_download_result_message)
			message (WARNING "CMake failed to download ${file_name} with result ${file_download_result_message}.")
			return ()
		endif ()
	else ()
		find_program (github_cli_command gh)
		if (NOT github_cli_command AND NOT GITHUB_TOKEN)
			message (WARNING "Could not find GitHub CLI. Please install it from https://cli.github.com and run 'gh auth login' to authorize it.")
			return ()
		endif ()

		file (MAKE_DIRECTORY "${destination}")
		if (github_cli_command)
			execute_process (COMMAND "${github_cli_command}" "release" "download" "${release_tag}" "--pattern" "${file_name}" "--repo" "${repository}" --output "${output_file}" WORKING_DIRECTORY "${destination}" TIMEOUT 240 RESULT_VARIABLE github_cli_result)
			if(github_cli_result)
				message (WARNING "GitHub CLI failed to download ${file_name} with result ${github_cli_result}.")
				return ()
			endif ()
		else ()
			set (api_headers
				HTTPHEADER "Authorization: Bearer ${GITHUB_TOKEN}"
				HTTPHEADER "Accept: application/vnd.github+json"
				HTTPHEADER "X-GitHub-Api-Version: 2022-11-28"
			)

			# Step 1: get release metadata (includes asset list + ids)
			set (release_json "${CMAKE_BINARY_DIR}/_gh_release_${release_tag}.json")
			file (DOWNLOAD
				"https://api.github.com/repos/${repository}/releases/tags/${release_tag}"
				"${release_json}"
				STATUS release_status
				${api_headers}
			)
			list (GET release_status 0 release_status_code)
			if (NOT release_status_code EQUAL 0)
				message (WARNING "Failed to fetch release metadata for ${release_tag}")
				return ()
			endif ()

			file (READ "${release_json}" release_contents)

			# Step 2: find the asset id matching file_name
			string (JSON asset_count LENGTH "${release_contents}" "assets")
			math (EXPR last_index "${asset_count} - 1")
			set (asset_id "")
			foreach (i RANGE ${last_index})
				string (JSON this_name GET "${release_contents}" "assets" ${i} "name")
				if (this_name STREQUAL "${file_name}")
					string (JSON asset_id GET "${release_contents}" "assets" ${i} "id")
					break ()
				endif ()
			endforeach ()

			if (asset_id STREQUAL "")
				message (WARNING "Asset ${file_name} not found in release ${release_tag}")
				return ()
			endif ()

			# Step 3: download the actual asset binary
			file (DOWNLOAD
				"https://api.github.com/repos/${repository}/releases/assets/${asset_id}"
				"${output_file}"
				STATUS asset_download_status
				HTTPHEADER "Authorization: Bearer ${GITHUB_TOKEN}"
				HTTPHEADER "Accept: application/octet-stream"
			)
			list (GET asset_download_status 0 asset_download_code)
			if (NOT asset_download_code EQUAL 0)
				list (GET asset_download_status 1 asset_download_message)
				message (WARNING "Failed to download asset ${file_name}: ${asset_download_message}")
			endif ()
		endif ()
	endif ()
endfunction ()
