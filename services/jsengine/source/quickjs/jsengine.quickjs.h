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
// Filename    : jsengine.quickjs.h
// Description : QuickJS JavaScript Engine
//
//************************************************************************************************

#ifndef _jsengine_quickjs_h
#define _jsengine_quickjs_h

#include "../jsenginebase.h"

namespace JScript {
namespace QuickJS {

class Context;

//************************************************************************************************
// JScript::QuickJS::Engine
//************************************************************************************************

class Engine: public JScript::EngineBase
{
public:
	Engine ();

	static CCL::IUnknown* createInstance (CCL::UIDRef, void*);

	void onContextDestroyed (Context* context);

	// EngineBase
	CCL::tresult CCL_API setOption (CCL::StringID id, CCL::VariantRef value) override;
	CCL::Scripting::IContext* CCL_API createContext () override;

protected:
	size_t gcThreshold; ///< passed to JS_SetGCThreshold() for every JSRuntime created by this engine
};

} // namespace QuickJS
} // namespace JScript

#endif // _jsengine_quickjs_h
