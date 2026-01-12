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
// Filename    : ccl/public/base/debug.h
// Description : Debugging
//
//************************************************************************************************

#ifndef _ccl_debug_h
#define _ccl_debug_h

#include "ccl/public/base/platform.h"

namespace CCL {

#undef ASSERT

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCL_DEBUGGER, CCL_NOT_IMPL and ASSERT:
// If the program hits one of these macros, the debugger will be invoked. They are not
// influenced by the DEBUG_LOG macro, because they are considered hard errors that need to 
// be seen at all times. Only in the release build they will be void statements.
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG

#if !defined (DEBUG_LOG)
#define DEBUG_LOG 0
#endif

#define CCL_DEBUGGER(s)		CCL::Debugger::debugBreak (s);
#define CCL_NOT_IMPL(s)		CCL::Debugger::debugBreak (s);
#define CCL_CHECK_HEAP		CCL::Debugger::checkHeap ();

#define ASSERT(e) \
	{ if(!(e)) CCL::Debugger::assertFailed (#e, __FILE__, __LINE__); }

#define SOFT_ASSERT(e,s) \
	{ if(!(e)) CCL::Debugger::printf ("ASSERT FAILED: \"%s\"  "#e"\n", s); }

//////////////////////////////////////////////////////////////////////////////////////////////////
#else // RELEASE
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef DEBUG_LOG
#define DEBUG_LOG 0

#define CCL_DEBUGGER(s)		{}
#define CCL_NOT_IMPL(s)		{}
#define CCL_CHECK_HEAP		{}
#define SOFT_ASSERT(e,s)	{}

#define ASSERT(e)			{}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCL_PRINT macros can be enabled (for a debug build) by defining DEBUG_LOG as 1
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG

#define CCL_PRINT(s)	         CCL::Debugger::print (s);
#define CCL_PRINTLN(s)	         CCL::Debugger::println (s);
#define CCL_PRINTF(s, ...)       CCL::Debugger::printf (s, __VA_ARGS__);

#define CCL_PROFILE_START(s)     double __profile##s = CCL::Debugger::getProfileTime ();
#define CCL_PROFILE_STOP(s)      __profile##s = CCL::Debugger::getProfileTime () - __profile##s; CCL::Debugger::printf (#s " took %f ms\n", (float)__profile##s * 1000.);

#define CCL_INDENT					CCL::Debugger::getIndent ()
#define CCL_ADD_INDENT(numChars)	CCL::Debugger::IndentGuard dbgIndentGuard (numChars);
#define CCL_LOGSCOPE(s)				CCL::Debugger::ScopeLogger dbgScopeLogger (s);
#define CCL_DEBUG_ID(p)				CCL::Debugger::ObjectID (p).str

#else

#define CCL_PRINT(s)			{}
#define CCL_PRINTLN(s)			{}
#define CCL_PRINTF(s, ...)      {}

#define CCL_PROFILE_START(s)	{}
#define CCL_PROFILE_STOP(s)		{}

#define CCL_INDENT
#define CCL_ADD_INDENT(numChars)	{}
#define CCL_LOGSCOPE(s)				{}
#define CCL_DEBUG_ID(p)

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug output in release builds
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_WARN(s, ...) \
	CCL::Debugger::warn (s, __VA_ARGS__);

#define CCL_RPROFILE_START(s) \
	double __profile##s = CCL::Debugger::getProfileTime ();

#define CCL_RPROFILE_STOP(s) \
	__profile##s = CCL::Debugger::getProfileTime () - __profile##s; CCL::Debugger::printf (#s " took %f ms\n", (float)__profile##s * 1000.);

//************************************************************************************************
// Debugger
//************************************************************************************************

class Debugger
{
public:
	// C-String Output
	static void print (CStringPtr string);
	static void printf (CStringPtr format, ...);
	static void println (CStringPtr string);
	static void warn (CStringPtr string, ...);

	// Unicode String Output
	static void print (StringRef string);
	static void println (StringRef string);

	// Breakpoint
	static void debugBreak (CStringPtr string);
	static void assertFailed (CStringPtr expr, CStringPtr file, int line);

	// Profiling
	static double getProfileTime (); // in seconds

	// Memory Debugging
	static bool checkHeap ();

	// Indent for debug output, not used automatically in print (...)
	static void indent (uint32 numChars = 2);
	static void unindent (uint32 numChars = 2);
	static CStringPtr getIndent ();

	struct IndentGuard
	{
		IndentGuard (uint32 numChars = 2);
		~IndentGuard ();
		uint32 numChars;
	};

	struct ScopeLogger
	{
		ScopeLogger (CStringPtr text);
		~ScopeLogger ();
		CStringPtr text;
	};

	// generates a human readable string for a pointer for debug output
	struct ObjectID
	{
		ObjectID (void* obj);
		char str[32];
	};

protected:
	static void reportWarning (CStringPtr string);
};

} // namespace CCL

#endif // _ccl_debug_h
