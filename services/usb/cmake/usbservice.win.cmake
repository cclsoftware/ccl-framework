include_guard (DIRECTORY) 

list (APPEND usbservice_source_files
    ${usb_DIR}/source/win/winusbhidmanager.cpp
	${usb_DIR}/source/win/winusbhidmanager.h
	${usb_DIR}/source/win/winusbhidstatics.cpp
)

list (APPEND usbservice_ccl_sources
	${corelib_DIR}/platform/win/windevicenotificationhandler.h
	${corelib_DIR}/platform/win/windevicenotificationhandler.cpp
)
