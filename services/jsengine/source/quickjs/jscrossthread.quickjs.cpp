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
// Filename    : jscrossthread.quickjs.cpp
// Description : QuickJS JavaScript Cross Thread Usage Support
//
//************************************************************************************************

#include "jscrossthread.quickjs.h"
#include "jscontext.quickjs.h"

#include "ccl/public/system/threadlocal.h"

using namespace CCL;
using namespace JScript::QuickJS;

//************************************************************************************************
// JScript::QuickJS::ThreadScope
//************************************************************************************************

static Threading::ThreadLocal<Context*> tlsNativeContext;

//////////////////////////////////////////////////////////////////////////////////////////////////

Context* ThreadScope::getCurrentContext ()
{
	return tlsNativeContext.get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadScope::isCurrentContext (const Context* context)
{
	return getCurrentContext () == context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadScope::ThreadScope (Context* context)
: oldContext (tlsNativeContext.get ())
{
	tlsNativeContext.set (context);

	// re-seed the JSRuntime's stack-overflow guard for the calling thread - see class comment
	if(context)
		JS_UpdateStackTop (JS_GetRuntime (context->getJSContext ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadScope::~ThreadScope ()
{
	tlsNativeContext.set (oldContext);
}
