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
// Filename    : core/public/coreobserver.h
// Description : Observer pattern class
//
//************************************************************************************************

#ifndef _coreobserver_h
#define _coreobserver_h

#include "core/public/corevector.h"
#include "core/public/coretypes.h"

namespace Core {
	
/**
 @class ObserverList

 ObserverList and the associated DEFINE_OBSERVER macro can be used to simplify the use of observers
 and observer notification.
 
 Simply add DEFINE_OBSERVER to your class header, which will add an ObserverList object named 
 'observers' and stub out addObserver() and removeObserver() for your observer class type.
 
 Then, when you want to notify observers call an appropriate notify() method.
 
 example:
 
	class SomeClass;

	struct SomeObserver
	{
		virtual void someSimpleNotification () = 0;
		virtual void someNotification (int someParam, SomeClass* otherParam) = 0;
	};

	class SomeClass
	{
	public:
		...
		
		DEFINE_OBSERVER (SomeObserver)
 
		void doStuff ()
		{
			observers.notify (&SomeObserver::someSimpleNotification);
			// or ...
			observers.notify (&SomeObserver::someNotification, 123, this);
		}
	};
 
	class MyObserver: public SomeObserver
	{
	public:
		MyObserver (SomeClass& notifier)
		{
			notifier.addObserver (this); // notify us when something changes
		}
 
		// SomeObserver
		void someNotification (int someParam, SomeClass* notifier)
		{
			// we got notified!
		}
	};
 */
	
//************************************************************************************************
// Observer macros
//************************************************************************************************

#define DEFINE_OBSERVER(type) \
protected: \
Core::ObserverList<type> observers; \
public: \
void addObserver (type* observer) { observers.addObserver (observer); } \
void removeObserver (type* observer) { observers.removeObserver (observer); }

#define DEFINE_OBSERVER_OVERRIDE(type) \
protected: \
Core::ObserverList<type> observers; \
public: \
void addObserver (type* observer) override { observers.addObserver (observer); } \
void removeObserver (type* observer) override { observers.removeObserver (observer); }

//************************************************************************************************
// Args
/** Typedefs for basic types to simplify template parameters.
 \ingroup core */
//************************************************************************************************

namespace Args
{
	template <typename Arg> struct ArgType					 { typedef const Arg& type; };
	template <typename Arg> struct ArgType <Arg&>			 { typedef Arg& type; };
	template <typename Arg> struct ArgType <Arg*>			 { typedef Arg* type; };
	template <>             struct ArgType <CStringPtr>      { typedef CStringPtr type; };
	template <>             struct ArgType <char>            { typedef char type; };
	template <>             struct ArgType <int>             { typedef int type; };
	template <>             struct ArgType <int64>           { typedef int64 type; };
	template <>             struct ArgType <bool>            { typedef bool type; };
	template <>             struct ArgType <float>           { typedef float type; };
	template <>             struct ArgType <double>          { typedef double type; };
}
	
//************************************************************************************************
// ObserverList
/** Template list of observer objects for easy Observer pattern implementation.
 \ingroup core */
//************************************************************************************************

template <class T>
class ObserverList
{
public:
	/** Check if list is empty. */
	bool isEmpty () const
	{
		return list.isEmpty ();
	}
	
	/** Check if list contains given observer. */
	bool contains (T* observer)
	{
		return list.contains (observer);
	}
	
	/** Add observer to list. */
	void addObserver (T* observer)
	{
		ASSERT (contains (observer) == false);
		if(contains (observer))
		   return;
		list.add (observer);
	}
	
	/** Remove observer from list. */
	void removeObserver (T* observer)
	{
		list.remove (observer);
	}
	
	/** Call method on all observers (no arguments). */
	template <typename S>
	void notify (void (S::*notification) ())
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) ();
		EndFor
	}
	
	#define ARGUMENT_TYPE(a) typename Args::ArgType<a>::type
	#define OL_TEMPLATE(a)	 typename Arg##a
	#define OL_ARG(a)		 ARGUMENT_TYPE(Arg##a) argument##a

	/** Call method on all observers (one argument). */
	template <typename S, OL_TEMPLATE(1)>
	void notify (void (S::*notification) (Arg1), OL_ARG(1))
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) (argument1);
		EndFor
	}
	
	/** Call method on all observers (two arguments). */
	template <typename S, OL_TEMPLATE(1), OL_TEMPLATE(2)>
	void notify (void (S::*notification) (Arg1, Arg2), OL_ARG(1), OL_ARG(2))
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) (argument1, argument2);
		EndFor
	}
	
	/** Call method on all observers (three arguments). */
	template <typename S, OL_TEMPLATE(1), OL_TEMPLATE(2), OL_TEMPLATE(3)>
	void notify (void (S::*notification) (Arg1, Arg2, Arg3), OL_ARG(1), OL_ARG(2), OL_ARG(3))
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) (argument1, argument2, argument3);
		EndFor
	}
	
	/** Call method on all observers (four arguments). */
	template <typename S, OL_TEMPLATE(1), OL_TEMPLATE(2), OL_TEMPLATE(3), OL_TEMPLATE(4)>
	void notify (void (S::*notification) (Arg1, Arg2, Arg3, Arg4), OL_ARG(1), OL_ARG(2), OL_ARG(3), OL_ARG(4))
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) (argument1, argument2, argument3, argument4);
		EndFor
	}
	
	/** Call method on all observers (five arguments). */
	template <typename S, OL_TEMPLATE(1), OL_TEMPLATE(2), OL_TEMPLATE(3), OL_TEMPLATE(4), OL_TEMPLATE(5)>
	void notify (void (S::*notification) (Arg1, Arg2, Arg3, Arg4, Arg5), OL_ARG(1), OL_ARG(2), OL_ARG(3), OL_ARG(4), OL_ARG(5))
	{
		VectorForEachFast(list, T*, observer)
			(observer->*notification) (argument1, argument2, argument3, argument4, argument5);
		EndFor
	}
    
	/** Call method on all observers (six arguments). */
    template <typename S, OL_TEMPLATE(1), OL_TEMPLATE(2), OL_TEMPLATE(3), OL_TEMPLATE(4), OL_TEMPLATE(5), OL_TEMPLATE(6)>
    void notify (void (S::*notification) (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6), OL_ARG(1), OL_ARG(2), OL_ARG(3), OL_ARG(4), OL_ARG(5), OL_ARG(6))
    {
        VectorForEachFast(list, T*, observer)
            (observer->*notification) (argument1, argument2, argument3, argument4, argument5, argument6);
        EndFor
    }

	#undef OL_TEMPLATE
	#undef OL_ARG
	#undef ARGUMENT_TYPE
	
private:
	Vector<T*> list;
};

} // namespace Core

#endif // _coreobserver_h
