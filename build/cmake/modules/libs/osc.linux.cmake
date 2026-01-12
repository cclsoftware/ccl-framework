list (APPEND osc_SOURCES
	${oscpack_DIR}/ip/posix/NetworkingUtils.cpp
	${oscpack_DIR}/ip/posix/UdpSocket.cpp
)

target_compile_definitions (osc INTERFACE OSC_HOST_LITTLE_ENDIAN=1)
