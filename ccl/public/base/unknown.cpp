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
// Filename    : ccl/public/base/unknown.cpp
// Description : Unknown class
//
//************************************************************************************************

/*
	COM object destructors are very sensitive functions
	https://devblogs.microsoft.com/oldnewthing/20050927-13/?p=34023

	Avoiding double-destruction when an object is released
	https://devblogs.microsoft.com/oldnewthing/20050928-10/?p=34013

	If there's already a bug, it's not surprising that there's a possibility for error
	https://devblogs.microsoft.com/oldnewthing/20081103-00/?p=20353
*/

#include "ccl/public/base/unknown.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Unknown
//************************************************************************************************

tresult CCL_API Unknown::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_UNKNOWN (IUnknown)
	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int Unknown::getRetainCount () const
{ 
	return retainCount; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API Unknown::retain ()
{
	#if CCL_DEBUG_INTERNAL
	// The last two will trigger when an already released object is retained
	if((debugFlags & kDebugFlagRetain) != 0 || debugFlags < 0 || debugFlags > 1000)
		Debugger::debugBreak ("retain\n");
	#endif

	ASSERT (retainCount > 0) // it is illegal to increment from 0 to 1
	ASSERT (retainCount != 0xDDDDDDDD) // pattern marking freed memory in debug builds

	AtomicAddInline ((int32&)retainCount, 1);
	return retainCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API Unknown::release ()
{ 
	#if CCL_DEBUG_INTERNAL
	// The last two will trigger when an already released object is released
	if((debugFlags & kDebugFlagRetain) != 0 || debugFlags < 0 || debugFlags > 1000)  
		Debugger::debugBreak ("release\n"); 
	#endif

	ASSERT (retainCount != 0) // release after stack object destruction
	ASSERT (retainCount != 0xDDDDDDDD) // memory has been freed already

	if(AtomicAddInline ((int32&)retainCount, -1) == 1)
	{
		#if RELEASE
		retainCount = 0xDEADBEEF;
		#endif
		delete this;
		return 0;
	}
	return retainCount;
}
