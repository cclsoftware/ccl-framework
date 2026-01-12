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
// Filename    : ccl/base/asyncoperation.cpp
// Description : Asynchronous operation
//
//************************************************************************************************

#include "ccl/base/asyncoperation.h"

#include "ccl/base/kernel.h"
#include "ccl/base/message.h"

#include "ccl/public/base/iprogress.h"

using namespace CCL;

//************************************************************************************************
// AsyncSequenceOperation
//************************************************************************************************

class AsyncSequenceOperation: public CCL::AsyncOperation
{
public:
	AsyncSequenceOperation (AsyncSequence* sequence)
	: sequence (sequence)
	{}

	void CCL_API cancel () override
	{
		if(getState () != IAsyncInfo::kStarted)
			return;
		SharedPtr<AsyncSequenceOperation> keeper (this);
		sequence->cancel ();
	}

protected:
	SharedPtr<AsyncSequence> sequence;
};

//************************************************************************************************
// AsyncOperation
//************************************************************************************************

AsyncOperation* AsyncOperation::createCompleted (VariantRef result, bool deferred)
{
	AsyncOperation* operation = NEW AsyncOperation;
	operation->setResult (result);
	if(deferred == true)
		operation->setStateDeferred (kCompleted);
	else
		operation->setState (kCompleted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncOperation* AsyncOperation::createFailed (bool deferred)
{
	AsyncOperation* operation = NEW AsyncOperation;
	if(deferred == true)
		operation->setStateDeferred (kFailed);
	else
		operation->setState (kFailed);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncOperation::deferDestruction (IAsyncOperation* operation)
{
	Kernel::instance ().deferDestruction (operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AsyncOperation, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncOperation::AsyncOperation ()
: state (kNone),
  progressHandler (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncOperation::~AsyncOperation ()
{
	cancelSignals ();

	if(progressHandler)
		progressHandler->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "setState")
		setState (msg[0].asInt ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncOperation::setState (State newState)
{
	if(newState != state)
	{
		ASSERT (state <= kStarted || (state == kCompleted && newState == kStarted)) // allow also restart

		state = newState;

		if(state > kStarted && completionHandler)
			completionHandler->onCompletion (*this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncOperation::setStateDeferred (State state)
{
	(NEW Message ("setState", state, asUnknown ()))->post (this, -1); // pass this to keep instance alive until message delivery
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::setResult (VariantRef value)
{
	result = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation::State CCL_API AsyncOperation::getState () const
{
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API AsyncOperation::getResult () const
{
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::cancel ()
{
	setState (kCanceled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::close ()
{
	result.clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::setCompletionHandler (IAsyncCompletionHandler* handler)
{
	completionHandler = handler;

	if(state > kStarted && completionHandler)
		completionHandler->onCompletion (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncOperation::setProgressHandler (IProgressNotify* handler)
{
	take_shared (progressHandler, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* CCL_API AsyncOperation::getProgressHandler () const
{
	if(progressHandler)
		return progressHandler;

	// a later operation in the completion chain might have a handler assigned
	UnknownPtr<IAsyncOperation> completionOperation (completionHandler);
	if(completionOperation)
		return completionOperation->getProgressHandler ();

	return nullptr;
}

//************************************************************************************************
// AsyncSequence
//************************************************************************************************

AsyncSequence::AsyncSequence ()
: currentIndex (-1),
  cancelOnError (false)
{
	calls.objectCleanup ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AsyncSequence::~AsyncSequence ()
{
	ASSERT (calls.isEmpty ())
	ASSERT (totalOperation == nullptr)
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncSequence::isEmpty () const
{
	return calls.isEmpty ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int AsyncSequence::getCount () const
{
	return calls.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncSequence::CallItem* AsyncSequence::getItem (int index) const
{
	return static_cast<CallItem*> (calls.at (index));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AsyncSequence::getOperation (int index) const
{
	CallItem* item = getItem (index);
	return item ? item->getOperation () : nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Variant AsyncSequence::getResult (int index) const
{
	IAsyncOperation* operation = getOperation (index);
	return operation ? operation->getResult () : Variant ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AsyncSequence::CallItem* AsyncSequence::findItem (const IAsyncOperation& operation) const
{
	return static_cast<CallItem*> (calls.findIf ([&] (Object* obj)
		{ return static_cast<CallItem*> (obj)->getOperation () == &operation; }));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int AsyncSequence::add (IAsyncCall* call)
{
	auto item = NEW CallItem;
	item->setAsyncCall (call);

	int index = calls.count ();
	calls.add (item);
	return index;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncSequence::then (IAsyncCompletionHandler* handler)
{
	auto lastItem = static_cast<CallItem*> (calls.last ());
	ASSERT (lastItem)
	if(lastItem)
	{
		ASSERT (!lastItem->getCompletionHandler ()) // (could also be a list of handlers if needed)
		lastItem->setCompletionHandler (handler);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Promise AsyncSequence::start ()
{
	ASSERT (totalOperation == nullptr)

	currentIndex = -1;
	totalOperation = NEW AsyncSequenceOperation (this);
	SharedPtr<AsyncOperation> returnOperation = totalOperation; // totalOperation might reset during startNext if the processing happens completely synchronously 
	returnOperation->setState (IAsyncInfo::kStarted);
	if(!startNext ())
		finish ();

	return Promise (return_shared<AsyncOperation> (returnOperation));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncSequence::isStarted () const
{
	return totalOperation && totalOperation->getState () == IAsyncInfo::kStarted;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncSequence::resume ()
{
	if(!isStarted ())
	{
		start ();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncSequence::cancel ()
{
	if(!isStarted ())
		return;
	IAsyncOperation* operation = getOperation (currentIndex);
	if(operation)
		operation->cancel ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncSequence::finish ()
{
	if(totalOperation->getState () == IAsyncInfo::kStarted)
		totalOperation->setState (IAsyncInfo::kCompleted);

	currentIndex = -1;
	calls.removeAll ();

	// resolve cyclic reference since AutoPtr does not support reentrance into the release call
	totalOperation.detach ()->release ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncSequence::startNext ()
{
	while(CallItem* item = getItem (currentIndex + 1))
	{
		currentIndex++;
		IAsyncOperation* op = item->getAsyncCall ()->call ();
		if(op)
		{
			item->setOperation (op);
			op->setCompletionHandler (this);
			return true;
		}
		// otherwise continue with next (call only performed synchronously)
	}
	return false; // no more calls
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncSequence::onCompletion (IAsyncOperation& operation)
{
	if(auto item = findItem (operation))
		if(auto handler = item->getCompletionHandler ())
			handler->onCompletion (operation);

	switch(operation.getState ())
	{
	case IAsyncInfo::kCompleted:
		if(!startNext ())
		{
			totalOperation->setResult (operation.getResult ());
			finish ();
		}
		break;

	case IAsyncInfo::kFailed:
		if(isCancelOnError ())
		{
			totalOperation->setState (operation.getState ());
			finish ();
		}
		else if(!startNext ())
		{
			totalOperation->setState (operation.getState ());
			finish ();
		}
		break;

	case IAsyncInfo::kCanceled:
		totalOperation->setState (operation.getState ());
		finish ();
		break;
	}
}
