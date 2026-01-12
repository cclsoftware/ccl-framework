
install (FILES
	"${CCL_DIR}/platform/cocoa/cocoamain.mm"
	"${CCL_DIR}/platform/cocoa/cocoaconsolemain.mm"
	"${CCL_DIR}/platform/cocoa/cclcocoa.h"
	DESTINATION "${CCL_PUBLIC_HEADERS_DESTINATION}/platform/cocoa"
	COMPONENT public_headers
)