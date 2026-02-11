//************************************************************************************************
//
// CCL Generator
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : appversion.h
// Description : Application Version
//
//************************************************************************************************

#ifndef _appversion_h
#define _appversion_h

#include "ccl/public/base/platform.h"
#include "core/public/coremacros.h"

#include "vendor.h"
#include "buildnumber.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCL Generator
//////////////////////////////////////////////////////////////////////////////////////////////////

#define APP_ID				"cclgenerator"
#define APP_NAME			"CCL Generator"

#define VER_MAJOR			1
#define VER_MINOR			1
#define VER_REVISION		0
#define VER_BUILD			BUILD_REVISION_NUMBER

#define APP_SHORT_VERSION   STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_REVISION)
#define APP_VERSION         APP_SHORT_VERSION "." BUILD_REVISION_STRING

#define APP_DATE			__DATE__
#define APP_PLATFORM		CCL_PLATFORM_STRING

#define APP_FULL_VERSION	APP_VERSION " " APP_PLATFORM " (Built on " APP_DATE ")"
#define APP_FULL_NAME		APP_NAME " " APP_FULL_VERSION

#define APP_COMPANY			VENDOR_NAME
#define APP_COPYRIGHT		VENDOR_COPYRIGHT
#define APP_WEBSITE			VENDOR_WEBSITE

#define APP_PACKAGE_DOMAIN	VENDOR_PACKAGE_DOMAIN
#define APP_PACKAGE_ID		APP_PACKAGE_DOMAIN "." APP_ID

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif
