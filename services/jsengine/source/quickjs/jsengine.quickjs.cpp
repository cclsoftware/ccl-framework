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
// Filename    : jsengine.quickjs.cpp
// Description : QuickJS JavaScript Engine
//
//************************************************************************************************

#include "jsengine.quickjs.h"
#include "jscontext.quickjs.h"

using namespace CCL;
using namespace Scripting;
using namespace JScript::QuickJS;

//************************************************************************************************
// JScript::QuickJS::Engine
//************************************************************************************************

IUnknown* Engine::createInstance (UIDRef, void*)
{
	return static_cast<IComponent*> (NEW Engine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Engine::Engine ()
: gcThreshold (kDefaultGCThreshold)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Engine::setOption (StringID id, VariantRef value)
{
	if(id == kGCThreshold)
	{
		gcThreshold = size_t(value.asInt ());
		return kResultOk;
	}
	else if(id == kJITThreshold)
	{
		// quickjs-ng is interpreter-only, there is no JIT warmup to configure; accepted as a no-op
		// for API compatibility with the SpiderMonkey backend.
		return kResultOk;
	}
	else if(id == kDebugProtocolID)
	{
		// No debug-protocol support for the quickjs backend yet: never construct a DebugContext.
		return kResultNotImplemented;
	}

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IContext* CCL_API Engine::createContext ()
{
	// Always install the logging allocator (mirrors the SpiderMonkey backend's unconditionally
	// installed js::logMalloc/etc. hooks, see jscontext.quickjs.cpp) - it delegates to the plain C
	// allocator and only logs when a thread has been flagged via setOption(kLogMemoryAllocations).
	JSRuntime* runtime = JS_NewRuntime2 (&kAllocFunctions, nullptr);
	if(!runtime)
		return nullptr;

	JS_SetGCThreshold (runtime, gcThreshold);

	#if JSENGINE_STACK_SIZE
	// QuickJS-ng's own soft stack-overflow check assumes JS_DEFAULT_STACK_SIZE (1 MB) unless told
	// otherwise. On targets with a smaller real stack, JSENGINE_STACK_SIZE (see jsengine-config.cmake
	// / jsengine.<platform>.cmake) caps it accordingly, so the check fires before the real stack is
	// exhausted rather than the process crashing outright.
	JS_SetMaxStackSize (runtime, JSENGINE_STACK_SIZE);
	#endif

	JSContext* cx = JS_NewContext (runtime); // adds all standard intrinsics automatically
	if(!cx)
	{
		JS_FreeRuntime (runtime);
		return nullptr;
	}

	Context* newContext = NEW Context (*this, cx);
	newContext->initialize ();
	return newContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::onContextDestroyed (Context* context)
{
	ASSERT (context)
	JSContext* cx = context->getJSContext ();
	JSRuntime* runtime = JS_GetRuntime (cx);
	JS_FreeContext (cx);
	JS_FreeRuntime (runtime);
}
