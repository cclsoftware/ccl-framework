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
// Filename    : ccl/platform/cocoa/system/debug.cocoa.mm
// Description : Platform-specific Debugger stuff
//
//************************************************************************************************

#include "ccl/public/systemservices.h"
#include "ccl/public/text/cstring.h"

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

#include "core/system/coredebug.h"
#include "core/system/coretime.h"

using namespace CCL;

static bool AmIBeingDebugged ();

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
// http://developer.apple.com/mac/library/qa/qa2004/qa1361.html
//////////////////////////////////////////////////////////////////////////////////////////////////
	
static bool AmIBeingDebugged ()
{
    int junk;
    int  mib[4];
    struct kinfo_proc info;
    size_t size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid ();

    // Call sysctl.

    size = sizeof(info);
    junk = sysctl (mib, sizeof(mib) / sizeof(*mib), &info, &size, nullptr, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.

    return ((info.kp_proc.p_flag & P_TRACED) != 0 );
}
