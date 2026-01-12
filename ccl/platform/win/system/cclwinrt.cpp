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
// Filename    : ccl/platform/win/system/cclwinrt.cpp
// Description : Windows Runtime (WinRT) Integration
//
//************************************************************************************************

#include "ccl/platform/win/system/cclwinrt.h"
#include "ccl/platform/win/system/cclcoinit.h"

using namespace CCL;
using namespace WinRT;

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult System::CoWinRTInitialize ()
{
	return static_cast<tresult> (GetWinRTPlatform ().initialize ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void System::CoWinRTUninitialize ()
{
	GetWinRTPlatform ().uninitialize ();
}
