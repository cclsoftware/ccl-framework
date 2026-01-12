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
// Filename    : ccl/system/threading/threadlocalstorage.h
// Description : Thread Local Storage
//
//************************************************************************************************

#ifndef _ccl_threadlocalstorage_h
#define _ccl_threadlocalstorage_h

#include "core/system/corethread.h"

#include "ccl/public/system/ithreading.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// NativeTLS
//************************************************************************************************

#define NativeTLS Core::Threads::TLS // moved to corelib

//************************************************************************************************
// TLS
//************************************************************************************************

namespace TLS
{
	struct SlotDestructor
	{
		TLSRef slot;
		ThreadLocalDestructor destructor;
		
		SlotDestructor (TLSRef slot = 0, ThreadLocalDestructor destructor = nullptr)
		: slot (slot),
		  destructor (destructor)
		{}

		bool operator == (const SlotDestructor& d) const
		{ return slot == d.slot && destructor == d.destructor; }
	};

	typedef LinkedList<SlotDestructor> DestructorList;

	DestructorList& getDestructorList ();
	CriticalSection& getDestructorListLock ();
	
	TLSRef allocate (ThreadLocalDestructor destructor);
	bool release (TLSRef slot);
	void cleanupOnThreadExit ();
}

} // namespace Threading
} // namespace CCL

#endif // _ccl_threadlocalstorage_h
