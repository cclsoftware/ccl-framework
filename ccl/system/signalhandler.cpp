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
// Filename    : ccl/system/signalhandler.cpp
// Description : Signal Handler
//
//************************************************************************************************

#define DEBUG_LOG 0
#define MAIN_THREAD_POLICY 1

#if 0
	#define MAIN_THREAD_POLICY_ASSERT ASSERT (System::IsInMainThread ())
#else
	#define MAIN_THREAD_POLICY_ASSERT SOFT_ASSERT (System::IsInMainThread (), "SignalHandler not in main thread!\n")
#endif

#include "ccl/system/signalhandler.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISignalHandler& CCL_API System::CCL_ISOLATED (GetSignalHandler) ()
{
	static SignalHandler theSignalHandler;
	return theSignalHandler;
}

//************************************************************************************************
// ObserverList
//************************************************************************************************

#if DEBUG_OBSERVERS
static bool isVtableInitialized (const IUnknown& obj)
{
	interface IUnknownCInterface
	{
		void* vtablePointer;
	};

	const IUnknownCInterface& cInterface = reinterpret_cast<const IUnknownCInterface&> (obj);
	return cInterface.vtablePointer != 0;
}

static MutableCString getClassName (IUnknown* unk)
{
	MutableCString className;
	UnknownPtr<IObject> obj (unk);
	if(obj)
	{
		const ITypeInfo& typeId = obj->getTypeInfo ();
		if(!isVtableInitialized (typeId)) // workaround for early dependencies of static objects
			className = CSTR ("!vtable");
		else
		{
			className = typeId.getClassName ();
		
			Variant name;
			if(obj->getProperty (name, "name"))
			{
				className += "-";
				className += VariantString (name);
			}
		}
	}
	if(className.isEmpty ())
		className = CSTR ("null");
	return className;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

ObserverList::ObserverList (ISubject* subject)
: subject (subject)
{
	#if DEBUG_OBSERVERS
	subjectInfo = getClassName (subject);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObserverList::isEmpty () const
{
	return observers.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObserverList::append (IObserver* observer)
{
	observers.append (observer);

	#if DEBUG_OBSERVERS
	observerInfo.append (getClassName (observer));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObserverList::remove (IObserver* observer)
{
	#if DEBUG_OBSERVERS
	int index = 0;
	ListForEach (observers, IObserver*, o)
		if(o == observer)
		{
			observerInfo.removeAt (index);
			break;
		}
		index++;
	EndFor
	#endif

	// check iterators before link is released
	if(!iterators.isEmpty ())
		ListForEach (iterators, ObserverIterator*, iter)
			iter->removed (observer);
		EndFor

	// remove from list
	observers.remove (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObserverList::performSignal (MessageRef msg)
{
	SharedPtr<ObserverList> keeper (this);
	
	ObserverIterator iter (*this);
	IObserver* observer;
	while((observer = iter.next ()) != nullptr)
		observer->notify (subject, msg);

	#if DEBUG_LOG
	if(getRetainCount () == 1)
		CCL_PRINTLN ("ObserverList deleted during performSignal()!")
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_OBSERVERS
void ObserverList::dump ()
{
	MutableCString message ("Subject: ");
	message += subjectInfo;
	message += ", Observers:";

	ListForEach (observerInfo, MutableCString, obsInfo)
		message += " ";
		message += obsInfo;
	EndFor

	Debugger::println (message);
}
#endif

//************************************************************************************************
// ObserverIterator
//************************************************************************************************

ObserverIterator::ObserverIterator (ObserverList& list)
: ListIterator<IObserver*> (list.observers),
  observerList (list)
{
	observerList.iterators.append (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObserverIterator::~ObserverIterator ()
{
	observerList.iterators.remove (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObserverIterator::removed (IObserver* observer)
{
	// if removed observer is next in list we have to advance manually - rare case, but happens.
	if(_next && _next->getData () == observer)
	{
		next ();
		CCL_PRINTLN ("Next observer removed during iteration!")
	}
}

//************************************************************************************************
// SignalHandler::CallbackMsg
//************************************************************************************************

SignalHandler::CallbackMsg::CallbackMsg (CallbackFunction _callback, CallbackID _id, IMessage* _msg, int64 _time, Waitable* waitable)
: callback (_callback),
  id (_id),
  msg (_msg),
  time (_time),
  waitable (waitable)
{
#if DEBUG_OBSERVERS
	if(callback == changedCallback)
	{
		ObserverList* list = reinterpret_cast<ObserverList*> (id);
		observerClass = list->subjectInfo;
		messageId = kChanged;
	}
	else
	{
		observerClass = getClassName ((IObserver*)_id);
		messageId = _msg ? _msg->getID () : "null";
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalHandler::CallbackMsg::~CallbackMsg ()
{
	if(waitable)
		waitable->done = true;

	if(msg)
		msg->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignalHandler::CallbackMsg::isEqual (IMessage* other) const
{ 
	return msg && other ? msg->getID ().compare (other->getID ()) == 0 : msg == other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::CallbackMsg::replace (IMessage* other)
{
	if(msg)
		msg->release ();
	msg = other;
}

//************************************************************************************************
// SignalHandler::DelayMessageGuard - used in SignalHandler::flush to ensure cleanup in case of exceptions
//************************************************************************************************

struct SignalHandler::DelayMessageGuard
{
	DelayMessageGuard (SignalHandler& signalHandler)
	: signalHandler (signalHandler)
	{
		Threading::ScopedLock scopedLock (signalHandler.lock);
		signalHandler.currentDelayedMessages.append (&delayedMessages);	
	}
	~DelayMessageGuard ()
	{
		Threading::ScopedLock scopedLock (signalHandler.lock);
		while(!delayedMessages.isEmpty ())
			signalHandler.callbackQueue.append (delayedMessages.removeFirst ());
	
		signalHandler.currentDelayedMessages.remove (&delayedMessages);
	}

	SignalHandler& signalHandler;
	CallbackQueue delayedMessages;
};

//************************************************************************************************
// SignalHandler
//************************************************************************************************

void SignalHandler::signalCallback (CallbackID id, IMessage* msg)
{
	ObserverList* list = (ObserverList*)id;
	ASSERT (list != nullptr && msg != nullptr)
	if(list && msg)
		list->performSignal (*msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::changedCallback (CallbackID id, IMessage*)
{
	static const Message changedMessage (kChanged);

	ObserverList* list = (ObserverList*)id;
	ASSERT (list != nullptr)
	if(list)
		list->performSignal (changedMessage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::messageCallback (CallbackID id, IMessage* msg)
{
	IObserver* observer = (IObserver*)id;
	ASSERT (observer != nullptr && msg != nullptr)
	if(observer && msg)
		observer->notify (nullptr, *msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalHandler::SignalHandler ()
: currentMessage (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalHandler::~SignalHandler ()
{
	// If queues aren't empty here, something went terribly wrong!
	
	// Note: This is the destructor of a static object, other singletons might be
	// dead already at this stage, thus we can not use stuff like CCL_WARN here!!!
	
	#if 1//DEBUG
	for(int i = 0; i < kHashSize; i++)
	{
		if(!buckets[i].isEmpty ())
		{
			#if DEBUG_OBSERVERS
			if(ObserverList* list = buckets[i].getFirst ())
				list->dump ();
			#endif

			CCL_DEBUGGER ("[Signals] Hash table not empty!")
			break;
		}
	}

	ASSERT (callbackQueue.isEmpty () == true)
	if(!callbackQueue.isEmpty ())
	{
		CCL_DEBUGGER ("[Signals] Callback queue not empty!\n")
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SignalHandler::hash (ISubject* subject) const
{ 
	return ccl_hash_pointer (subject, kHashSize); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObserverList* SignalHandler::lookup (ISubject* subject) const
{
	CCL_PROFILE_START (lookup)
	ListForEach (buckets[hash (subject)], ObserverList*, list)
		if(list->getSubject () == subject)
			return list;
	EndFor
	CCL_PROFILE_STOP (lookup)
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::queueCallback (CallbackFunction callback, CallbackID id, IMessage* _msg, int64 time, Waitable* waitable)
{
	CallbackMsg* msg = NEW CallbackMsg (callback, id, _msg, time, waitable);

	Threading::ScopedLock scopedLock (lock);
	callbackQueue.append (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::cancelCallback (CallbackID id)
{
	Threading::ScopedLock scopedLock (lock);

	ListForEach (callbackQueue, CallbackMsg*, msg)
		if(msg->id == id)
		{
			callbackQueue.remove (msg);
			delete msg;
		}
	EndFor

	ListForEach (currentDelayedMessages, CallbackQueue*, queue)
		ListForEach (*queue, CallbackMsg*, msg)
			if(msg->id == id)
			{
				queue->remove (msg);
				delete msg;
			}
		EndFor
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SignalHandler::messagesPending (IObserver* observer)
{
	Threading::ScopedLock scopedLock (lock);

	ListForEach (callbackQueue, CallbackMsg*, msg)
		if(msg->id == observer)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::flush (IObserver* observer)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#endif

	if(callbackQueue.isEmpty ())
		return kResultOk;

	DelayMessageGuard guard (*this);

	while(1)
	{
		CallbackMsg* msg = nullptr;
		{
			Threading::ScopedLock scopedLock (lock);
			msg = callbackQueue.removeFirst ();
		}
		if(msg == nullptr)
			break;

		if(observer && msg->id != observer)
			guard.delayedMessages.append (msg);
		else if(msg->time > 0)
		{
			if(observer != nullptr || System::GetSystemTicks () >= msg->time)
			{
				{
					ScopedVar<CallbackMsg*> scope (currentMessage, msg);
					msg->execute ();
				}
				delete msg;
			}
			else
				guard.delayedMessages.append (msg);
		}
		else
		{
			msg->execute ();
			delete msg;
		}
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::advise (ISubject* subject, IObserver* observer)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#endif

	CCL_PROFILE_START (advise)
	Threading::ScopedLock scopedLock (lock);

	ObserverList* list = lookup (subject);
	if(list == nullptr)
	{
		list = NEW ObserverList (subject);
		buckets[hash (subject)].append (list);
	}

	list->append (observer);
	CCL_PROFILE_STOP (advise)
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::unadvise (ISubject* subject, IObserver* observer)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#endif

	CCL_PROFILE_START (unadvise)
	Threading::ScopedLock scopedLock (lock);
	
	ObserverList* list = lookup (subject);
	if(list)
	{
		list->remove (observer);
		if(list->isEmpty ())
		{
			cancelSignals (subject); // cancel queued signals

			buckets[hash (subject)].remove (list);
			list->release ();
		}
	}
	CCL_PROFILE_STOP (unadvise)
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SignalHandler::hasObservers (ISubject* subject)
{
	return lookup (subject) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::performSignal (ISubject* subject, MessageRef msg)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#else
	Threading::ScopedLock scopedLock (lock);
	#endif

	ObserverList* list = lookup (subject);
	if(list)
		list->performSignal (msg);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::queueSignal (ISubject* subject, IMessage* msg)
{
	ASSERT (subject != nullptr && msg != nullptr)
	if(subject == nullptr || msg == nullptr)
		return kResultInvalidPointer;

	Threading::ScopedLock scopedLock (lock);

	ObserverList* list = lookup (subject);
	if(list)
	{
		// check if message already exists...
		ListForEach (callbackQueue, CallbackMsg*, cbMsg)
			if(cbMsg->callback == signalCallback &&
			   cbMsg->id == list && 
			   cbMsg->isEqual (msg))
			{
				cbMsg->replace (msg);
				return kResultOk;
			}
		EndFor
	
		queueCallback (signalCallback, list, msg);
	}
	else
		msg->release ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::queueChanged (ISubject* subject)
{
#if 0
	return queueSignal (subject, NEW Message (kChanged));
#else
	ASSERT (subject != nullptr)
	if(subject == nullptr)
		return kResultInvalidPointer;

	Threading::ScopedLock scopedLock (lock);

	ObserverList* list = lookup (subject);
	if(list)
	{
		#if DEBUG_OBSERVERS
		static bool shouldDumpQueue = false; // <-- can be set at runtime to dump the callback queue
		if(shouldDumpQueue && !callbackQueue.isEmpty ())
		{
			Debugger::println ("*** Begin Callback Queue ***");
			ListForEach (callbackQueue, CallbackMsg*, cbMsg)
				Debugger::print ("Observer: ");
				Debugger::print (cbMsg->observerClass);
				Debugger::print (" Message: ");
				Debugger::println (cbMsg->messageId);
			EndFor
			Debugger::println ("*** End Callback Queue ***");
		}
		#endif

		// check if message already exists...
		ListForEach (callbackQueue, CallbackMsg*, cbMsg)
			if(cbMsg->callback == changedCallback)
			{
				if(cbMsg->id == list)
					return kResultOk;
			}
		EndFor
	
		queueCallback (changedCallback, list, nullptr);
	}

	return kResultOk;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::cancelSignals (ISubject* subject)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#endif

	Threading::ScopedLock scopedLock (lock);

	ObserverList* list = lookup (subject);
	if(list)
		cancelCallback (list);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::postMessage (IObserver* observer, IMessage* msg, int delay)
{
	ASSERT (observer != nullptr && msg != nullptr)
	if(observer == nullptr || msg == nullptr)
		return kResultInvalidPointer;

	int64 time = 0;
	if(delay != 0) // Note: negative delay is allowed to collect messages and execute them immediately!
	{
		time = System::GetSystemTicks () + delay;

		Threading::ScopedLock scopedLock (lock);

		// check if message already exists...
		ListForEach (callbackQueue, CallbackMsg*, cbMsg)
			if(cbMsg->callback == messageCallback &&
			   cbMsg->id == observer && 
			   cbMsg->isEqual (msg))
			{
				cbMsg->time = time; // <--- update time!!!
				cbMsg->replace (msg);
				return kResultOk;
			}
		EndFor

		ListForEach (currentDelayedMessages, CallbackQueue*, queue)
			ListForEach (*queue, CallbackMsg*, cbMsg)
				if(cbMsg->callback == messageCallback &&
					cbMsg->id == observer && 
					cbMsg->isEqual (msg))
				{
					cbMsg->time = time; // <--- update time!!!
					cbMsg->replace (msg);
					return kResultOk;
				}
			EndFor
		EndFor

		// when a delayed message is posted while flush () delivers a message with same ID and observer, avoid delivering the new message in the running loop
		if(currentMessage && System::IsInMainThread () &&
			currentMessage->id == observer &&
			currentMessage->isEqual (msg))
		{
			if(CallbackQueue* currentDelayedQueue = currentDelayedMessages.getLast ())
			{
				// add the new message to the local list in DelayMessageGuard, so it will be appended after the flush loop
				CallbackMsg* callbackMsg = NEW CallbackMsg (messageCallback, observer, msg, time, nullptr);
				currentDelayedQueue->append (callbackMsg);
				return kResultOk;
			}
		}
	}

	queueCallback (messageCallback, observer, msg, time);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::postMessageBlocking (IObserver* observer, IMessage* msg)
{
	ASSERT (observer != nullptr && msg != nullptr)
	if(observer == nullptr || msg == nullptr)
		return kResultInvalidPointer;

	if(System::IsInMainThread ())
		observer->notify (nullptr, *msg);
	else
	{
		AutoPtr<Waitable> waitable = NEW Waitable;
		queueCallback (messageCallback, observer, msg, 0, waitable);
		while(!waitable->done)
			System::ThreadSleep (1);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SignalHandler::cancelMessages (IObserver* observer)
{
	#if MAIN_THREAD_POLICY
	MAIN_THREAD_POLICY_ASSERT
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	#endif

	cancelCallback (observer);
	return kResultOk;
}
