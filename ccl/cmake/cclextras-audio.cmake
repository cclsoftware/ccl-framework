#************************************************************************************************
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
# Filename    : cclextras-audio.cmake
# Description : CCL extras audio
#
#************************************************************************************************

include_guard (DIRECTORY)

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# collect source files
ccl_list_append_once (cclextras_audio_headers
	${CCL_DIR}/extras/audio/audiocomponent.h
	${CCL_DIR}/extras/audio/audiocurves.h
	${CCL_DIR}/extras/audio/audiodatabuffer.h
	${CCL_DIR}/extras/audio/audiofileio.h
	${CCL_DIR}/extras/audio/audioformatter.h
	${CCL_DIR}/extras/audio/audioprocessor.h
	${CCL_DIR}/extras/audio/audioscratchbuffer.h
	${CCL_DIR}/extras/audio/audiostreamlayout.h
	${CCL_DIR}/extras/audio/cclaudiodataops.h
	${CCL_DIR}/extras/audio/meter/audiometerbuffer.h
	${CCL_DIR}/extras/audio/meter/audiometercomponent.h
	${CCL_DIR}/extras/audio/meter/audiometerview.h
	${CCL_DIR}/extras/audio/runtime/audiodevicesession.h
	${CCL_DIR}/extras/audio/runtime/audiodevicesetup.h
	${CCL_DIR}/extras/audio/runtime/audioplayer.h
	${CCL_DIR}/extras/audio/runtime/audioplayercomponent.h
	${CCL_DIR}/extras/audio/runtime/audiorecorder.h
	${CCL_DIR}/extras/audio/runtime/audiorecordercomponent.h
	${CCL_DIR}/extras/audio/runtime/audiotransport.h
	${CCL_DIR}/extras/audio/runtime/audiotransportcomponent.h
	${CCL_DIR}/extras/audio/waveform/audiolivepeakbuilder.h
	${CCL_DIR}/extras/audio/waveform/audiopeakbuffer.h
	${CCL_DIR}/extras/audio/waveform/audiopeakfile.h
	${CCL_DIR}/extras/audio/waveform/audiopeakpainter.h
	${CCL_DIR}/extras/audio/waveform/audiowaveform.h
	${CCL_DIR}/extras/audio/waveform/audiowaveformview.h
)

ccl_list_append_once (cclextras_audio_sources
	${CCL_DIR}/extras/audio/audiocomponent.cpp
	${CCL_DIR}/extras/audio/audiocurves.cpp
	${CCL_DIR}/extras/audio/audiodatabuffer.cpp
	${CCL_DIR}/extras/audio/audiofileio.cpp
	${CCL_DIR}/extras/audio/audioformatter.cpp
	${CCL_DIR}/extras/audio/audioprocessor.cpp
	${CCL_DIR}/extras/audio/audioscratchbuffer.cpp
	${CCL_DIR}/extras/audio/audiostreamlayout.cpp
	${CCL_DIR}/extras/audio/meter/audiometerbuffer.cpp
	${CCL_DIR}/extras/audio/meter/audiometercomponent.cpp
	${CCL_DIR}/extras/audio/meter/audiometerview.cpp
	${CCL_DIR}/extras/audio/runtime/audiodevicesession.cpp
	${CCL_DIR}/extras/audio/runtime/audiodevicesetup.cpp
	${CCL_DIR}/extras/audio/runtime/audioplayer.cpp
	${CCL_DIR}/extras/audio/runtime/audioplayercomponent.cpp
	${CCL_DIR}/extras/audio/runtime/audiorecorder.cpp
	${CCL_DIR}/extras/audio/runtime/audiorecordercomponent.cpp
	${CCL_DIR}/extras/audio/runtime/audiotransport.cpp
	${CCL_DIR}/extras/audio/runtime/audiotransportcomponent.cpp
	${CCL_DIR}/extras/audio/waveform/audiolivepeakbuilder.cpp
	${CCL_DIR}/extras/audio/waveform/audiopeakbuffer.cpp
	${CCL_DIR}/extras/audio/waveform/audiopeakfile.cpp
	${CCL_DIR}/extras/audio/waveform/audiopeakpainter.cpp
	${CCL_DIR}/extras/audio/waveform/audiowaveform.cpp
	${CCL_DIR}/extras/audio/waveform/audiowaveformview.cpp
)

# Add targets
if (NOT TARGET cclextras-audio)
	ccl_add_library (cclextras-audio STATIC)

	source_group (TREE ${CCL_DIR}/extras/audio PREFIX "source" FILES ${cclextras_audio_sources} ${cclextras_audio_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

	target_sources (cclextras-audio PRIVATE ${cclextras_audio_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (cclextras-audio INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclextras_audio_headers})

	if (CCL_EXPORTS_PATH)
		install (TARGETS cclextras-audio EXPORT ccl-targets FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers)
	elseif (CCL_SYSTEM_INSTALL)
		install (TARGETS cclextras-audio EXPORT ccl-targets DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}"
										 ARCHIVE DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FRAMEWORK DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
										 FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclextras_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:cclextras-audio>/cclextras-audio$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()
