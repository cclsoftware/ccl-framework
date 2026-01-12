
install (FILES
	"${CCL_DIR}/platform/linux/linuxmain.cpp"
	"${CCL_DIR}/platform/linux/linuxconsolemain.cpp"
	"${CCL_DIR}/platform/linux/linuxplatform.h"
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/platform/linux"
	COMPONENT public_headers_${VENDOR_NATIVE_COMPONENT_SUFFIX}
)
