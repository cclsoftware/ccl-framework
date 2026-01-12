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
// Filename    : ccl/platform/linux/gui/webbrowserview.linux.cpp
// Description : Linux Web Browser View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/nativewindow.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

bool NativeWebControl::isAvailable ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWebControl* NativeWebControl::createInstance (WebBrowserView& owner)
{
	return nullptr;
}
