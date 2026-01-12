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
// Filename    : jsengine.h
// Description : JavaScript Engine
//
//************************************************************************************************

#ifndef _jsengine_h
#define _jsengine_h

#include "ccl/public/plugins/serviceplugin.h"
#include "ccl/public/plugins/iscriptengine.h"

#include "jsinclude.h"

namespace JScript {

class Context;

//************************************************************************************************
// JScript::Engine
//************************************************************************************************

class Engine: public CCL::ServicePlugin,
			  public CCL::Scripting::IEngine,
			  public JSErrorInterceptor
{
public:
	Engine ();

	static IUnknown* createInstance (CCL::UIDRef, void*);

	CCL::Scripting::IEngineHost* getHost () const;

	void onContextDestroyed (Context* context);

	// Scripting::IEngine
	const CCL::FileType& CCL_API getLanguage () const override;
	CCL::tresult CCL_API setOption (CCL::StringID id, CCL::VariantRef value) override;
	CCL::Scripting::IContext* CCL_API createContext () override;

	// JSErrorInterceptor
	void interceptError (JSContext* cx, JS::HandleValue error) override;

	CLASS_INTERFACE (IEngine, ServicePlugin)

protected:
	static constexpr int kDefaultBytesBeforeGC = 32 * 1024 * 1024; // 32 MB

	int bytesBeforeGC;
	int callsBeforeJIT;
	CCL::String debugProtocolId;

	static void gcCallback (JSContext* cx, JSGCStatus status, JS::GCReason reason, void* data);
	static void gcTraceCallback (JSTracer* tracer, void* data);
};

} // namespace JScript

#endif // _jsengine_h
