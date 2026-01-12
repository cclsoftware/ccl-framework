include_guard (DIRECTORY)

if (TARGET osc)
	set (OSC_LIBRARIES osc)
	return ()
endif ()

ccl_find_path (oscpack_DIR NAMES "osc/OscTypes.h" HINTS "${CCL_SUBMODULES_DIR}/tuio11/oscpack"  DOC "OSC directory")
mark_as_advanced (oscpack_DIR)

add_library (osc INTERFACE)
ccl_include_platform_specifics (osc)

list (APPEND osc_SOURCES
	${oscpack_DIR}/ip/IpEndpointName.cpp
	${oscpack_DIR}/ip/IpEndpointName.h
	${oscpack_DIR}/ip/NetworkingUtils.h
	${oscpack_DIR}/ip/PacketListener.h
	${oscpack_DIR}/ip/TimerListener.h
	${oscpack_DIR}/ip/UdpSocket.h
	${oscpack_DIR}/osc/MessageMappingOscPacketListener.h
	${oscpack_DIR}/osc/OscException.h
	${oscpack_DIR}/osc/OscHostEndianness.h
	${oscpack_DIR}/osc/OscOutboundPacketStream.cpp
	${oscpack_DIR}/osc/OscOutboundPacketStream.h
	${oscpack_DIR}/osc/OscPacketListener.h
	${oscpack_DIR}/osc/OscPrintReceivedElements.cpp
	${oscpack_DIR}/osc/OscPrintReceivedElements.h
	${oscpack_DIR}/osc/OscReceivedElements.cpp
	${oscpack_DIR}/osc/OscReceivedElements.h
	${oscpack_DIR}/osc/OscTypes.cpp
	${oscpack_DIR}/osc/OscTypes.h
)
source_group (TREE ${oscpack_DIR} PREFIX "source" FILES ${osc_SOURCES})

target_sources (osc INTERFACE ${osc_SOURCES})
target_include_directories (osc INTERFACE ${oscpack_DIR})

set (OSC_LIBRARIES osc)
