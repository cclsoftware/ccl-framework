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
// Filename    : core/public/coreversion.h
// Description : Version Information
//
//************************************************************************************************

#ifndef _coreversion_h
#define _coreversion_h

#include "buildtime.h"		// UNIX_BUILD_TIME and CURRENT_YEAR
#include "buildnumber.h"	// BUILD_REVISION_NUMBER and BUILD_REVISION_STRING

#include "core/public/corebasicmacros.h" // STRINGIFY

//////////////////////////////////////////////////////////////////////////////////////////////////
// Version
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CORE_VERSION_MAJOR		5
#define CORE_VERSION_MINOR		0
#define CORE_VERSION_REVISION	0
#define CORE_VERSION_BUILD		BUILD_REVISION_NUMBER

#define CORE_VERSION_STRING		STRINGIFY(CORE_VERSION_MAJOR) "." STRINGIFY(CORE_VERSION_MINOR) "." STRINGIFY(CORE_VERSION_REVISION)
#define CORE_BUILD_TIMESTAMP	"Built on " __DATE__ " " __TIME__ " (Rev. " BUILD_REVISION_STRING ")"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Legal Information
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CORE_AUTHOR_NAME		"CCL Software Licensing GmbH"
#define CORE_COPYRIGHT_YEAR		CURRENT_YEAR
#define CORE_AUTHOR_COPYRIGHT	"Copyright (c) " CORE_COPYRIGHT_YEAR " " CORE_AUTHOR_NAME

#endif // _coreversion_h
