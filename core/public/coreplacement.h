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
// Filename    : core/public/coreplacement.h
// Description : Helpers for placing instances in preallocated buffers
//
//************************************************************************************************

#ifndef _coreplacement_h
#define _coreplacement_h

#include <new>

#include "coreprimitives.h"

namespace Core {

#if __cpp_variadic_templates

//************************************************************************************************
// MaxSizeOf
/** Calculates the maximum size of multiple types */
//************************************************************************************************

template<typename... Args> 
struct MaxSizeOf 
{ static constexpr int value = 0; };

template<typename FirstArg, typename... Args> 
struct MaxSizeOf<FirstArg, Args...> 
{ static constexpr int value = static_max<sizeof(FirstArg), MaxSizeOf<Args...>::value>::value; };

//************************************************************************************************
// PlacementBuffer
/** Places one instance from a selection of classes in a preallocated buffer */
//************************************************************************************************

template<class... Args> 
struct PlacementBuffer
{
	PlacementBuffer ();

	/** Returns the instance placed in this placement buffer. */
	template<typename T> 
	T* as ();

	/**
		Creates an instance of a default-constructible class.
		@param index The index of the template argument corresponding to the class of the requested instance.
	*/
	void create (int index);
		
	/**
		Creates an instance of type T with given constructor arguments.
		@param args Constructor arguments.
		@return A pointer to the requested instance.
	*/
	template<typename T, typename... CArgs> 
	T* create (CArgs&&... args);

protected:
	static constexpr int kMaxSize = MaxSizeOf<Args...>::value;
	
	template<class... Classes> struct Placer
	{
		static void place (int index, int8* buffer) {} 
	};

	template<class First, class... Classes> struct Placer<First, Classes...>
	{
		static void place (int index, int8* buffer)
		{
			if(index == 0)
				new (buffer) First;
			else
				Placer<Classes...>::place (index - 1, buffer);
		}
	};

	CORE_ALIGN (int8, 16) buffer[kMaxSize];
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// PlacementBuffer implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class... Args>
PlacementBuffer<Args...>::PlacementBuffer ()
: buffer { 0 }
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class... Args>
void PlacementBuffer<Args...>::create (int index)
{
	Placer<Args...>::place (index, buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class... Args>
template<typename T, typename... CArgs>
T* PlacementBuffer<Args...>::create (CArgs&&... args)
{
	static_assert (sizeof(T) <= kMaxSize, "The requested type cannot be placed in this placement buffer.");	
	
	new (buffer) T (args...);
	return as<T> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class... Args>
template<typename T> 
inline T* PlacementBuffer<Args...>::as ()
{
	return reinterpret_cast<T*> (buffer);
};

#endif // __cpp_variadic_templates

} // namespace Core

#endif // _coreplacement_h
