
install (EXPORT ccl-targets 
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}/${PROJECT_ARCHITECTURE_SUBDIRECTORY}" 
	COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)

include (CMakePackageConfigHelpers)
configure_package_config_file (${CMAKE_CURRENT_LIST_DIR}/config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/ccl-config.cmake"
	INSTALL_DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
	NO_SET_AND_CHECK_MACRO
)

write_basic_package_version_file (
	"${CMAKE_CURRENT_BINARY_DIR}/ccl-config-version.cmake"
	VERSION "${CCL_VERSION}"
	COMPATIBILITY ExactVersion

	# Disable architecture check as this checks pointer
	# size (only) and prevents configuring for Android.
	ARCH_INDEPENDENT
)

install (FILES
	${CMAKE_CURRENT_BINARY_DIR}/ccl-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/ccl-config-version.cmake
	${corelib_DIR}/cmake/coremacros.cmake
	${corelib_DIR}/cmake/coremacros.android.cmake
	${corelib_DIR}/cmake/coremacros.linux.cmake
	${corelib_DIR}/cmake/coremacros.ios.cmake
	${corelib_DIR}/cmake/coremacros.mac.cmake
	${corelib_DIR}/cmake/coremacros.win.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.android.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.linux.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.ios.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.mac.cmake
	${CMAKE_CURRENT_LIST_DIR}/cclmacros.win.cmake
	${CMAKE_CURRENT_LIST_DIR}/repomacros.cmake
	${CCL_SKINS_DIR}/cmake/skins-config.cmake
	${CCL_TRANSLATIONS_DIR}/cmake/translations-config.cmake
	${CCL_TOOLS_DIR}/ccl/cmake/ccltools-config.cmake
	${CCL_REPOSITORY_ROOT}/build/cmake/modules/shared/targetarch.cmake
	${CCL_REPOSITORY_ROOT}/build/cmake/modules/shared/githubmacros.cmake
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_DIR}/main ${CCL_DIR}/packaging
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}" 
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_DIR}/extras/javascript
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/extras/"
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_DIR}/meta/generated
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/meta"
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_IDENTITIES_DIR}
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}" 
	COMPONENT public_headers
)

# UNSURE: better configure version headers and skip these files?
install (FILES
	${REPOSITORY_ROOT}/buildtime.h
	${REPOSITORY_ROOT}/buildnumber.h
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/resource"
	COMPONENT public_headers
)

foreach (service bluetoothservice cclspy dapservice jsengine modelimporter3d sqlite usbservice)
	if (TARGET "${service}")
		file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/${service}-config.cmake "
			if (${service}_FIND_REQUIRED)
				find_package (ccl ${CCL_VERSION} REQUIRED COMPONENTS ${service})
			else ()
				find_package (ccl ${CCL_VERSION} QUIET COMPONENTS ${service})
			endif ()
			if (TARGET ${service})
				ccl_copy_imported_target (${service} FOLDER services/ccl SUBDIR Plugins)
			endif ()
			"
		)
		install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${service}-config.cmake
			DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
			COMPONENT public_headers
		)
	endif ()
endforeach ()

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/uninstaller-config.cmake "
	find_package (ccl ${CCL_VERSION} REQUIRED COMPONENTS uninstaller)
	ccl_copy_imported_target (uninstaller)
	"
)
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/corelib-config.cmake "
	find_package (ccl ${CCL_VERSION} REQUIRED COMPONENTS corelib \${corelib_FIND_COMPONENTS})
	set (corelib_malloc_sources
		\${CCL_DIR}/../core/malloc/coremalloc.cpp
	)
	"
)
install (FILES 
	${CMAKE_CURRENT_BINARY_DIR}/uninstaller-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/corelib-config.cmake
	DESTINATION "${CCL_CMAKE_EXPORT_DESTINATION}"
	COMPONENT public_headers
)

set (ccl_support_dest_path "${CCL_SUPPORT_DESTINATION}/")
if ("${ccl_support_dest_path}" STREQUAL "./")
	set (ccl_support_dest_path "")
endif ()
cmake_path (NORMAL_PATH ccl_support_dest_path)
install (DIRECTORY ${CCL_SKINS_DIR}/neutralbase
	DESTINATION "${ccl_support_dest_path}skins/"
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_SKINS_DIR}/neutraldesign
	DESTINATION "${ccl_support_dest_path}skins/"
	COMPONENT public_headers
)

install (DIRECTORY ${CCL_REPOSITORY_ROOT}/classmodels
	DESTINATION "${CCL_SUPPORT_DESTINATION}"
	COMPONENT public_headers
)

include (CPackComponent)

cpack_add_component (public_headers DISPLAY_NAME "Public Headers" DESCRIPTION "Public header files")
cpack_add_component (public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX} DISPLAY_NAME "Public Headers (${VENDOR_NATIVE_COMPONENT_SUFFIX})" DESCRIPTION "Platform-specific public header files" DEPENDS public_headers HIDDEN)

cpack_add_component (documentation DISPLAY_NAME "Documentation" DESCRIPTION "Documentation")

cpack_add_component_group (prebuilt_libraries DISPLAY_NAME "Prebuilt Libraries" DESCRIPTION "Prebuilt libraries" EXPANDED)
cpack_add_component (prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX} DISPLAY_NAME "${VENDOR_NATIVE_COMPONENT_SUFFIX}" DESCRIPTION "Prebuilt libraries" GROUP prebuilt_libraries DEPENDS public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX})

cpack_add_component_group (services DISPLAY_NAME "Services" DESCRIPTION "Prebuilt services" EXPANDED)
cpack_add_component (services_${VENDOR_NATIVE_COMPONENT_SUFFIX} DISPLAY_NAME "${VENDOR_NATIVE_COMPONENT_SUFFIX}" DESCRIPTION "Prebuilt services" GROUP services DEPENDS prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})

cpack_add_component (tools DISPLAY_NAME "Tools" DESCRIPTION "Prebuilt tools" DEPENDS services_${VENDOR_NATIVE_COMPONENT_SUFFIX})

ccl_include_platform_specifics (ccl-install)
