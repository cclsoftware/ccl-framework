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
// Filename    : ccl/base/signalslot.cpp
// Description : Signal Slot
//
//************************************************************************************************

#include "ccl/base/signalslot.h"

using namespace CCL;

//************************************************************************************************
// SignalSlot
//************************************************************************************************

SignalSlot::SignalSlot (ISubject* subject, StringID signalName)
: ListLink<SignalSlot*> (0),
  subject (subject),
  signalName (signalName),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSlot::~SignalSlot ()
{
	if(isOrphaned () == false)
		deactivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSlot* SignalSlot::getNext ()
{
	return static_cast<SignalSlot*> (next);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSlot::activate ()
{
	ASSERT (subject != nullptr)

	if(!isActive () && subject)
		subject->addObserver (this),
		isActive (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSlot::deactivate ()
{
	ASSERT (subject != nullptr)

	if(isActive () && subject)
		subject->removeObserver (this),
		isActive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignalSlot::isOrphaned () const
{
	return subject == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SignalSlot::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		// slot remains in orphaned state
		deactivate ();
		this->subject = nullptr;
	}
	
	// make sure no code follows after dispatch(), in case we are killed via SignalSlotList::unadvise()!
	if(signalName.isEmpty () || msg == signalName)
		dispatch (msg);
}

//************************************************************************************************
// SignalSlotList
//************************************************************************************************

SignalSlotList::~SignalSlotList ()
{
	unadviseAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignalSlotList::isEmpty () const
{
	return const_cast<SignalSlotList*> (this)->getFirst () == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSlot* SignalSlotList::getFirst ()
{
	return (SignalSlot*)_head;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSlotList::unadvise (SignalSlot* slot)
{
	ASSERT (slot != nullptr)
	if(slot == nullptr)
		return;

	removeLink (slot);
	delete slot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSlotList::unadvise (ISubject* subject)
{
	ASSERT (subject != nullptr)
	if(subject == nullptr)
		return;

	for(SignalSlot* slot = getFirst (); slot != nullptr; )
	{
		SignalSlot* next = slot->getNext ();
		if(slot->subject == subject)
		{
			removeLink (slot);
			delete slot;
		}
		slot = next;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSlotList::unadviseAll ()
{
	for(SignalSlot* slot = getFirst (); slot != nullptr; )
	{
		SignalSlot* next = slot->getNext ();
		delete slot;
		slot = next;
	}

	removeAllLinks ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSlot* SignalSlotList::advise (SignalSlot* slot)
{
	ASSERT (slot != nullptr)
	appendLink (slot);
	slot->activate ();
	return slot;
}
