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
// Filename    : ccl/system/threading/threadlocalstorage.cpp
// Description : Thread Local Storage
//
//************************************************************************************************

#include "ccl/system/threading/threadlocalstorage.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT TLSRef CCL_API System::CCL_ISOLATED (CreateThreadLocalSlot) (ThreadLocalDestructor destructor)
{
	return TLS::allocate (destructor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (DestroyThreadLocalSlot) (TLSRef slot)
{
	return TLS::release (slot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (SetThreadLocalData) (TLSRef slot, void* data)
{
	return NativeTLS::setValue (slot, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* CCL_API System::CCL_ISOLATED (GetThreadLocalData) (TLSRef slot)
{
	return NativeTLS::getValue (slot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (CleanupThreadLocalStorage) ()
{
	TLS::cleanupOnThreadExit ();
}

//************************************************************************************************
// TLS
//************************************************************************************************

TLS::DestructorList& TLS::getDestructorList ()
{
	static DestructorList destructorList;
	return destructorList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CriticalSection& TLS::getDestructorListLock ()
{
	static CriticalSection destructorListLock;
	return destructorListLock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TLSRef TLS::allocate (ThreadLocalDestructor destructor)
{
	TLSRef slot = NativeTLS::allocate ();
	if(slot && destructor)
	{
		ScopedLock scopedLock (getDestructorListLock ());
		getDestructorList ().append (SlotDestructor (slot, destructor));
	}
	return slot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::release (TLSRef slot)
{
	// remove from destructor list first...
	{
		ScopedLock scopedLock (getDestructorListLock ());
		ListForEach (getDestructorList (), SlotDestructor, d)
			if(d.slot == slot)
			{
				// handle destructor for calling thread
				void* value = NativeTLS::getValue (slot);
				if(value)
					d.destructor (value);
					
				getDestructorList ().remove (d);
				break;
			}
		EndFor
	}

	return NativeTLS::release (slot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TLS::cleanupOnThreadExit ()
{
	// call destructor for current thread's local data...
	ScopedLock scopedLock (getDestructorListLock ());
	ListForEach (getDestructorList (), SlotDestructor, d)
		void* value = NativeTLS::getValue (d.slot);
		if(value)
			d.destructor (value);
	EndFor
}
