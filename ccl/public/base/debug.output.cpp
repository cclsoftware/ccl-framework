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
// Filename    : ccl/public/base/debug.output.cpp
// Description : Debugger Output
//
//************************************************************************************************

#include "ccl/public/base/debug.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Debugger
//************************************************************************************************

static bool gSuppressDebugBreak = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::print (CStringPtr string)
{
	System::DebugPrintCString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::print (StringRef string)
{
	System::DebugPrintString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::debugBreak (CStringPtr string)
{
	if(gSuppressDebugBreak == false)
	{
		System::DebugPrintString (string);
		System::DebugBreakPoint ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::reportWarning (CStringPtr string)
{
	System::DebugReportWarning (System::GetCurrentModuleRef (), String (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Debugger::getProfileTime ()
{
	return System::GetProfileTime ();
}
