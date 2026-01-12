include_guard (DIRECTORY)

if (TARGET tuio)
	list (APPEND TUIO_LIBRARIES tuio)
	return ()
endif ()

find_package (OSC)

ccl_find_path (tuio_DIR NAMES "TuioListener.h" HINTS "${CCL_SUBMODULES_DIR}/tuio11/TUIO" CACHE PATH "TUIO directory")
mark_as_advanced (tuio_DIR)

ccl_add_library (tuio SHARED)

list (APPEND tuio_SOURCES
    ${tuio_DIR}/FlashSender.cpp
	${tuio_DIR}/FlashSender.h
	${tuio_DIR}/LibExport.h
	${tuio_DIR}/OneEuroFilter.cpp
	${tuio_DIR}/OneEuroFilter.h
	${tuio_DIR}/OscReceiver.cpp
	${tuio_DIR}/OscReceiver.h
	${tuio_DIR}/OscSender.h
	${tuio_DIR}/TcpReceiver.cpp
	${tuio_DIR}/TcpReceiver.h
	${tuio_DIR}/TcpSender.cpp
	${tuio_DIR}/TcpSender.h
	${tuio_DIR}/TuioBlob.cpp
	${tuio_DIR}/TuioBlob.h
	${tuio_DIR}/TuioClient.cpp
	${tuio_DIR}/TuioClient.h
	${tuio_DIR}/TuioContainer.cpp
	${tuio_DIR}/TuioContainer.h
	${tuio_DIR}/TuioCursor.cpp
	${tuio_DIR}/TuioCursor.h
	${tuio_DIR}/TuioDispatcher.cpp
	${tuio_DIR}/TuioDispatcher.h
	${tuio_DIR}/TuioListener.h
	${tuio_DIR}/TuioManager.cpp
	${tuio_DIR}/TuioManager.h
	${tuio_DIR}/TuioObject.cpp
	${tuio_DIR}/TuioObject.h
	${tuio_DIR}/TuioPoint.cpp
	${tuio_DIR}/TuioPoint.h
	${tuio_DIR}/TuioServer.cpp
	${tuio_DIR}/TuioServer.h
	${tuio_DIR}/TuioTime.cpp
	${tuio_DIR}/TuioTime.h
	${tuio_DIR}/UdpReceiver.cpp
	${tuio_DIR}/UdpReceiver.h
	${tuio_DIR}/UdpSender.cpp
	${tuio_DIR}/UdpSender.h
	${tuio_DIR}/WebSockSender.cpp
	${tuio_DIR}/WebSockSender.h
)
source_group ("source" FILES ${tuio_SOURCES})

target_sources (tuio PRIVATE ${tuio_SOURCES})
target_include_directories (tuio PUBLIC ${tuio_DIR} ${oscpack_DIR})
target_link_libraries (tuio PRIVATE ${OSC_LIBRARIES})
target_compile_definitions (tuio PUBLIC "LIB_EXPORT")

set_target_properties (tuio PROPERTIES USE_FOLDERS ON FOLDER "services/ccl")

list (APPEND TUIO_LIBRARIES tuio)

