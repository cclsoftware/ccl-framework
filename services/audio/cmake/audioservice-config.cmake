#************************************************************************************************
#
# Audio Service CMake Configuration
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2026 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
#
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : audioservice-config.cmake
# Description : CMake target for Audio service
#
#************************************************************************************************

include_guard (DIRECTORY)

find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui cclextras-audio cclextras-devices)
find_package (miniaudio REQUIRED)

ccl_find_path (audioservice_DIR NAMES "source/audioservice.h" HINTS "${CMAKE_CURRENT_LIST_DIR}/.." DOC "Audio service directory")

if (TARGET audioservice)
	return ()
endif ()

# Add target
ccl_add_plugin_library (audioservice
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	VERSION_PREFIX PLUG
)

set_target_properties (audioservice PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND audioservice_source_files
	${audioservice_DIR}/source/audioservice.cpp
	${audioservice_DIR}/source/audioservice.h
	${audioservice_DIR}/source/plugmain.cpp
	${audioservice_DIR}/source/plugversion.h
	${audioservice_DIR}/source/base/audiocodecbase.cpp
	${audioservice_DIR}/source/base/audiocodecbase.h
	${audioservice_DIR}/source/base/audiodevicebase.cpp
	${audioservice_DIR}/source/base/audiodevicebase.h
	${audioservice_DIR}/source/base/audiographbase.cpp
	${audioservice_DIR}/source/base/audiographbase.h
	${audioservice_DIR}/source/base/audiometering.cpp
	${audioservice_DIR}/source/base/audiometering.h
	${audioservice_DIR}/source/base/audiopanning.h
	${audioservice_DIR}/source/base/audiopanning.cpp
	${audioservice_DIR}/source/base/audioparams.h
	${audioservice_DIR}/source/base/audioparams.cpp
	${audioservice_DIR}/source/base/audiosourcebase.cpp
	${audioservice_DIR}/source/base/audiosourcebase.h
	${audioservice_DIR}/source/system/miniaudioconverter.cpp
	${audioservice_DIR}/source/system/miniaudioconverter.h
	${audioservice_DIR}/source/system/miniaudiocodec.cpp
	${audioservice_DIR}/source/system/miniaudiocodec.h
	${audioservice_DIR}/source/system/miniaudiodevice.cpp
	${audioservice_DIR}/source/system/miniaudiodevice.h
	${audioservice_DIR}/source/system/miniaudioglue.cpp
	${audioservice_DIR}/source/system/miniaudioglue.h
	${audioservice_DIR}/source/system/miniaudiogenerator.cpp
	${audioservice_DIR}/source/system/miniaudiogenerator.h
	${audioservice_DIR}/source/system/miniaudiograph.cpp
	${audioservice_DIR}/source/system/miniaudiograph.h
	${audioservice_DIR}/source/system/miniaudioresampler.cpp
	${audioservice_DIR}/source/system/miniaudioresampler.h
	${audioservice_DIR}/source/system/miniaudioringbuffer.cpp
	${audioservice_DIR}/source/system/miniaudioringbuffer.h
	${audioservice_DIR}/source/system/miniaudiosources.cpp
	${audioservice_DIR}/source/system/miniaudiosources.h
	${audioservice_DIR}/source/system/miniaudiosystem.cpp
	${audioservice_DIR}/source/system/miniaudiosystem.h
)

list (APPEND audioservice_ccl_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/public/plugins/classfactory.cpp
	${CCL_DIR}/public/plugins/serviceplugin.cpp
)

if ("${VENDOR_PLATFORM}" STREQUAL "wasm")
	list (APPEND audioservice_source_files ${audioservice_DIR}/source/system/miniaudiosystem.wasm.cpp)
elseif (APPLE)
	list (APPEND audioservice_source_files ${audioservice_DIR}/source/system/miniaudiosystem.cocoa.mm)
endif ()

source_group (TREE ${audioservice_DIR}/source PREFIX "source" FILES ${audioservice_source_files})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

list (APPEND audioservice_sources ${audioservice_source_files} ${audioservice_ccl_sources} ${CMAKE_CURRENT_LIST_FILE})

ccl_add_resources (audioservice ${audioservice_resources})
target_sources (audioservice PRIVATE ${audioservice_sources})
target_link_libraries (audioservice PRIVATE cclapp cclbase ccltext cclsystem cclgui cclextras-audio cclextras-devices ${miniaudio_LIBRARY})

if (APPLE)
	# audio capture permission (iOS gets AVFoundation via miniaudio already)
	target_link_libraries (audioservice PRIVATE "-framework AVFoundation")
endif ()

# Frames per node bus cache of the miniaudio node graph (default 480): the largest chunk the nodes are processed in.
# Used by miniaudio.c (compiled into this target) and MiniAudioGraph::kMaxChunkFrames.
target_compile_definitions (audioservice PRIVATE MA_DEFAULT_NODE_CACHE_CAP_IN_FRAMES_PER_BUS=512)

ccl_export_target (audioservice)
if (CCL_SYSTEM_INSTALL)
	install (TARGETS audioservice
		EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
		LIBRARY DESTINATION "${CCL_PLUGINS_DESTINATION}"
		COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX}
	)
    if (WIN32)
        install (FILES $<TARGET_FILE_DIR:audioservice>/audioservice.pdb DESTINATION "${CCL_PLUGINS_DESTINATION}" OPTIONAL COMPONENT services_${VENDOR_NATIVE_COMPONENT_SUFFIX})
    endif ()
elseif (VENDOR_PLUGINS_RUNTIME_DIRECTORY)
	install (TARGETS audioservice LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY} FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY}
								FRAMEWORK DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})
endif ()
