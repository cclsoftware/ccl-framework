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
// Filename    : ccl/text/strings/stringstats.h
// Description : String Statistics
//
//************************************************************************************************

#ifndef _ccl_stringstats_h
#define _ccl_stringstats_h

#include "ccl/public/base/debug.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// String allocation macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#if 1 // bypass debug heap
#define string_free		(::free)
#define string_malloc	(::malloc)
#define string_realloc	(::realloc)
#else
#define string_free		core_free
#define string_malloc	core_malloc
#define string_realloc	core_realloc
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Statistics macros
//////////////////////////////////////////////////////////////////////////////////////////////////

// CCL_PRINT_STRING_STATS is defined by CMake, see ccltext.cmake
#ifndef CCL_PRINT_STRING_STATS
#define CCL_PRINT_STRING_STATS 1
#endif

#define STRING_STATS (CCL_PRINT_STRING_STATS && DEBUG)

#if STRING_STATS
	#define STRING_ADDED						theStats.stringAdded ();
	#define STRING_REMOVED						theStats.stringRemoved ();
	#define STRING_RESIZED(oldSize, newSize)	theStats.stringResized (oldSize, newSize);
#else
	#define STRING_ADDED
	#define STRING_REMOVED
	#define STRING_RESIZED(oldSize, newSize)
#endif

namespace CCL {

//************************************************************************************************
// StringStatistics
//************************************************************************************************

#if STRING_STATS
template <typename CharType>
struct StringStatistics
{
	const char* title;
	int stringCount;
	int byteCount;	
	int maxStringCount;
	int maxByteCount;
	int maxLength;

	StringStatistics (const char* title)
	: title (title),
	  stringCount (0),
	  byteCount (0),
	  maxStringCount (0),
	  maxByteCount (0),
	  maxLength (0)
	{}

	~StringStatistics ()
	{
		Debugger::printf  ("\n=== %s ===\n", title);
		Debugger::printf  ("maxStringCount = %d\n", maxStringCount);
		Debugger::printf  ("maxByteCount = %d Bytes (%.2lf KB)\n", maxByteCount, (double)maxByteCount / 1024.);
		Debugger::printf  ("maxLength = %d\n", maxLength);
		Debugger::printf  ("averageLength = %.2lf\n", (double)maxByteCount / (double)maxStringCount / sizeof(CharType)); // nonsense
		Debugger::println ("=================================");
	}

	void stringAdded ()
	{
		stringCount++;
		if(stringCount > maxStringCount)
			maxStringCount = stringCount;
	}

	void stringRemoved ()
	{
		stringCount--;
	}

	void stringResized (int oldSize, int newSize)
	{
		byteCount -= oldSize;
		byteCount += newSize;
		if(byteCount > maxByteCount)
			maxByteCount = byteCount;

		int length = newSize / sizeof(CharType);
		if(length > maxLength)
			maxLength = length;
	}
};
#endif // STRING_STATS

} // namespace CCL

#endif // _ccl_stringstats_h
