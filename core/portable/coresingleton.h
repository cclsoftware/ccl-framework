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
// Filename    : core/portable/coresingleton.h
// Description : Singleton class
//
//************************************************************************************************

#ifndef _coresingleton_h
#define _coresingleton_h

#include "core/public/coretypes.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// StaticSingleton
/** Template for static singleton object. 
	\ingroup core_portable */
//************************************************************************************************

template <class T>
class StaticSingleton
{
public:
	static T& instance ();
};

#define DEFINE_STATIC_SINGLETON(T) \
	namespace Core { namespace Portable { template<> T& StaticSingleton<T>::instance () { static T theInstance; return theInstance; } } }

//************************************************************************************************
// Deletable
/** Object deleted when application terminates. 
	\ingroup core_portable */
//************************************************************************************************

class Deletable
{
public:
	virtual ~Deletable () {}

	static void addInstance (Deletable* instance);
};

//************************************************************************************************
// Singleton
/** Template for singleton created on heap upon first request.
	\ingroup core_portable */
//************************************************************************************************

template <class T>
class Singleton: public Deletable
{
public:
	~Singleton ()
	{
		if(static_cast<T*> (this) == theInstance)
			theInstance = nullptr;
	}

	static T& instance ()
	{
		struct TInstance: public T // T might have a protected constructor
		{};
		
		if(!theInstance)
		{
			theInstance = NEW TInstance;
			Deletable::addInstance (theInstance);
		}
		return *theInstance;
	}

protected:
	static T* theInstance;
};

#define DEFINE_SINGLETON(T) namespace Core { namespace Portable { template<> T* Singleton<T>::theInstance = nullptr; }}

} // namespace Portable
} // namespace Core

#endif // _coresingleton_h
