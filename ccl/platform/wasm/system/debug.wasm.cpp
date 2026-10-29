//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/system/debug.wasm.cpp
// Description : Platform-specific Debugger stuff
//
//************************************************************************************************

#include "ccl/public/systemservices.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "core/system/coredebug.h"
#include "core/system/coretime.h"

using namespace CCL;

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
	emscripten_debugger ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugExitProcess) (int exitCode)
{
	// TODO
}
