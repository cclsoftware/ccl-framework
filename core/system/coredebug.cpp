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
// Filename    : core/system/coredebug.cpp
// Description : Debugging Functions
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_DEBUG_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coredebug)
#elif CORE_DEBUG_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coredebug)
#elif CORE_DEBUG_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coredebug.posix.h"
#else
	// Implementation provided by the library user
	#include "core/platform/shared/coreplatformdebug.h"
#endif

#include "core/system/coredebug.h"

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

void Core::DebugPrint (CStringPtr string)
{ 
	Platform::Debug::print (string); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Core::DebugPrintf (CStringPtr format, ...)
{
	va_list marker;
	va_start (marker, format);

	const int kPrintfBufferSize = STRING_STACK_SPACE_MAX; 
	char string[kPrintfBufferSize];
	::vsnprintf (string, kPrintfBufferSize-1, format, marker);
	string[kPrintfBufferSize-1] = 0;

	DebugPrint (string);
	va_end (marker);
}
