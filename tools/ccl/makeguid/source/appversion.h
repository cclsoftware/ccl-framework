//************************************************************************************************
//
// CCL GUID Generator
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
// Description : Version Information
//
//************************************************************************************************

#ifndef _appversion_h
#define _appversion_h

#include "ccl/public/cclversion.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define APP_ID				"cclmakeguid"
#define APP_NAME			"CCL GUID Generator"

#define VER_MAJOR			1
#define VER_MINOR			0
#define VER_REVISION		0
#define VER_BUILD			BUILD_REVISION_NUMBER

#define APP_SHORT_VERSION   STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_REVISION)
#define APP_VERSION         APP_SHORT_VERSION "." BUILD_REVISION_STRING

#define APP_DATE			__DATE__
#define APP_PLATFORM		CCL_PLATFORM_STRING

#define APP_FULL_VERSION	APP_VERSION " " APP_PLATFORM " (Built on " APP_DATE ")"
#define APP_FULL_NAME		APP_NAME " " APP_FULL_VERSION

#define APP_COPYRIGHT		CCL_AUTHOR_COPYRIGHT

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif
