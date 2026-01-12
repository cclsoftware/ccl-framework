//************************************************************************************************
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
// Filename    : ccl/platform/linux/cclgui-xdgportal/version.h
// Description : CCL GUI Integration using XDG Portal
//
//************************************************************************************************

#ifndef _xdgportal_version_h
#define _xdgportal_version_h

#include "ccl/public/cclversion.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_ID					"cclgui-xdgportal"
#define PLUG_NAME				"CCL GUI Integration using XDG Portal"

#define PLUG_COMPANY			CCL_AUTHOR_NAME
#define PLUG_COPYRIGHT			CCL_AUTHOR_COPYRIGHT
#define PLUG_WEBSITE			CCL_PRODUCT_WEBSITE

#define PLUG_VER_MAJOR			1
#define PLUG_VER_MINOR			0
#define PLUG_VER_REVISION		0
#define PLUG_VER_BUILD			BUILD_REVISION_NUMBER

#define PLUG_SHORT_VERSION		STRINGIFY(PLUG_VER_MAJOR) "." STRINGIFY(PLUG_VER_MINOR) "." STRINGIFY(PLUG_VER_REVISION)
#define PLUG_VERSION			PLUG_SHORT_VERSION "." BUILD_REVISION_STRING

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _xdgportal_version_h
