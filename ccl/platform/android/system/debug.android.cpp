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
// Filename    : ccl/platform/android/system/debug.android.cpp
// Description : Platform-specific Debugger stuff
//
//************************************************************************************************

#include "ccl/public/systemservices.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "core/system/coredebug.h"
#include "core/system/coretime.h"

#include <fcntl.h>
#include <unistd.h>

using namespace CCL;

static bool AmIBeingDebugged (void);

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Debugging APIs
//////////////////////////////////////////////////////////////////////////////////////////////////
 
CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugPrintCString) (CStringPtr string)
{
	Core::DebugPrint (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugPrintString) (StringRef string)
{
	MutableCString cString (string, Text::kUTF8);
	Core::DebugPrint (cString);
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
	if(AmIBeingDebugged ()) // do not crash if not being debugged!
		ASSERT (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugExitProcess) (int exitCode)
{
	if(AmIBeingDebugged ())
		::exit (exitCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool AmIBeingDebugged (void)
{
	int statusFd = ::open ("/proc/self/status", O_RDONLY);
	if(statusFd < 0)
		return false;

	char buffer[STRING_STACK_SPACE_MAX];
	ssize_t bytesRead = ::read (statusFd, buffer, STRING_STACK_SPACE_MAX - 1);
	if(bytesRead <= 0)
		return false;
	buffer[bytesRead] = '\0';

	String content;
	content.appendCString (Text::kSystemEncoding, buffer);

	int position = content.index ("TracerPid:");
	if(position >= 0)
	{
		int64 value;
		if(content.subString (position + 10, 20).trimWhitespace ().getIntValue (value) && value > 0)
			return true;
	}
	return false;
}
