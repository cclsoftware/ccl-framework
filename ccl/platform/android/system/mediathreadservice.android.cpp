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
// Filename    : ccl/platform/android/system/mediathreadservice.android.cpp
// Description : Android Multimedia Threading Services
//
//************************************************************************************************

#include "ccl/system/threading/mediathreadservice.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IMediaThreadService& CCL_API System::CCL_ISOLATED (GetMediaThreadService) ()
{
	static MediaThreadService theMediaThreadService;
	return theMediaThreadService;
}
