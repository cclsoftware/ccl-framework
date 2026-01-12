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
// Filename    : core/system/coreatomicstack.h
// Description : Atomic Stack
//
//************************************************************************************************

#ifndef _coreatomicstack_h
#define _coreatomicstack_h

#include "core/platform/corefeatures.h"

#if CORE_ATOMIC_STACK_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coreatomicstack)
	#define CORE_HAS_ATOMIC_STACK 1
#elif CORE_ATOMIC_STACK_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coreatomicstack)
	#define CORE_HAS_ATOMIC_STACK 1
#elif CORE_ATOMIC_STACK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coreatomicstack.posix.h"
	#define CORE_HAS_ATOMIC_STACK 1
#else
	#include "core/platform/shared/coreplatformatomicstack.h"
	#define CORE_HAS_ATOMIC_STACK 0
#endif

#include "core/system/coreatomic.h"
#include "core/system/corethread.h"

namespace Core {

#if CORE_HAS_ATOMIC_STACK

//************************************************************************************************
// AtomicStack
/** Lock-free atomic stack.
	\ingroup core_thread */
//************************************************************************************************

class AtomicStack
{
public:
	typedef Platform::AtomicStackElement Element;

	Element* pop ();
	void push (Element* e);
	void flush ();
	int depth ();

protected:
	Platform::AtomicStack platformStack;
};

#endif // CORE_HAS_ATOMIC_STACK

//************************************************************************************************
// AtomicStackLocked
/** Non-lockfree atomic stack.
	\ingroup core_thread */
//************************************************************************************************

class AtomicStackLocked
{
public:
	AtomicStackLocked ();

	typedef Platform::AtomicStackElement Element;

	Element* pop ();
	void push (Element* e);
	void flush ();
	int depth ();

protected:
	Threads::Lock lock;
	Element* head;
	int stackDepth;
};

#if CORE_HAS_ATOMIC_STACK

//************************************************************************************************
// AtomicStack implementation
//************************************************************************************************

inline AtomicStack::Element* AtomicStack::pop ()
{ return platformStack.pop (); }

inline void AtomicStack::push (Element* e)
{ platformStack.push (e); }

inline void AtomicStack::flush ()
{ platformStack.flush (); }

inline int AtomicStack::depth ()
{ return platformStack.depth (); }

#endif // CORE_HAS_ATOMIC_STACK

} // namespace Core

#endif // _coreatomicstack_h
