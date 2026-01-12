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
// Filename    : jsdebugcontext.cpp
// Description : JavaScript Debug Context
//
//************************************************************************************************

#include "jsdebugcontext.h"
#include "jsengine.h"
#include "js/Debug.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/ibuffer.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Scripting;
using namespace JScript;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Interface between debug context and protocol handler script
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace DebugInterface
{
	// Handler Script -> JSContext

	/* sendDebugMessage: (message: string) => void
	Send a debug message constructed from raw data to the attached debug client (IDE). */
	DEFINE_STRINGID_ (kSendDebugMessage, "sendDebugMessage")

	/* pause: (state: boolean) => void
	When invoked with state true, halt program execution on main thread until invoked with state false. */
	DEFINE_STRINGID_ (kPause, "pause")

	/* println: (message: string) => void
	Print a message to the console for debug purposes. */
	DEFINE_STRINGID_ (kPrintln, "println")


	// JSContext -> Handler Script

	/* onDebugMessage: (data: string, threadId: number) => void
	Handle a debug message as raw data. */
	DEFINE_STRINGID_ (kOnDebugMessage, "onDebugMessage")
}

//************************************************************************************************
// JScript::DebugContext
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (DebugContext, Context)

//////////////////////////////////////////////////////////////////////////////////////////////////

DebugContext::DebugContext (Engine& engine, ::JSContext* context, StringRef debugProtocolId)
: Context (engine, context),
  debuggerGlobal (context),
  debuggerRealm (NEW Realm (this)),
  threadId (-1),
  debugMessageSender (nullptr),
  executionHalted (false),
  useDebuggerGlobal (false),
  debugProtocolId (debugProtocolId)
{
	static JSClass debuggerGlobalClass =
	{
		"global",
		JSCLASS_GLOBAL_FLAGS,
		&JS::DefaultGlobalClassOps
	};

	JS::RealmOptions options;
	debuggerGlobal = JS_NewGlobalObject (context, &debuggerGlobalClass, nullptr, JS::DontFireOnNewGlobalHook, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DebugContext::~DebugContext ()
{
	ThreadScope guard (this);
	debuggerGlobal = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DebugContext::initialize ()
{
	if(globalInitialized)
	{
		ASSERT (false)
		return;
	}

	String protocolHandlerFileName;
	if(debugProtocolId == "dap")
		protocolHandlerFileName = "daphandler.js";

	if(protocolHandlerFileName.isEmpty ())
	{
		ASSERT (false)
		return;
	}

	RealmScope debuggerGuard (this, debuggerGlobal);
	JS_DefineObject (context, debuggerGlobal, "exports"); // allow loading modules
	if(AutoPtr<Scripting::IScript> script = System::GetScriptingManager ().loadScript (ResourceUrl (protocolHandlerFileName, Url::kFile)))
	{
		JS::SetRealmPrivate (JS::GetObjectRealmOrNull (debuggerGlobal), debuggerRealm);
		JS::SetDestroyRealmCallback (context, &destroyRealmCallback);
		JS_DefineDebuggerObject (context, debuggerGlobal);

		JS_DefineFunction (context, debuggerGlobal, DebugInterface::kPrintln, &printLineCallback, 1, 0);
		JS_DefineFunction (context, debuggerGlobal, DebugInterface::kPause, &pauseCallback, 1, 0);
		JS_DefineFunction (context, debuggerGlobal, DebugInterface::kSendDebugMessage, &sendDebugMessage, 1, 0);

		Variant returnValue;
		executeScriptInternal (returnValue, *script);
	}
	else
	{
		ASSERT (false)
	}

	Context::initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DebugContext::setThreadId (int _threadId)
{
	threadId = _threadId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DebugContext::getThreadId () const
{
	return threadId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DebugContext::setSender (IDebugMessageSender* sender)
{
	debugMessageSender = sender;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DebugContext::receiveMessage (const IDebugMessage& message)
{
	RealmScope guard (this, debuggerGlobal);
	if(!guard.isValid ())
		return;

	String data;
	message.getRawData (data);
	JS::RootedValueArray<2> argArray (context);
	ScriptArguments::fromVariant (argArray[0], data, context);
	ScriptArguments::fromVariant (argArray[1], threadId, context);
	JS::RootedValue retval (context);
	JS_CallFunctionName (context, debuggerGlobal, DebugInterface::kOnDebugMessage, argArray, &retval);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DebugContext::getName () const
{
	return engine.getLanguage ().getDescription ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DebugContext::onDisconnected ()
{
	executionHalted = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugContext::printLineCallback (JSContext* cx, unsigned argc, JS::Value* vp)
{
	String message;
	getStringArgument (message, cx, argc, vp);
	Debugger::println (message);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugContext::pauseCallback (JSContext* cx, unsigned argc, JS::Value* vp)
{
	// SignalHandler::flush () only works on main thread
	if(!System::IsInMainThread ())
		return false;

	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);
	Variant argValue;
	if(argc >= 1)
		ScriptArguments::toVariant (argValue, args.get (0), cx);
	else
	{
		ASSERT (false)
		return false;
	}

	DebugContext* This = ccl_cast<DebugContext> (Context::getNativeContext (cx));
	if(argValue.asBool () == false)
	{
		ASSERT (This->executionHalted)
		This->executionHalted = false;
		return true;
	}

	ASSERT (This->executionHalted == false)
	This->executionHalted = true;
	while(This->executionHalted)
	{
		System::ThreadSleep (50);
		System::GetSignalHandler ().flush ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugContext::sendDebugMessage (JSContext* cx, unsigned argc, JS::Value* vp)
{
	DebugContext* This = ccl_cast<DebugContext> (Context::getNativeContext (cx));
	if(This->debugMessageSender)
	{
		String data;
		getStringArgument (data, cx, argc, vp);
		if(AutoPtr<IDebugMessage> message = This->debugMessageSender->createMessage (data))
			return This->debugMessageSender->sendMessage (*message);
	}

	return false;
}
