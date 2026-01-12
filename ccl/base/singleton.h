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
// Filename    : ccl/base/singleton.h
// Description : Singleton templates
//
//************************************************************************************************

#ifndef _ccl_singleton_h
#define _ccl_singleton_h

#include "ccl/base/object.h"

namespace CCL {

//************************************************************************************************
// StaticSingleton
/** Singleton instance using automatic variable. */
//************************************************************************************************

template <class T>
class StaticSingleton
{
public:
	static T& instance ()
	{
		static T theInstance;
		return theInstance;
	}
};

//************************************************************************************************
// Singleton
/** Singleton instance, destroyed automatically on exit. */
//************************************************************************************************

template <class T>
class Singleton
{
public:
	virtual ~Singleton ()
	{
		if((Singleton*)theInstance == this)
			theInstance = nullptr;
	}

	static T& instance ()
	{
		if(!theInstance)
		{
			theInstance = NEW T;
			Object::addGarbageCollected (theInstance, false);
		}
		return *theInstance;
	}

	static T* peekInstance ()
	{
		return theInstance;
	}
	
	static Object* __createSingleton () // used by meta class
	{
		return return_shared (&instance ());
	}

protected:
	static T* theInstance;
};

#define DEFINE_SINGLETON(Class) namespace CCL { template<> Class* Singleton<Class>::theInstance = nullptr; }

//************************************************************************************************
// ExternalSingleton
/** Singleton instance, the actual instance can be provided by a derived class declared in another source file. */
//************************************************************************************************

template <class T>
class ExternalSingleton: public Singleton<T>
{
public:
	static T& instance ()
	{
		if(!Singleton<T>::theInstance)
		{
			Singleton<T>::theInstance = createExternalInstance ();
			Object::addGarbageCollected (Singleton<T>::theInstance, false);
		}
		return *Singleton<T>::theInstance;
	}

	static T* createExternalInstance ();

	static Object* __createSingleton () // used by meta class
	{
		return return_shared (&instance ());
	}
};

#define DEFINE_EXTERNAL_SINGLETON(Class,Impl) DEFINE_SINGLETON(Class)\
	namespace CCL { template<> Class* ExternalSingleton<Class>::createExternalInstance () { return NEW Impl; } }

//************************************************************************************************
// SharedSingleton
/** Singleton instance, released by caller. */
//************************************************************************************************

template <class T>
class SharedSingleton
{
public:
	virtual ~SharedSingleton ()
	{
		if((SharedSingleton*)theInstance == this)
			theInstance = nullptr;
	}

	static T* instance ()
	{
		if(!theInstance)
			theInstance = NEW T;
		else
			theInstance->retain ();
		return theInstance;
	}

	static T* peekInstance ()
	{
		return theInstance;
	}

	static Object* __createSingleton () // used by meta class
	{
		return instance ();
	}

protected:
	static T* theInstance;
};

#define DEFINE_SHARED_SINGLETON(Class) namespace CCL { template<> Class* SharedSingleton<Class>::theInstance = nullptr; }

//************************************************************************************************
// UnmanagedSingleton
/** Singleton instance, must be released explicitely via cleanup call. */
//************************************************************************************************

template <class T>
class UnmanagedSingleton
{
public:
	virtual ~UnmanagedSingleton ()
	{
		if(theInstance == this)
			theInstance = nullptr;
	}

	static T& instance ()
	{
		if(!theInstance)
			theInstance = NEW T;
		return *theInstance;
	}

	static T* peekInstance ()
	{
		return theInstance;
	}

	static void cleanupInstance ()
	{
		if(theInstance)
			delete theInstance;
	}

	static Object* __createSingleton () // used by meta class
	{
		return return_shared (&instance ());
	}

protected:
	static T* theInstance;
};

#define DEFINE_UNMANAGED_SINGLETON(Class) namespace CCL { template<> Class* UnmanagedSingleton<Class>::theInstance = nullptr; }

} // namespace CCL

#endif // _ccl_singleton_h
