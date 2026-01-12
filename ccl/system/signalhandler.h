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
// Filename    : ccl/system/signalhandler.h
// Description : Signal Handler
//
//************************************************************************************************

#ifndef _ccl_signalhandler_h
#define _ccl_signalhandler_h

#define DEBUG_OBSERVERS (0 && DEBUG)

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class ObserverIterator;
	
//************************************************************************************************
// ObserverList
//************************************************************************************************

class ObserverList: public Unknown
{
public:
	ObserverList (ISubject* subject);

	PROPERTY_POINTER (ISubject, subject, Subject)

	bool isEmpty () const;
	void append (IObserver* observer);
	void remove (IObserver* observer);
	void performSignal (MessageRef msg);

	#if DEBUG_OBSERVERS
	void dump ();
	#endif

protected:
	friend class ObserverIterator;

	LinkedList<IObserver*> observers;
	LinkedList<ObserverIterator*> iterators;

	#if DEBUG_OBSERVERS
	public:
	MutableCString subjectInfo;
	LinkedList<MutableCString> observerInfo;
	#endif
};

//************************************************************************************************
// ObserverIterator
//************************************************************************************************

class ObserverIterator: public ListIterator<IObserver*>
{
public:
	ObserverIterator (ObserverList& observerList);
	~ObserverIterator ();

	void removed (IObserver* observer);

protected:
	ObserverList& observerList;
};

//************************************************************************************************
// SignalHandler
//************************************************************************************************

class SignalHandler: public Unknown,
					 public ISignalHandler
{
public:
	SignalHandler ();
	~SignalHandler ();

	// ISignalHandler
	tresult CCL_API advise (ISubject* subject, IObserver* observer) override;
	tresult CCL_API unadvise (ISubject* subject, IObserver* observer) override;
	tresult CCL_API performSignal (ISubject* subject, MessageRef msg) override;
	tresult CCL_API queueSignal (ISubject* subject, IMessage* msg) override;
	tresult CCL_API queueChanged (ISubject* subject) override;
	tresult CCL_API cancelSignals (ISubject* subject) override;
	tresult CCL_API postMessage (IObserver* observer, IMessage* msg, int delay) override;
	tresult CCL_API postMessageBlocking (IObserver* observer, IMessage* msg) override;
	tresult CCL_API cancelMessages (IObserver* observer) override;
	tresult CCL_API flush (IObserver* observer = nullptr) override;
	tbool CCL_API hasObservers (ISubject* subject) override;

	CLASS_INTERFACE (ISignalHandler, Unknown)

protected:
	enum Constants { kHashSize = 512 };

	typedef void* CallbackID;
	typedef void (*CallbackFunction) (CallbackID id, IMessage* msg);

	struct Waitable: Unknown
	{
		bool done;
		
		Waitable ()
		: done (false)
		{}
	};

	struct CallbackMsg
	{
		CallbackFunction callback;
		CallbackID id;
		IMessage* msg;
		int64 time;
		SharedPtr<Waitable> waitable;

		#if DEBUG_OBSERVERS
		MutableCString observerClass;
		MutableCString messageId;
		#endif

		CallbackMsg (CallbackFunction callback, CallbackID id, IMessage* msg, int64 time, Waitable* waitable);
		~CallbackMsg ();

		bool isEqual (IMessage* other) const;
		void replace (IMessage* other);
		void execute () { callback (id, msg); }
	};

	Threading::CriticalSection lock;
	LinkedList<ObserverList*> buckets[kHashSize];
	typedef LinkedList<CallbackMsg*> CallbackQueue;
	CallbackQueue callbackQueue;
	LinkedList<CallbackQueue*> currentDelayedMessages;  ///< points to local variables in flush ()
	CallbackMsg* currentMessage;
	struct DelayMessageGuard;

	int hash (ISubject* subject) const;
	ObserverList* lookup (ISubject* subject) const;
	void queueCallback (CallbackFunction callback, CallbackID id, IMessage* msg, int64 time = 0, Waitable* waitable = nullptr);
	void cancelCallback (CallbackID id);
	tbool CCL_API messagesPending (IObserver* observer) override;

	static void signalCallback (CallbackID id, IMessage* msg);
	static void changedCallback (CallbackID id, IMessage* msg);
	static void messageCallback (CallbackID id, IMessage* msg);
};

} // namespace CCL

#endif // _ccl_signalhandler_h
