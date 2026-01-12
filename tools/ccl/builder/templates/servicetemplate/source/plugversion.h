//************************************************************************************************
//
// (Service Template)
// (Service Copyright)
//
// Filename    : plugversion.h
// Description : Plug-In Version
//
//************************************************************************************************

#ifndef _plugversion_h
#define _plugversion_h

#include "ccl/public/base/platform.h"
#include "core/public/coremacros.h"

#include "vendor.h"
#include "buildnumber.h" // BUILD_REVISION_NUMBER

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_ID				"servicetemplate"
#define PLUG_PACKAGE_ID		VENDOR_PACKAGE_DOMAIN "." PLUG_ID
#define PLUG_NAME			"(Service Template)"

#define PLUG_VER_MAJOR		1
#define PLUG_VER_MINOR		0
#define PLUG_VER_REVISION	0
#define PLUG_VER_BUILD		BUILD_REVISION_NUMBER

#define PLUG_SHORT_VERSION	STRINGIFY(PLUG_VER_MAJOR) "." STRINGIFY(PLUG_VER_MINOR) "." STRINGIFY(PLUG_VER_REVISION)
#define PLUG_VERSION		PLUG_SHORT_VERSION "." BUILD_REVISION_STRING

#define PLUG_COMPANY		VENDOR_NAME
#define PLUG_COPYRIGHT		VENDOR_COPYRIGHT
#define PLUG_WEBSITE		VENDOR_WEBSITE

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _plugversion_h
