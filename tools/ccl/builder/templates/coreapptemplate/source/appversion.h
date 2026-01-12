//************************************************************************************************
//
// Application Template
// (Application Copyright)
//
// Filename    : appversion.h
// Description : Application Version
//
//************************************************************************************************

#ifndef _appversion_h
#define _appversion_h

#include "core/public/coremacros.h"

#include "vendor.h"
#include "buildnumber.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Application Template
//////////////////////////////////////////////////////////////////////////////////////////////////

#define APP_ID				"apptemplate"
#define APP_NAME			"Application Template"

#define VER_MAJOR			1
#define VER_MINOR			0
#define VER_REVISION		0
#define VER_BUILD			BUILD_REVISION_NUMBER

#define APP_SHORT_VERSION   STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_REVISION)
#define APP_VERSION         APP_SHORT_VERSION "." BUILD_REVISION_STRING

#define APP_DATE			__DATE__

#define APP_FULL_VERSION	APP_VERSION " " APP_PLATFORM " (Built on " APP_DATE ")"
#define APP_FULL_NAME		APP_NAME " " APP_FULL_VERSION

#define APP_COMPANY			VENDOR_NAME
#define APP_COPYRIGHT		VENDOR_COPYRIGHT
#define APP_WEBSITE			VENDOR_WEBSITE

#define APP_PACKAGE_DOMAIN	VENDOR_PACKAGE_DOMAIN
#define APP_PACKAGE_ID		APP_PACKAGE_DOMAIN "." APP_ID

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif
