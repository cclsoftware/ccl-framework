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
// Filename    : jsengine.h
// Description : JavaScript Engine - backend dispatch
//
//************************************************************************************************

#ifndef _jsengine_h
#define _jsengine_h

#if JSENGINE_USE_QUICKJS
	#include "quickjs/jsengine.quickjs.h"
#else
	#include "spidermonkey/jsengine.spidermonkey.h"
#endif

namespace JScript {

#if JSENGINE_USE_QUICKJS
using Engine = QuickJS::Engine;
#else
using Engine = SpiderMonkey::Engine;
#endif

} // namespace JScript

#endif // _jsengine_h
