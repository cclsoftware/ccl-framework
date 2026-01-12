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
// Filename    : jscrossthread.h
// Description : JavaScript Cross Thread Usage Support
//
//************************************************************************************************

#ifndef _jscrossthread_h
#define _jscrossthread_h

#include "jsinclude.h"

namespace JScript {

class Context;

//************************************************************************************************
// JScript::ThreadScope
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
	JSContext* oldJSContext;
	JS::GCContext* oldGCContext;
};

} // namespace JScript

#endif // _jscrossthread_h
