//************************************************************************************************
//
// Plug-In Template
// (Plug-In Copyright)
//
// Filename    : plugversion.h
// Description : Plug-In Version
//
//************************************************************************************************

#ifndef _plugversion_h
#define _plugversion_h

#include "core/public/coremacros.h"

#include "vendor.h"
#include "buildnumber.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-In Template
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_ID				"plugintemplate"
#define PLUG_NAME			"Plug-In Template"

#define VER_MAJOR			1
#define VER_MINOR			0
#define VER_REVISION		0
#define VER_BUILD			BUILD_REVISION_NUMBER

#define PLUG_SHORT_VERSION	STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_REVISION)
#define PLUG_VERSION		PLUG_SHORT_VERSION "." BUILD_REVISION_STRING

#define PLUG_DATE			__DATE__

#define PLUG_FULL_VERSION	PLUG_VERSION " " PLUG_PLATFORM " (Built on " PLUG_DATE ")"
#define PLUG_FULL_NAME		PLUG_NAME " " PLUG_FULL_VERSION

#define PLUG_COMPANY		VENDOR_NAME
#define PLUG_COPYRIGHT		VENDOR_COPYRIGHT
#define PLUG_WEBSITE		VENDOR_WEBSITE

#define PLUG_PACKAGE_DOMAIN	VENDOR_PACKAGE_DOMAIN
#define PLUG_PACKAGE_ID		PLUG_PACKAGE_DOMAIN "." PLUG_ID

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _plugversion_h
