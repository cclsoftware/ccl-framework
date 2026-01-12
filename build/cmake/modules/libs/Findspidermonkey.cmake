include_guard (DIRECTORY)

ccl_find_path (jsengine_DIR NAMES "source/jsengine.h" HINTS "${CCL_REPOSITORY_ROOT}/services/jsengine" DOC "JavaScript Engine directory")
mark_as_advanced (jsengine_DIR)

set (spidermonkey_version "133.0.3")
set (spidermonkey_release_name "ccl-${spidermonkey_version}")
set (spidermonkey_architecture "${VENDOR_TARGET_ARCHITECTURE}")

if (VENDOR_ENABLE_DEBUG_SPIDERMONKEY)
	set (spidermonkey_postfix "-debug")
else ()
	set (spidermonkey_postfix "")
endif ()

if (APPLE)
	set (spidermonkey_architecture "universal")
elseif (${spidermonkey_architecture} STREQUAL "arm64ec")
	set (spidermonkey_architecture "x86_64")
endif ()

set (spidermonkey_download_path "${jsengine_DIR}/libs")
set (spidermonkey_platform_zip_name "spidermonkey-${spidermonkey_version}-${VENDOR_PLATFORM}-${spidermonkey_architecture}${spidermonkey_postfix}.zip")
set (spidermonkey_platform_zip_path "${spidermonkey_download_path}/${spidermonkey_platform_zip_name}")
set (spidermonkey_platform_dir "${spidermonkey_download_path}/spidermonkey-${spidermonkey_version}-${VENDOR_PLATFORM}-${spidermonkey_architecture}${spidermonkey_postfix}")

if (NOT EXISTS "${spidermonkey_platform_dir}")	
	message (STATUS "Downloading SpiderMonkey platform libraries...")
	download_from_github_release (cclsoftware/gecko "${spidermonkey_release_name}" "${spidermonkey_platform_zip_name}" "${spidermonkey_download_path}" PUBLIC)

	if (NOT EXISTS "${spidermonkey_platform_zip_path}")
		message (FATAL_ERROR "Failed to download SpiderMonkey platform libraries.")
		return ()
	endif ()

	# extract the download and remove archive
	message (STATUS "Extracting SpiderMonkey platform libraries...")
	file (ARCHIVE_EXTRACT INPUT "${spidermonkey_platform_zip_path}" DESTINATION "${spidermonkey_download_path}" TOUCH)
	if (EXISTS "${spidermonkey_platform_dir}")
		ccl_add_to_cache (spidermonkey "${spidermonkey_platform_zip_path}")
	endif ()
	file (REMOVE "${spidermonkey_platform_zip_path}")
	if (NOT EXISTS "${spidermonkey_platform_dir}")
		message (FATAL_ERROR "Failed to extract SpiderMonkey platform libraries from downloaded ${spidermonkey_platform_zip_name}.")
		return ()
	endif ()
endif ()

# continue creating library targets
ccl_find_path (spidermonkey_DIR NAMES "dist/include/jsapi.h" HINTS "${spidermonkey_platform_dir}" DOC "spidermonkey libs directory")
mark_as_advanced (spidermonkey_DIR)

add_library (js_static STATIC IMPORTED)
ccl_include_platform_specifics (js_static)

add_library (jsrust STATIC IMPORTED)
ccl_include_platform_specifics (jsrust)

target_include_directories (js_static INTERFACE ${spidermonkey_DIR}/dist/include)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (spidermonkey
	REQUIRED_VARS spidermonkey_LIBRARIES
)
