//************************************************************************************************
//
// JavaScript Engine
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : jscrossthread.quickjs.h
// Description : QuickJS JavaScript Cross Thread Usage Support
//
//************************************************************************************************

#ifndef _jscrossthread_quickjs_h
#define _jscrossthread_quickjs_h

#include "quickjs.h"

namespace JScript {
namespace QuickJS {

class Context;

//************************************************************************************************
// JScript::ThreadScope
/** QuickJS-ng takes JSContext* and JSRuntime* explicitly in every API call, so no per-thread JSContext 
	swapping is needed. However, a JSRuntime's stack-overflow guard is seeded from the stack pointer of 
	whichever thread called JS_NewRuntime() (see JS_UpdateStackTop() in quickjs.h) - if a Context is later
	entered from a *different* thread (e.g. a Context created on the main thread but driven from a
	worker-pool thread), that guard must be re-seeded for the new thread's stack, or the interpreter's
	stack-depth checks operate against a meaningless baseline. The constructor here re-seeds it via
	JS_UpdateStackTop() every time a Context is entered, in addition to tracking which Context "owns"
	the calling thread (so code can assert against accidental *concurrent* cross-thread usage - see
	IContext: "usually one per thread"). */
//************************************************************************************************

class ThreadScope
{
public:
	ThreadScope (Context* context);
	~ThreadScope ();

	static Context* getCurrentContext ();
	static bool isCurrentContext (const Context* context); // check that context belongs to current thread

private:
	Context* oldContext;
};

} // namespace QuickJS
} // namespace JScript

#endif // _jscrossthread_quickjs_h
