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
// Filename    : ccl/base/asyncoperation.h
// Description : Asynchronous operation and Promise
//
//************************************************************************************************

#ifndef _ccl_asyncoperation_h
#define _ccl_asyncoperation_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {

//************************************************************************************************
// AsyncOperation
//************************************************************************************************

class AsyncOperation: public Object,
					  public IAsyncOperation
{
public:
	DECLARE_CLASS (AsyncOperation, Object)

	AsyncOperation ();
	~AsyncOperation ();

	static AsyncOperation* createCompleted (VariantRef result = Variant (), bool deferred = false);
	static AsyncOperation* createFailed (bool deferred = false);
	static void deferDestruction (IAsyncOperation* operation);

	virtual void setState (State state);
	virtual void setStateDeferred (State state);

	// IAsyncOperation
	void CCL_API setResult (VariantRef value) override;
	State CCL_API getState () const override;
	Variant CCL_API getResult () const override;
	void CCL_API cancel () override;
	void CCL_API close () override;
	void CCL_API setCompletionHandler (IAsyncCompletionHandler* handler) override;
	void CCL_API setProgressHandler (IProgressNotify* handler) override;
	IProgressNotify* CCL_API getProgressHandler () const override;

	CLASS_INTERFACE2 (IAsyncOperation, IAsyncInfo, Object)

private:
	State state;
	Variant result;
	SharedPtr<IAsyncCompletionHandler> completionHandler;
	IProgressNotify* progressHandler;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// AsyncCompletionHandler
//************************************************************************************************

class AsyncCompletionHandler: public Unknown,
							  public IAsyncCompletionHandler
{
public:
	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override {}

	CLASS_INTERFACE (IAsyncCompletionHandler, Unknown)
};

//************************************************************************************************
// AsyncCompletionOperation
//************************************************************************************************

class AsyncCompletionOperation: public AsyncOperation,
								public IAsyncCompletionHandler
{
public:
	AsyncCompletionOperation (IAsyncOperation* operation, IAsyncCompletionHandler* handler)
	: originalOperation (operation),
	  handler (handler)
	{}

	// AsyncOperation
	void CCL_API cancel () override
	{
		if(originalOperation)
			originalOperation->cancel ();
	}

	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		setState (kStarted);

		if(handler)
			handler->onCompletion (operation);
		
		setResult (operation.getResult ());
		setState (operation.getState ());

		originalOperation.release ();
	}

	CLASS_INTERFACE (IAsyncCompletionHandler, AsyncOperation)

private:
	SharedPtr<IAsyncOperation> originalOperation;
	SharedPtr<IAsyncCompletionHandler> handler;
};

//************************************************************************************************
// MemberFuncCompletionHandler
//************************************************************************************************

template <class T>
class MemberFuncCompletionHandler: public AsyncCompletionHandler
{
public:
	typedef void (T::*MemberFunc) (IAsyncOperation&);

	MemberFuncCompletionHandler (T* target, MemberFunc memberFunc)
	: target (target),
	  memberFunc (memberFunc)
	{}

protected:
	T* target;
	MemberFunc memberFunc;

	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		(target->*memberFunc) (operation);
	}
};

//************************************************************************************************
// LambdaCompletionHandler
//************************************************************************************************

template <typename T>
class LambdaCompletionHandler: public AsyncCompletionHandler
{
public:
	LambdaCompletionHandler (const T& lambda)
	: lambda (lambda)
	{}

protected:
	T lambda; ///< lambda instance is copied here!

	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		lambda (operation);
	}
};

//************************************************************************************************
// IAsyncStateModifier
//************************************************************************************************

struct IAsyncStateModifier
{
	virtual IAsyncInfo::State modifyState (const IAsyncOperation& operation) = 0;
	virtual ~IAsyncStateModifier () {}
};

//************************************************************************************************
// LambdaStateModifier
//************************************************************************************************

template <typename T>
class LambdaStateModifier: public IAsyncStateModifier
{
public:
	LambdaStateModifier (const T& lambda)
	: lambda (lambda)
	{}

protected:
	T lambda; ///< lambda instance is copied here!

	// IAsyncStateModifier
	IAsyncInfo::State modifyState (const IAsyncOperation& operation) override
	{
		return lambda (operation);
	}
};

//************************************************************************************************
// MemberFuncStateModifier
//************************************************************************************************

template <class T>
class MemberFuncStateModifier: public IAsyncStateModifier
{
public:
	typedef IAsyncInfo::State (T::*MemberFunc) (IAsyncOperation&);

	MemberFuncStateModifier (T* target, MemberFunc memberFunc)
	: target (target),
	  memberFunc (memberFunc)
	{}

protected:
	T* target;
	MemberFunc memberFunc;

	// IAsyncStateModifier
	IAsyncInfo::State CCL_API modifyState (const IAsyncOperation& operation) override
	{
		return (target->*memberFunc) (operation);
	}
};

//************************************************************************************************
// AsyncStateChangeOperation
//************************************************************************************************

class AsyncStateChangeOperation: public AsyncOperation,
								 public IAsyncCompletionHandler
{
public:
	AsyncStateChangeOperation (IAsyncOperation* operation, IAsyncStateModifier* modifier)
	: originalOperation (operation),
	  modifier (modifier)
	{}

	~AsyncStateChangeOperation ()
	{
		delete modifier;
	}

	// AsyncOperation
	void CCL_API cancel () override
	{
		if(originalOperation)
			originalOperation->cancel ();
	}

	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		setState (kStarted);

		setResult (operation.getResult ());
		if(modifier)
		{
			IAsyncInfo::State newState = modifier->modifyState (operation);
			setState (newState);
		}

		originalOperation.release ();
	}

	CLASS_INTERFACE (IAsyncCompletionHandler, AsyncOperation)

private:
	SharedPtr<IAsyncOperation> originalOperation;
	IAsyncStateModifier* modifier;
};

//************************************************************************************************
// Promise
//************************************************************************************************

class Promise
{
public:
	Promise (IAsyncOperation* operation); ///< takes ownership!
	Promise (const Promise& promise);

	Promise then (IAsyncCompletionHandler* handler);
	template <typename T> Promise then (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc);
	template <typename T> Promise then (const T& lambda);

	Promise modifyState (IAsyncStateModifier* modifier);
	template <typename T> Promise modifyState (const T& lambda);
	template <typename T> Promise modifyState (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc);

	operator IAsyncOperation* () const		{ return asyncOperation; }
	IAsyncOperation* operator -> () const	{ return asyncOperation; }

private:
	AutoPtr<IAsyncOperation> asyncOperation;
};

//************************************************************************************************
// AsyncSequence
//************************************************************************************************

class AsyncSequence: public AsyncCompletionHandler
{
public:
	AsyncSequence ();
	~AsyncSequence ();

	PROPERTY_BOOL (cancelOnError, CancelOnError)

	int add (IAsyncCall* call); // returns index of call (e.g. for getResult); sequence owns call
	void then (IAsyncCompletionHandler* handler); // will be exeuted after the operation of the most recently added call completes

	template<typename T> int add (const T& lambda);
	template<typename T> void then (const T& lambda);
	template <typename T> void then (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc);

	Promise start ();
	bool isStarted () const;
	void resume ();
	void cancel ();

	bool isEmpty () const;
	int getCount () const;
	Variant getResult (int index) const;
	IAsyncOperation* getOperation (int index) const;

private:
	ObjectArray calls;
	int currentIndex;
	AutoPtr<AsyncOperation> totalOperation;

	class CallItem: public Object
	{
	public:
		PROPERTY_AUTO_POINTER (IAsyncCall, call, AsyncCall)
		PROPERTY_AUTO_POINTER (IAsyncOperation, operation, Operation)
		PROPERTY_SHARED_AUTO (IAsyncCompletionHandler, completionHandler, CompletionHandler)
	};

	CallItem* getItem (int index) const;
	CallItem* findItem (const IAsyncOperation& operation) const;

	bool startNext ();
	void finish ();

	// AsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override;
};

//************************************************************************************************
// AsyncCall
//************************************************************************************************

class AsyncCall: public Unknown,
				 public IAsyncCall
{
public:
	CLASS_INTERFACE (IAsyncCall, Unknown)

	template <typename T> 
	static AutoPtr<IAsyncCall> make (const T& lambda);
};

//************************************************************************************************
// AsyncLambdaCall
//************************************************************************************************

template <typename T>
class AsyncLambdaCall: public AsyncCall
{
public:
	AsyncLambdaCall (const T& lambda)
	: lambda (lambda)
	{}

	// AsyncCall
	IAsyncOperation* CCL_API call () override
	{
		return lambda ();
	}

protected:
	T lambda; ///< lambda instance is copied here!
};

//************************************************************************************************
// AsyncStep
//************************************************************************************************

class AsyncStep: public Object
{
public:
	AsyncStep (AsyncSequence* sequence)
	: sequence (sequence)
	{}

	void start ()
	{
		sequence->add ([this] () { return call->call (); });
		sequence->then (static_cast<IAsyncCompletionHandler*> (completionHandler));
	}

	template <typename T>
	void onStart (const T& lambda)
	{
		call = AsyncCall::make (lambda);
	}

	template <typename T>
	void onCompletion (const T& lambda)
	{
		completionHandler = NEW LambdaCompletionHandler<T> (lambda);
	}

protected:
	AsyncSequence* sequence;
	AutoPtr<IAsyncCall> call;
	AutoPtr<IAsyncCompletionHandler> completionHandler;
};

//************************************************************************************************
// AsyncStepMachine
//************************************************************************************************

class AsyncStepMachine: public Unknown
{
public:
	AsyncStepMachine ()
	: sequence (NEW AsyncSequence)
	{
		steps.objectCleanup (true);
	}

	AsyncStep* createStep ()
	{
		auto step = NEW AsyncStep (sequence);
		steps.add (step);
		return step;
	}

	IAsyncOperation* start (AsyncStep* step)
	{
		step->start ();
		SharedPtr<AsyncStepMachine> self (this);
		return return_shared<IAsyncOperation> (sequence->start ().then ([self] (IAsyncOperation& op) { /* NOOP, keep self from early deconstruction */ }));
	}

protected:
	ObjectArray steps;
	AutoPtr<AsyncSequence> sequence;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Promise inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Promise::Promise (IAsyncOperation* operation)
: asyncOperation (operation)
{
	if(!asyncOperation)
	{
		AsyncOperation* failed = NEW AsyncOperation;
		failed->setState (AsyncOperation::kFailed);
		asyncOperation = failed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Promise::Promise (const Promise& promise)
{
	asyncOperation.share (promise.asyncOperation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Promise Promise::then (IAsyncCompletionHandler* handler)
{
	AsyncCompletionOperation* completionOperation = NEW AsyncCompletionOperation (asyncOperation, handler);
	asyncOperation->setCompletionHandler (completionOperation);
	return Promise (completionOperation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Promise Promise::then (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc)
{
	ASSERT (target && memberFunc)
	AutoPtr<IAsyncCompletionHandler> handler (NEW MemberFuncCompletionHandler<T> (target, memberFunc));
	return then (static_cast<IAsyncCompletionHandler*> (handler));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Promise Promise::then (const T& lambda)
{
	AutoPtr<IAsyncCompletionHandler> handler (NEW LambdaCompletionHandler<T> (lambda));
	return then (static_cast<IAsyncCompletionHandler*> (handler));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Promise Promise::modifyState (IAsyncStateModifier* modifier)
{
	ASSERT (modifier)
	AsyncStateChangeOperation* completionOperation = NEW AsyncStateChangeOperation (asyncOperation, modifier);
	asyncOperation->setCompletionHandler (completionOperation);
	return Promise (completionOperation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Promise Promise::modifyState (const T& lambda)
{
	return modifyState (static_cast<IAsyncStateModifier*> (NEW LambdaStateModifier<T> (lambda)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Promise Promise::modifyState (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc)
{
	ASSERT (target && memberFunc)
	AutoPtr<IAsyncCompletionHandler> handler (NEW MemberFuncStateModifier<T> (target, memberFunc));
	return then (static_cast<IAsyncCompletionHandler*> (handler));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncCall inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> 
AutoPtr<IAsyncCall> AsyncCall::make (const T& lambda)
{
	return AutoPtr<IAsyncCall> (NEW AsyncLambdaCall<T> (lambda));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncSequence inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline int AsyncSequence::add (const T& lambda)
{
	AutoPtr<IAsyncCall> call (AsyncCall::make (lambda));
	return add (call.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline void AsyncSequence::then (const T& lambda)
{
	AutoPtr<IAsyncCompletionHandler> handler (NEW LambdaCompletionHandler<T> (lambda));
	then (static_cast<IAsyncCompletionHandler*> (handler));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void AsyncSequence::then (T* target, typename MemberFuncCompletionHandler<T>::MemberFunc memberFunc)
{
	ASSERT (target && memberFunc)
	AutoPtr<IAsyncCompletionHandler> handler (NEW MemberFuncCompletionHandler<T> (target, memberFunc));
	then (static_cast<IAsyncCompletionHandler*> (handler));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_asyncoperation_h
