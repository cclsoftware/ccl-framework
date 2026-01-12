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
// Filename    : ccl/base/signalslot.h
// Description : Signal Slot
//
//************************************************************************************************

#ifndef _ccl_signalslot_h
#define _ccl_signalslot_h

#include "ccl/public/base/iobserver.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//************************************************************************************************
// SignalSlot
//************************************************************************************************

class SignalSlot: ListLink<SignalSlot*>,
				  IObserver
{
public:
	SignalSlot (ISubject* subject, StringID signalName);
	virtual ~SignalSlot ();

	PROPERTY_MUTABLE_CSTRING (signalName, SignalName)

	void activate ();
	void deactivate ();
	bool isOrphaned () const; ///< true if subject has been destroyed 

	SignalSlot* getNext ();

protected:
	ISubject* subject;
	int flags;
	enum Flags { kActive = 1<<0 };
	friend class SignalSlotList;

	PROPERTY_FLAG (flags, kActive, isActive)

	virtual void dispatch (MessageRef msg) = 0;

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IMPLEMENT_DUMMY_UNKNOWN (IObserver)
};

//************************************************************************************************
// MemberFuncSlot
//************************************************************************************************

template <class T>
class MemberFuncSlot: public SignalSlot
{
public:
	typedef void (T::*MemberFunc) (MessageRef msg);

	MemberFuncSlot (ISubject* subject, StringID signalName, T* target, MemberFunc memberFunc)
	: SignalSlot (subject, signalName),
	  target (target),
	  memberFunc (memberFunc)
	{}

protected:
	T* target;
	MemberFunc memberFunc;

	// SignalSlot
	void dispatch (MessageRef msg) override
	{
		(target->*memberFunc) (msg);
	}
};

//************************************************************************************************
// SignalSlotList
//************************************************************************************************

class SignalSlotList: LinkedList<SignalSlot*>
{
public:
	~SignalSlotList ();

	bool isEmpty () const;

	template <typename T>
	SignalSlot* advise (ISubject* subject, StringID signalName, T* target, typename MemberFuncSlot<T>::MemberFunc memberFunc)
	{
		ASSERT (subject && target && memberFunc)
		return advise (NEW MemberFuncSlot<T> (subject, signalName, target, memberFunc));
	}

	void unadvise (SignalSlot* slot);
	void unadvise (ISubject* subject);	///< unadvise all slots of given subject
	void unadviseAll ();

	SignalSlot* getFirst ();

protected:
	SignalSlot* advise (SignalSlot* slot);
};

} // namespace CCL

#endif // _ccl_signalslot_h
