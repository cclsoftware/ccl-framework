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
// Filename    : jsengine.spidermonkey.h
// Description : SpiderMonkey JavaScript Engine
//
//************************************************************************************************

#ifndef _jsengine_spidermonkey_h
#define _jsengine_spidermonkey_h

#include "../jsenginebase.h"

#include "jsinclude.spidermonkey.h"

namespace JScript {
namespace SpiderMonkey {

class Context;

//************************************************************************************************
// JScript::SpiderMonkey::Engine
//************************************************************************************************

class Engine: public JScript::EngineBase,
			  public JSErrorInterceptor
{
public:
	Engine ();

	static IUnknown* createInstance (CCL::UIDRef, void*);

	void onContextDestroyed (Context* context);

	// EngineBase
	CCL::tresult CCL_API setOption (CCL::StringID id, CCL::VariantRef value) override;
	CCL::Scripting::IContext* CCL_API createContext () override;

	// JSErrorInterceptor
	void interceptError (JSContext* cx, JS::HandleValue error) override;

protected:
	int bytesBeforeGC;
	int callsBeforeJIT;
	CCL::String debugProtocolId;

	static void gcCallback (JSContext* cx, JSGCStatus status, JS::GCReason reason, void* data);
	static void gcTraceCallback (JSTracer* tracer, void* data);
};

} // namespace SpiderMonkey
} // namespace JScript

#endif // _jsengine_spidermonkey_h
