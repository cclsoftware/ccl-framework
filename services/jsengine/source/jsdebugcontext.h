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
// Filename    : jsdebugcontext.h
// Description : JavaScript Debug Context
//
//************************************************************************************************

#ifndef _jsdebugcontext_h
#define _jsdebugcontext_h

#include "jscontext.h"

namespace JScript {

//************************************************************************************************
// JScript::DebugContext
//************************************************************************************************

class DebugContext: public Context,
                    public CCL::IDebuggable
{
public:
	DECLARE_CLASS_ABSTRACT (DebugContext, Context)

	DebugContext (Engine& engine, ::JSContext* context, CCL::StringRef debugProtocolId);
	~DebugContext ();

	// Context
	void initialize () override;

	// IDebuggable
	void CCL_API setThreadId (int threadId) override;
	int CCL_API getThreadId () const override;
	void CCL_API setSender (CCL::IDebugMessageSender* sender) override;
	void CCL_API receiveMessage (const CCL::IDebugMessage& message) override;
	CCL::StringRef CCL_API getName () const override;
	void CCL_API onDisconnected () override;

	CLASS_INTERFACE (IDebuggable, Context)

protected:
	CCL::IDebugMessageSender* debugMessageSender;

	JS::PersistentRootedObject debuggerGlobal;
	JS::PersistentRootedObject scriptRealm;

	Realm* debuggerRealm;

	int threadId;
	bool executionHalted;
	bool useDebuggerGlobal;
	CCL::String debugProtocolId;

	static bool printLineCallback (JSContext* cx, unsigned argc, JS::Value* vp);
	static bool pauseCallback (JSContext* cx, unsigned argc, JS::Value* vp);
	static bool sendDebugMessage (JSContext* cx, unsigned argc, JS::Value* vp);
};

} // namespace JScript

#endif // _jsdebugcontext_h
