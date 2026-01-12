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
// Filename    : ccl/public/system/threadlocal.h
// Description : Thread Local Storage
//
//************************************************************************************************

#ifndef _ccl_threadlocal_h
#define _ccl_threadlocal_h

#include "ccl/public/systemservices.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// ThreadLocal
/** \ingroup ccl_system */
//************************************************************************************************

template <class T>
class ThreadLocal
{
public:
	ThreadLocal (ThreadLocalDestructor destructor = nullptr)
	: slot (System::CreateThreadLocalSlot (destructor)),
	  destructor (destructor)
	{}
	
	ThreadLocal (const ThreadLocal& other)
	: slot (System::CreateThreadLocalSlot (other.destructor)),
	  destructor (other.destructor)
	{ set (other.get ()); }

	~ThreadLocal ()
	{
		System::DestroyThreadLocalSlot (slot);
	}
	
	T get () const
	{
		return (T)System::GetThreadLocalData (slot);
	}
	
	bool set (T value)
	{
		return System::SetThreadLocalData (slot, (void*)value) != 0;
	}
	
	ThreadLocal& operator = (const ThreadLocal& other)
	{ set (other.get ()); return *this; }

protected:
	TLSRef slot;
	ThreadLocalDestructor destructor;
};

//************************************************************************************************
// ThreadSingleton
/** \ingroup ccl_system */
//************************************************************************************************

template <class T>
class ThreadSingleton
{
public:
	virtual ~ThreadSingleton () {}

	static T& instance ()
	{
		T* localInstance = theInstance.get ();
		if(!localInstance)
		{
			localInstance = NEW T;
			theInstance.set (localInstance);
		}
		return *localInstance;
	}

private:
	static ThreadLocal<T*> theInstance;
	
	static void CCL_API destructor (void* data)
	{
		T* localInstance = (T*)data;
		delete localInstance;
		theInstance.set (nullptr);
	}
};

#define DEFINE_THREAD_SINGLETON(ClassName) \
namespace CCL { namespace Threading { \
template<> ThreadLocal<ClassName*> \
ThreadSingleton<ClassName>::theInstance (ThreadSingleton<ClassName>::destructor); }}

} // namespace Threading
} // namespace CCL

#endif // _ccl_threadlocal_h
