list (APPEND osc_SOURCES
	${oscpack_DIR}/ip/win32/NetworkingUtils.cpp
	${oscpack_DIR}/ip/win32/UdpSocket.cpp
)

target_link_libraries (osc INTERFACE Ws2_32 Winmm)