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
// Filename    : ccl/platform/win/system/debug.win.cpp
// Description : Platform-specific Debugger stuff
//
//************************************************************************************************

#include "ccl/public/systemservices.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/platform/win/cclwindows.h"

#include "core/system/coredebug.h"
#include "core/system/coretime.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Debugging APIs
//////////////////////////////////////////////////////////////////////////////////////////////////
 
CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugPrintCString) (CStringPtr string)
{
	::OutputDebugStringA (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugPrintString) (StringRef string)
{
	::OutputDebugStringW (StringChars (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT double CCL_API System::CCL_ISOLATED (GetProfileTime) ()
{
	return Core::SystemClock::getSeconds ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT int64 CCL_API System::CCL_ISOLATED (GetSystemTicks) ()
{
	return Core::SystemClock::getMilliseconds ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugBreakPoint) ()
{
	if(::IsDebuggerPresent ()) // do not crash if not being debugged!
		::DebugBreak ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugExitProcess) (int exitCode)
{
	if(::IsDebuggerPresent ())
		::TerminateProcess (::GetCurrentProcess (), exitCode);
}
