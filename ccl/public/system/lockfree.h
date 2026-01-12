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
// Filename    : ccl/public/system/lockfree.h
// Description : Lock free data structures
//
//************************************************************************************************

#ifndef _ccl_lockfree_h
#define _ccl_lockfree_h

#include "ccl/public/base/platform.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/atomic.h"

namespace CCL {
namespace LockFree {

//************************************************************************************************
// Element with aligned next pointer
//************************************************************************************************

template <typename T>
struct Element
{
	CCL_ALIGN(T*) next;

	Element ()
	: next (nullptr)
	{}
};

//************************************************************************************************
// Lock Free Stack -> Last In First Out
//************************************************************************************************

template <typename T>
class Stack
{
public:
	Stack ();
	
	void push (T* e);
	void pushFast (T* e);
	T* pop ();
	void flush ();

protected:
	CCL_ALIGN(T*) volatile head;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Stack<T>::Stack ()
{
	head = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void Stack<T>::push (T* e)
{
	do
	{
		e->next = head;
	}
	while (!AtomicTestAndSetPtrInline((void*&)head, (void*)e, (void*)e->next));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void Stack<T>::pushFast (T* e)
{
	e->next = head;
	head = e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T* Stack<T>::pop ()
{
	T* volatile e;
	do
	{
		e = head;
        if(e == nullptr)
			return e;
	}
	while (!AtomicTestAndSetPtrInline((void*&)head, (void*)e->next, (void*)e));
	
	e->next = nullptr;
	return (T*)e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void Stack<T>::flush ()
{
	AtomicSetPtrInline ((void*&)head, (void*)0);
}

//************************************************************************************************
//	Lock Free Queue -> First In First Out
//  Only works for Single Producer Single Consumer
//	T must have an aligned(16) next pointer
//************************************************************************************************

template <typename T>
class Queue
{
public:
	Queue ();
	~Queue ();
	
	void push (T* e);
	T* pop ();
	T* peek ();
	void flush ();

protected:
	CCL_ALIGN(T*) volatile allocated;
	CCL_ALIGN(T*) volatile first;
	CCL_ALIGN(T*) volatile divider;
	CCL_ALIGN(T*) volatile last;  
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Queue<T>::Queue ()
{
	allocated = last = divider = first = NEW T;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Queue<T>::~Queue ()
{
	delete allocated;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void Queue<T>::push (T* e)
{
	ASSERT((((int64)e) & 7) == 0)

	last->next = e;
    last = last->next;					// publish it
	
    while(first != divider) 
	{
		T* e = first;
		first = first->next;
		delete e;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T* Queue<T>::pop ()
{
	T* result = 0;
	
	// if queue is nonempty
	if(divider != last)
	{
		result = divider->next;			// C: copy it back
		divider = result;				// D: publish that we took it
    }
    return result;						// return result
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T* Queue<T>::peek ()
{
	// if queue is nonempty
	if(divider != last)
		return divider;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void Queue<T>::flush ()
{
	AtomicSetPtrInline ((void*&)divider, (void*)last);
	AtomicSetPtrInline ((void*&)first, (void*)last);
}

} // namespace LockFree
} // namespace CCL

#endif // _ccl_lockfree_h
