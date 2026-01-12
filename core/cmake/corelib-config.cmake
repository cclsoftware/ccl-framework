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
# Filename    : corelib-config.cmake
#************************************************************************************************

find_path (corelib_DIR NAMES "public/coreversion.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "corelib directory")
set (corelib_BASEDIR "${corelib_DIR}/..")

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include (coremacros)

find_package (PNG REQUIRED)

if (VENDOR_STATIC_LIBRARY_DESTINATION)
	set (corelib_STATIC_LIBRARY_DESTINATION "${VENDOR_STATIC_LIBRARY_DESTINATION}")
else ()
	set (corelib_STATIC_LIBRARY_DESTINATION "${VENDOR_LIBRARY_DESTINATION}")
endif ()
set (corelib_PUBLIC_HEADERS_DESTINATION "${VENDOR_PUBLIC_HEADERS_DESTINATION}/core")

# collect source files
ccl_list_append_once (corelib_malloc_sources
	${corelib_DIR}/malloc/coremalloc.cpp
)

ccl_list_append_once (corelib_newoperator_sources
	${corelib_DIR}/malloc/corenewoperator.cpp
	${corelib_DIR}/malloc/corenewoperatorstubs.cpp
	${corelib_DIR}/malloc/coredeleteoperatorstubs.cpp
)

ccl_list_append_once (corelib_public_headers
	${corelib_DIR}/public/coreallocator.h
	${corelib_DIR}/public/corebasicmacros.h
	${corelib_DIR}/public/corebitset.h
	${corelib_DIR}/public/corebuffer.h
	${corelib_DIR}/public/coreclassbundle.h
	${corelib_DIR}/public/coreclassmacros.h
	${corelib_DIR}/public/coreclibrary.h
	${corelib_DIR}/public/corecontainer.h
	${corelib_DIR}/public/coredatetime.h
	${corelib_DIR}/public/coredefpush.h
	${corelib_DIR}/public/coredefpop.h
	${corelib_DIR}/public/coredevelopment.h
	${corelib_DIR}/public/coreenumdef.h
	${corelib_DIR}/public/coreformatter.h
	${corelib_DIR}/public/corehashmap.h
	${corelib_DIR}/public/corehttp.h
	${corelib_DIR}/public/coreinterpolator.h
	${corelib_DIR}/public/coreintrusivelist.h
	${corelib_DIR}/public/corejsonsecurity.h
	${corelib_DIR}/public/corelinkedlist.h
	${corelib_DIR}/public/coremacros.h
	${corelib_DIR}/public/coremalloc.h
	${corelib_DIR}/public/coremath.h
	${corelib_DIR}/public/coremap.h
	${corelib_DIR}/public/coremempool.h
	${corelib_DIR}/public/corememstream.h
	${corelib_DIR}/public/coreobserver.h
	${corelib_DIR}/public/coreplatform.h
	${corelib_DIR}/public/coreplugin.h
	${corelib_DIR}/public/corepoolallocator.h
	${corelib_DIR}/public/coreprimitives.h
	${corelib_DIR}/public/coreprofiler.h
	${corelib_DIR}/public/coreproperty.h
	${corelib_DIR}/public/coresocketaddress.h
	${corelib_DIR}/public/corestorable.h
	${corelib_DIR}/public/corestream.h
	${corelib_DIR}/public/corestreamaccessor.h
	${corelib_DIR}/public/corestringbuffer.h
	${corelib_DIR}/public/corestringtraits.h
	${corelib_DIR}/public/corethreading.h
	${corelib_DIR}/public/coretreeset.h
	${corelib_DIR}/public/coretypes.h
	${corelib_DIR}/public/coreuid.h
	${corelib_DIR}/public/coreurlencoding.h
	${corelib_DIR}/public/corevector.h
	${corelib_DIR}/public/coreversion.h
	${corelib_DIR}/public/coreversionstruct.h
	${corelib_DIR}/public/gui/corealignment.h
	${corelib_DIR}/public/gui/corebitmapdata.h
	${corelib_DIR}/public/gui/corecolor.h
	${corelib_DIR}/public/gui/coremultitouch.h
	${corelib_DIR}/public/gui/corepoint.h
	${corelib_DIR}/public/gui/corerect.h
	${corelib_DIR}/public/gui/corerectlist.h
	${corelib_DIR}/public/gui/coreuiproperties.h
	${corelib_DIR}/public/gui/coreviewinterface.h
	${corelib_DIR}/public/gui/coregameinterface.h
	${corelib_DIR}/public/devices/coregattshared.h
	${corelib_DIR}/public/devices/coregattcentral.h
	${corelib_DIR}/public/devices/coregattperipheral.h
	${corelib_DIR}/public/devices/coreusbhid.h
)

ccl_list_append_once (corelib_generated_headers
	${corelib_DIR}/meta/generated/cpp/coregui-constants-generated.h
)

ccl_list_append_once (corelib_public_sources
	${corelib_DIR}/public/corebuffer.cpp
	${corelib_DIR}/public/coreformatter.cpp
	${corelib_DIR}/public/coreinterpolator.cpp
	${corelib_DIR}/public/corememstream.cpp
	${corelib_DIR}/public/corestreamaccessor.cpp
	${corelib_DIR}/public/coreuid.cpp
)

ccl_list_append_once (corelib_api_headers
	${corelib_DIR}/network/corediscovery.h
	${corelib_DIR}/network/corenetstream.h
	${corelib_DIR}/network/corenetwork.h
	${corelib_DIR}/network/coresocket.h
	${corelib_DIR}/network/coresslsocket.h
	${corelib_DIR}/network/coreudpconnection.h
	
	${corelib_DIR}/portable/gui/corealertbox.h
	${corelib_DIR}/portable/gui/corebitmap.h
	${corelib_DIR}/portable/gui/corecontrols.h
	${corelib_DIR}/portable/gui/corefont.h
	${corelib_DIR}/portable/gui/coregraphics.h
	${corelib_DIR}/portable/gui/corekeyboard.h
	${corelib_DIR}/portable/gui/corelistview.h
	${corelib_DIR}/portable/gui/corestaticview.h
	${corelib_DIR}/portable/gui/coretouchinput.h
	${corelib_DIR}/portable/gui/coreview.h
	${corelib_DIR}/portable/gui/coreviewbuilder.h
	${corelib_DIR}/portable/gui/coreviewcontroller.h
	${corelib_DIR}/portable/gui/coreviewshared.h

	${corelib_DIR}/gui/corebitmapprimitives.h
	${corelib_DIR}/gui/coregesturerecognition.h
	${corelib_DIR}/gui/coreskinformat.h

	${corelib_DIR}/platform/shared/coreplatformatomicstack.h
	${corelib_DIR}/platform/shared/coreplatformdiscovery.h
	${corelib_DIR}/platform/shared/coreplatformdynamiclibrary.h
	${corelib_DIR}/platform/shared/coreplatformfilesystem.h
	${corelib_DIR}/platform/shared/coreplatforminterprocess.h
	${corelib_DIR}/platform/shared/coreplatformnetwork.h
	${corelib_DIR}/platform/shared/coreplatformsocket.h
	${corelib_DIR}/platform/shared/coreplatformsslcontext.h
	${corelib_DIR}/platform/shared/coreplatformthread.h
	${corelib_DIR}/platform/shared/coreplatformtime.h
	${corelib_DIR}/platform/shared/corerecursivereadwritelock.h
	${corelib_DIR}/platform/corefeatures.h

	${corelib_DIR}/portable/coreattributes.h
	${corelib_DIR}/portable/corebasecodec.h
	${corelib_DIR}/portable/corecomponent.h
	${corelib_DIR}/portable/corecomponentflags.h
	${corelib_DIR}/portable/corecontrollershared.h
	${corelib_DIR}/portable/corecrc.h
	${corelib_DIR}/portable/coredynamiclibrary.h
	${corelib_DIR}/portable/corefile.h
	${corelib_DIR}/portable/corefilename.h
	${corelib_DIR}/portable/corehtmlwriter.h
	${corelib_DIR}/portable/corelogging.h
	${corelib_DIR}/portable/coreparaminfo.h
	${corelib_DIR}/portable/coreparams.h
	${corelib_DIR}/portable/corepersistence.h
	${corelib_DIR}/portable/corepluginmanager.h	
	${corelib_DIR}/portable/corepool.h
	${corelib_DIR}/portable/coresimplereader.h
	${corelib_DIR}/portable/coresingleton.h
	${corelib_DIR}/portable/corestorage.h
	${corelib_DIR}/portable/coretypeinfo.h
	${corelib_DIR}/portable/corevalues.h
	${corelib_DIR}/portable/coreworker.h
	${corelib_DIR}/portable/corexmlwriter.h
	${corelib_DIR}/portable/corezipstream.h

	${corelib_DIR}/system/coreatomic.h
	${corelib_DIR}/system/coreatomicstack.h
	${corelib_DIR}/system/coredebug.h
	${corelib_DIR}/system/coreinterprocess.h
	${corelib_DIR}/system/corethread.h
	${corelib_DIR}/system/coretime.h
	${corelib_DIR}/system/corezipfileformat.h
	${corelib_DIR}/system/corezipfileformat.impl.h

	${corelib_DIR}/text/coreattributehandler.h
	${corelib_DIR}/text/corejsonhandler.h
	${corelib_DIR}/text/coretexthelper.h
	${corelib_DIR}/text/coreutfcodec.h
)

ccl_list_append_once (corelib_network_sources
	${corelib_DIR}/network/corenetwork.cpp
	${corelib_DIR}/network/coresslsocket.cpp
	${corelib_DIR}/network/coreudpconnection.cpp
	${corelib_DIR}/network/coreudpconnection.h
)

ccl_list_append_once (corelib_gui_sources
	${corelib_DIR}/gui/coregesturerecognition.impl.h
	${corelib_DIR}/gui/coreskinformat.impl.h
	${corelib_DIR}/gui/corebitmapprimitives.impl.h
	${corelib_DIR}/gui/corebmphandler.h 
	${corelib_DIR}/gui/corepnghandler.h # not included in api headers to avoid libpng header dependency
	${corelib_DIR}/portable/gui/corealertbox.cpp
	${corelib_DIR}/portable/gui/corebitmap.cpp
	${corelib_DIR}/portable/gui/corecontrols.cpp
	${corelib_DIR}/portable/gui/corefont.cpp
	${corelib_DIR}/portable/gui/coregraphics.cpp
	${corelib_DIR}/portable/gui/corekeyboard.cpp
	${corelib_DIR}/portable/gui/corelistview.cpp
	${corelib_DIR}/portable/gui/corestaticview.cpp
	${corelib_DIR}/portable/gui/coretouchinput.cpp
	${corelib_DIR}/portable/gui/coreview.cpp
	${corelib_DIR}/portable/gui/coreviewbuilder.cpp
	${corelib_DIR}/portable/gui/coreviewshared.cpp
)

ccl_list_append_once (corelib_source_files
	${corelib_DIR}/platform/shared/coreplatformtime.cpp

	${corelib_DIR}/portable/coreattributes.cpp
	${corelib_DIR}/portable/corecomponent.cpp
	${corelib_DIR}/portable/corefile.cpp
	${corelib_DIR}/portable/corefilename.cpp
	${corelib_DIR}/portable/corelogging.cpp
	${corelib_DIR}/portable/coreparams.cpp
	${corelib_DIR}/portable/corepersistence.cpp
	${corelib_DIR}/portable/corepluginmanager.cpp	
	${corelib_DIR}/portable/coresimplereader.cpp
	${corelib_DIR}/portable/coresingleton.cpp
	${corelib_DIR}/portable/corevalues.cpp
	${corelib_DIR}/portable/coreworker.cpp
	${corelib_DIR}/portable/corezipstream.cpp

	${corelib_DIR}/system/coreatomicstack.cpp
	${corelib_DIR}/system/coredebug.cpp

	${corelib_DIR}/text/corejsonhandler.cpp
	${corelib_DIR}/text/coretexthelper.cpp
	${corelib_DIR}/text/coreutfcodec.cpp
	${corelib_DIR}/text/coreutfcodec.h
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL corelib_SOURCE_DIR)
	source_group (TREE ${corelib_DIR} PREFIX "source" FILES ${corelib_source_files} ${corelib_network_sources} ${corelib_gui_sources} ${corelib_api_headers})
	source_group ("public" FILES ${corelib_public_sources} ${corelib_public_headers})
	source_group ("generated" FILES  ${corelib_generated_headers})
	source_group ("source\\core" FILES ${corelib_malloc_sources} ${corelib_newoperator_sources})
	source_group ("cmake" FILES "${CMAKE_CURRENT_LIST_FILE}")
	set (corelib_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

if (NOT TARGET corelib)
	ccl_add_library (corelib STATIC VENDOR ccl)
	set_target_properties (corelib PROPERTIES USE_FOLDERS ON FOLDER "ccl")

	ccl_add_optional (corelib corelib_ENABLE_NETWORK "Network and sockets support." ON
		corelib_network_sources
	)

	ccl_add_optional (corelib corelib_ENABLE_GUI "Build gui components." ON
		corelib_gui_sources
	)

	# collect source files for mandatory features
	ccl_list_append_once (corelib_sources
		${corelib_source_files}
		${corelib_public_sources}
		${CMAKE_CURRENT_LIST_FILE}
	)

	target_link_libraries (corelib PUBLIC ${ZLIB_LIBRARIES} ${PNG_LIBRARIES})
	if (PROJECT_DEBUG_SUFFIX)
		set_target_properties (corelib ${ZLIB_LIBRARIES} ${PNG_LIBRARIES} PROPERTIES DEBUG_POSTFIX ${PROJECT_DEBUG_SUFFIX})
	endif ()
		
	target_sources (corelib PRIVATE ${corelib_sources})
	ccl_target_headers (corelib INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${corelib_DIR} FILES ${corelib_public_headers} ${corelib_api_headers} ${corelib_generated_headers})

	target_include_directories (corelib PUBLIC "$<BUILD_INTERFACE:${corelib_BASEDIR}>" "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")
		
	# Set macro for build products location
	ccl_get_build_output_directory (build_directory)
	cmake_path (RELATIVE_PATH build_directory BASE_DIRECTORY "${REPOSITORY_ROOT}/build" OUTPUT_VARIABLE relative_build_directory)
	target_compile_definitions (corelib PRIVATE ${corelib_COMPILE_DEFINITIONS})
	
	if (CCL_SYSTEM_INSTALL)
		install (TARGETS corelib EXPORT ccl-targets DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}"
								ARCHIVE DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								FRAMEWORK DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								FILE_SET HEADERS DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${corelib_malloc_sources} ${corelib_newoperator_sources} DESTINATION ${corelib_PUBLIC_HEADERS_DESTINATION}/malloc COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:corelib>/corelib$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${corelib_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
else ()
	ccl_include_platform_specifics (corelib)
endif ()

# find additional components
set (corelib_FOUND ON)
set (corelib_corelib_FOUND ON)
foreach (component IN ITEMS ${corelib_FIND_COMPONENTS})
	if ("${component}" STREQUAL corelib)
		continue ()
	endif ()

	set (target ${component})
	set (listfile "${corelib_DIR}/cmake/${component}.cmake")
	if (NOT EXISTS ${listfile})
		if (${CCL_FIND_REQUIRED_${component}})
			message (FATAL_ERROR "corelib::${component} not found.")
		endif ()
		set (corelib_${component}_FOUND OFF)
		set (corelib_FOUND OFF)
	else ()
		include ("${component}")
		set (corelib_${component}_FOUND ON)
		if (NOT "${${component}}" STREQUAL "")
			set (target ${${component}})
		endif ()
		ccl_list_append_once (corelib_LIBRARIES "${target}")
		
		get_target_property (imported ${target} IMPORTED)
		get_target_property (target_type ${target} TYPE)
		if (NOT imported AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
			target_include_directories (${target} PUBLIC "$<BUILD_INTERFACE:${corelib_BASEDIR}>" "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>" PRIVATE "$<BUILD_INTERFACE:${CCL_SUBMODULES_DIR}>")
		else ()
			target_include_directories (${target} INTERFACE "$<BUILD_INTERFACE:${corelib_BASEDIR}>")
		endif ()
	endif ()
endforeach ()

# enable internal debugging if requested
option (DEBUG_INTERNAL "Enable additional internal debugging" ON)

if (DEBUG_INTERNAL)
	add_compile_definitions ($<$<CONFIG:DEBUG>:CORE_DEBUG_INTERNAL=1>)
endif ()

# Add targets
ccl_list_append_once (corelib_LIBRARIES corelib)

# Add include directories
ccl_list_append_once (corelib_INCLUDE_DIRS ${corelib_BASEDIR})
ccl_list_append_once (corelib_INCLUDE_DIRS ${PNG_INCLUDE_DIRS})

# Read corelib version from header file
if ("${corelib_VERSION}" STREQUAL "")
	find_file (corelib_VERSION_FILE NAMES "coreversion.h" HINTS "${corelib_DIR}/public" DOC "corelib version header file.")
	ccl_read_version (corelib "${corelib_VERSION_FILE}" "CORE")
endif ()

# Set result variables
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (corelib
	FOUND_VAR corelib_FOUND
	REQUIRED_VARS corelib_LIBRARIES corelib_INCLUDE_DIRS
	VERSION_VAR corelib_VERSION
	HANDLE_COMPONENTS
)

# make subsequent find_package calls for ccl silent
set (corelib_FIND_QUIETLY ON)
