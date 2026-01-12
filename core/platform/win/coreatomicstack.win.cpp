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
// Filename    : core/platform/win/coreatomicstack.win.cpp
// Description : Atomic Stack Windows implementation
//
//************************************************************************************************

#include "coreatomicstack.win.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// Win32AtomicStack
//************************************************************************************************

Win32AtomicStack::Win32AtomicStack ()
: head (nullptr)
{
	head = (SLIST_HEADER*)_aligned_malloc (sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
	ASSERT (head != nullptr)
	::InitializeSListHead (head);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32AtomicStack::~Win32AtomicStack ()
{
	_aligned_free (head);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32AtomicStack::Element* Win32AtomicStack::pop ()
{
	SLIST_ENTRY* _e = ::InterlockedPopEntrySList (head);
	Element* e = reinterpret_cast<Element*> (_e);
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32AtomicStack::push (Element* e)
{
	ASSERT(((int64)e & 0x7) == 0)
	SLIST_ENTRY* _e = reinterpret_cast<SLIST_ENTRY*> (e);
	::InterlockedPushEntrySList (head, _e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32AtomicStack::flush ()
{
	::InterlockedFlushSList (head);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32AtomicStack::depth ()
{
	return ::QueryDepthSList (head);
}
