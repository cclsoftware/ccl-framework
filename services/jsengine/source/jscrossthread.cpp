//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jscrossthread.cpp
// Description : JavaScript Cross Thread Usage Support
//
//************************************************************************************************

#include "jscrossthread.h"
#include "jscontext.h"

#include "ccl/public/system/threadlocal.h"

namespace js
{
extern MOZ_THREAD_LOCAL (JSContext*) TlsContext;
extern MOZ_THREAD_LOCAL (JS::GCContext*) TlsGCContext;
}

using namespace CCL;
using namespace JScript;

//************************************************************************************************
// JScript::ThreadScope
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
: oldContext (tlsNativeContext.get ()),
  oldJSContext (js::TlsContext.get ()),
  oldGCContext (js::TlsGCContext.get ())
{
	tlsNativeContext.set (context);

	// ensure TLS storage is initialized for the current thread
	if(!js::TlsContext.initialized ())
		(void)js::TlsContext.init ();

	if(!js::TlsGCContext.initialized ())
		(void)js::TlsGCContext.init ();

	js::TlsContext.set (context ? context->getJSContext () : nullptr);
	js::TlsGCContext.set (context ? context->getGCContext () : nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadScope::~ThreadScope ()
{
	tlsNativeContext.set (oldContext);
	
	js::TlsContext.set (oldJSContext);
	js::TlsGCContext.set (oldGCContext);
}
