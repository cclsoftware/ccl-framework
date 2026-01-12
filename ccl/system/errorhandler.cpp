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
// Filename    : ccl/system/errorhandler.cpp
// Description : Error Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/errorhandler.h"

#include "ccl/base/storage/logfile.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/system/threadlocal.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ErrorContext
//************************************************************************************************

class ErrorContext: public Object,
					public IErrorContext
{
public:
	ErrorContext ();

	void addEvent (LogEvent* e);
	void addChild (ErrorContext* c);

	// IErrorContext
	int CCL_API getEventCount () const override;
	AlertEventRef CCL_API getEvent (int index) const override;
	int CCL_API getChildCount () const override;
	IErrorContext* CCL_API getChild (int index) const override;
	void CCL_API removeAll () override;

	CLASS_INTERFACE (IErrorContext, Object)

protected:
	LogEventList eventList;
	Alert::Event emptyEvent;
	ObjectArray children;
};

//************************************************************************************************
// ThreadErrorHandler
//************************************************************************************************

class ThreadErrorHandler: public Object,
						  public Threading::ThreadSingleton<ThreadErrorHandler>
{
public:
	ThreadErrorHandler ();
	~ThreadErrorHandler ();

	void report (AlertEventRef e);
	bool begin ();
	bool end ();
	ErrorContext* peek () const;
	int depth () const;
	void push ();

protected:
	ErrorContext& root;
	Stack<ErrorContext*> stack;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IErrorHandler& CCL_API System::CCL_ISOLATED (GetErrorHandler) ()
{
	return ErrorHandler::instance ();
}

//************************************************************************************************
// ThreadErrorHandler
//************************************************************************************************

DEFINE_THREAD_SINGLETON (ThreadErrorHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadErrorHandler::ThreadErrorHandler ()
: root (*NEW ErrorContext)
{
	CCL_PRINTF ("ThreadErrorHandler ctor for thread %d\n", System::GetThreadSelfID ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadErrorHandler::~ThreadErrorHandler ()
{
	CCL_PRINTF ("ThreadErrorHandler dtor for thread %d\n", System::GetThreadSelfID ())
	root.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadErrorHandler::report (AlertEventRef _e)
{
	ErrorContext* c = peek ();
	if(c == nullptr)
		return;

	LogEvent* e = NEW LogEvent (_e);
	if(e->time == DateTime ())
		System::GetSystem ().getLocalTime (e->time);

	c->addEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadErrorHandler::begin ()
{
	ErrorContext* parent = peek ();
	if(parent == nullptr)
		parent = &root;

	ErrorContext* c = NEW ErrorContext;
	parent->addChild (c);
	stack.push (c);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadErrorHandler::end ()
{
	ASSERT (peek () != nullptr)
	if(peek () == nullptr)
		return false;

	stack.pop ();

	if(stack.isEmpty ())
		root.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorContext* ThreadErrorHandler::peek () const
{
	return stack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ThreadErrorHandler::depth () const
{
	return stack.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadErrorHandler::push ()
{
	int count = stack.count ();
	if(count >= 2)
	{
		ErrorContext* child = stack.at (0);
		ASSERT (child == stack.peek ())
		ErrorContext* parent = stack.at (1);
		for(int i = 0; i < child->getEventCount (); i++)
			parent->addEvent (NEW LogEvent (child->getEvent (i)));
	}
}

//************************************************************************************************
// ErrorHandler
//************************************************************************************************

ErrorHandler& ErrorHandler::instance ()
{
	static ErrorHandler theErrorHandler; // construction is not thread-safe (=> use "One-Time Initialization")!
	return theErrorHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ErrorHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ErrorHandler::reportEvent (const Alert::Event& e)
{
	ThreadErrorHandler::instance ().report (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ErrorHandler::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ErrorHandler::beginContext ()
{
	return ThreadErrorHandler::instance ().begin () ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ErrorHandler::endContext ()
{
	return ThreadErrorHandler::instance ().end () ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IErrorContext* CCL_API ErrorHandler::peekContext ()
{
	return ThreadErrorHandler::instance ().peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ErrorHandler::getContextDepth ()
{
	return ThreadErrorHandler::instance ().depth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ErrorHandler::pushToParent (IErrorContext* context)
{
	ASSERT (context == peekContext ())
	if(context != peekContext ())
		return kResultUnexpected;
	ThreadErrorHandler::instance ().push ();
	return kResultOk;
}

//************************************************************************************************
// ErrorContext
//************************************************************************************************

ErrorContext::ErrorContext ()
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContext::addEvent (LogEvent* e)
{
	eventList.getEvents ().add (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContext::addChild (ErrorContext* c)
{
	children.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ErrorContext::getEventCount () const
{
	return eventList.getEvents ().count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertEventRef CCL_API ErrorContext::getEvent (int index) const
{
	LogEvent* e = (LogEvent*)eventList.getEvents ().at (index);
	if(e)
		return *e;
	return emptyEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ErrorContext::getChildCount () const
{
	return children.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IErrorContext* CCL_API ErrorContext::getChild (int index) const
{
	return (ErrorContext*)children.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ErrorContext::removeAll ()
{
	eventList.getEvents ().removeAll ();
	children.removeAll ();
}
