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
// Filename    : ccl/public/base/debug.cpp
// Description : Debugging
//
//************************************************************************************************

#include "ccl/public/base/debug.h"

#include "core/public/corestringbuffer.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL 
{ 
	static Core::CString256 debugIndent; 
	static const int kPrintfBufferSize = STRING_STACK_SPACE_MAX;
}

using namespace CCL;

//************************************************************************************************
// Debugger::IndentGuard
//************************************************************************************************

Debugger::IndentGuard::IndentGuard (uint32 numChars)
: numChars (numChars)
{
	Debugger::indent (numChars);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

Debugger::IndentGuard::~IndentGuard ()
{
	Debugger::unindent (numChars);
}

//************************************************************************************************
// Debugger::ScopeLogger
//************************************************************************************************

Debugger::ScopeLogger::ScopeLogger (CStringPtr text)
: text (text)
{
	Debugger::printf ("%sBegin %s\n", Debugger::getIndent (), text);
	Debugger::indent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Debugger::ScopeLogger::~ScopeLogger ()
{
	Debugger::unindent ();
	Debugger::printf ("%sEnd   %s\n", Debugger::getIndent (), text);
}

//************************************************************************************************
// Debugger::ObjectID
//************************************************************************************************

Debugger::ObjectID::ObjectID (void* obj)
{
	int i = 0;
	int64 x = (int64)obj;
	while(x > 0)
	{
		str[i] = (i % 2) ?  char ('a' + (x % ('z' - 'a'))) :  char ('A' + (x % ('Z' - 'A')));
		i++;
		x = x / 16;
	}
	if(i==0)
		snprintf (str, sizeof(str), "%x", (int32)((int64)obj));
	else
		str[i]=0;
}

//************************************************************************************************
// Debugger
//************************************************************************************************

void Debugger::printf (CStringPtr format, ...)
{
	va_list marker;
	va_start (marker, format);

	char string[kPrintfBufferSize];
	::vsnprintf (string, kPrintfBufferSize-1, format, marker);
	string[kPrintfBufferSize-1] = 0;

	print (string);
	va_end (marker);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::println (CStringPtr string)
{
	print (string);
	print ("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::println (StringRef string)
{
	print (string);
	print ("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::warn (CStringPtr format, ...)
{
	va_list marker;
	va_start (marker, format);

	char string[kPrintfBufferSize];
	::vsnprintf (string, kPrintfBufferSize-1, format, marker);
	string[kPrintfBufferSize-1] = 0;

	printf ("### Warning: %s", string);

	// remove line breaks
	int length = (int)::strlen (string);
	while(length > 0 && string[length-1] == '\n')
		string[length-1] = 0,
		length--;

	reportWarning (string);
	va_end (marker);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::assertFailed (CStringPtr expr, CStringPtr file, int line)
{
	printf ("\n ### ASSERT FAILED: %s\n%s(%d)\n", expr, file, line);
	
	debugBreak ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Debugger::checkHeap ()
{
	#if CORE_MALLOC_ENABLED
	if(::core_check_heap () == 0)
	{
		debugBreak ("Heap check failed!");
		return false;
	}
	#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::indent (uint32 numChars)
{
	for(uint32 i = 0; i < numChars; i++)
		debugIndent.append (" ");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Debugger::unindent (uint32 numChars)
{
	debugIndent.truncate (debugIndent.length () - numChars);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Debugger::getIndent ()
{
	return debugIndent;
}
