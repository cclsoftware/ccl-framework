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
// Filename    : jsengine.cpp
// Description : JavaScript Engine
//
//************************************************************************************************

#define DISABLE_JIT (0 && DEBUG)
#define FORCE_JIT (0 && DEBUG)

#include "jsengine.h"
#include "jsdebugcontext.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/storage/filetype.h"

using namespace CCL;
using namespace Scripting;
using namespace JScript;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool initialized = false;

CCL_KERNEL_INIT (JSEngine)
{
	if(!initialized)
		initialized = JS_Init ();
	return initialized;
}

CCL_KERNEL_TERM (JSEngine)
{
	#if !(CCL_PLATFORM_ANDROID || CCL_PLATFORM_MAC || CCL_PLATFORM_IOS) // on Mac and iOS jsengine is not succesfully unloaded (this is a leak)
	if(initialized)
		JS_ShutDown ();
	#endif
}

//************************************************************************************************
// JScript::Engine
//************************************************************************************************

void Engine::gcCallback (JSContext* cx, JSGCStatus status, JS::GCReason reason, void* data)
{
	if(status == JSGC_END)
	{
		if(Context* context = ThreadScope::getCurrentContext ())
			context->onGCFinished ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::gcTraceCallback (JSTracer* tracer, void* data)
{
	if(Context* context = ThreadScope::getCurrentContext ())
		context->tracePropertyAccessors (tracer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Engine::createInstance (UIDRef, void*)
{
	return static_cast<IComponent*> (NEW Engine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Engine::Engine ()
: bytesBeforeGC (kDefaultBytesBeforeGC),
  callsBeforeJIT (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Engine::setOption (StringID id, VariantRef value)
{
	if(id == kGCThreshold)
	{
		bytesBeforeGC = value.asInt ();
		return kResultOk;
	}
	else if(id == kJITThreshold)
	{
		callsBeforeJIT = value.asInt ();
		return kResultOk;
	}
	else if(id == kDebugProtocolID)
	{
		debugProtocolId = value.asString ();
		return kResultOk;
	}

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEngineHost* Engine::getHost () const
{
	return UnknownPtr<IEngineHost> (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API Engine::getLanguage () const
{
	static FileType jsFileType ("JavaScript", "js", kJavaScript);
	return jsFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct EnvironmentPreparer: public js::ScriptEnvironmentPreparer
{
	explicit EnvironmentPreparer(JSContext* cx)
	: cx (cx)
	{
		js::SetScriptEnvironmentPreparer (cx, this);
	}

	void invoke(JS::HandleObject global, Closure& closure) override;

	JSContext* cx;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void EnvironmentPreparer::invoke (JS::HandleObject global, Closure& closure)
{
	ASSERT (JS_IsGlobalObject (global));
	ASSERT (!JS_IsExceptionPending (cx));

	if(!closure (cx))
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IContext* CCL_API Engine::createContext ()
{
	ThreadScope guard (nullptr);

	JSContext* cx = JS_NewContext (bytesBeforeGC);
	if(!cx)
		return nullptr;

	if(callsBeforeJIT >= 0)
	{
		JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_BASELINE_INTERPRETER_WARMUP_TRIGGER, callsBeforeJIT);
		JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_BASELINE_WARMUP_TRIGGER, callsBeforeJIT);
		JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_ION_NORMAL_WARMUP_TRIGGER, callsBeforeJIT);
	}

	#if DEBUG
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_FULL_DEBUG_CHECKS, 1);
	#endif
	#if DISABLE_JIT
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_BASELINE_ENABLE, 0);
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_ION_ENABLE, 0);
	#endif
	#if FORCE_JIT
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_BASELINE_INTERPRETER_WARMUP_TRIGGER, 0);
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_BASELINE_WARMUP_TRIGGER, 0);
	JS_SetGlobalJitCompilerOption (cx, JSJitCompilerOption::JSJITCOMPILER_ION_NORMAL_WARMUP_TRIGGER, 0);
	#endif

	js::UseInternalJobQueues (cx);
	if(!JS::InitSelfHostedCode (cx))
	{
		JS_DestroyContext (cx);
		return nullptr;
	}

	EnvironmentPreparer preparer (cx);

	JS_SetGCCallback (cx, &gcCallback, nullptr);
	JS_AddExtraGCRootsTracer (cx, &gcTraceCallback, nullptr);

	JS_SetErrorInterceptorCallback (JS_GetRuntime (cx), this);
	ASSERT (JS_GetErrorInterceptorCallback (JS_GetRuntime (cx))) // fails if SpiderMonkey was built without NIGHTLY_BUILD macro defined

	Context* context = nullptr;
	if(!debugProtocolId.isEmpty ())
		context = NEW DebugContext (*this, cx, debugProtocolId);
	else
		context = NEW Context (*this, cx);

	context->initialize ();
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::onContextDestroyed (Context* context)
{
	ASSERT (context)	
	JSContext* cx = context->getJSContext ();
	JS_RemoveExtraGCRootsTracer (cx, &gcTraceCallback, nullptr);
	JS_DestroyContext (cx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::interceptError (JSContext* cx, JS::HandleValue errorValue)
{
	if(Context* context = Context::getNativeContext (cx))
		context->reportError (errorValue);
}
