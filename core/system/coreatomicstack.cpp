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
// Filename    : core/system/coreatomicstack.cpp
// Description : Atomic Stack
//
//************************************************************************************************

#include "core/system/coreatomicstack.h"

using namespace Core;

//************************************************************************************************
// AtomicStackLocked
//************************************************************************************************

AtomicStackLocked::AtomicStackLocked ()
: head (nullptr),
  stackDepth (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AtomicStackLocked::Element* AtomicStackLocked::pop ()
{
	Threads::ScopedLock scopedLock (lock);

	if(!head)
		return nullptr;

	Element* e = head;
	head = head->next;
	e->next = nullptr;
	stackDepth--;
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomicStackLocked::push (Element* e)
{
	ASSERT (e->next == nullptr)

	Threads::ScopedLock scopedLock (lock);

	Element* oldHead = head;
	head = e;
	head->next = oldHead;
	stackDepth++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomicStackLocked::flush ()
{
	Threads::ScopedLock scopedLock (lock);
	head = nullptr;
	stackDepth = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AtomicStackLocked::depth ()
{
	Threads::ScopedLock scopedLock (lock);
	return stackDepth;
}
