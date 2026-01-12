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
// Filename    : jsinclude.h
// Description : JavaScript Includes
//
//************************************************************************************************

#ifndef _jsinclude_h
#define _jsinclude_h

#include "ccl/public/base/platform.h"

#pragma push_macro ("ForEach")
#undef ForEach
#if !defined(ENABLE_DEBUG_SPIDERMONKEY) // can be switched with VENDOR_ENABLE_DEBUG_SPIDERMONKEY cmake variable
	#pragma push_macro ("DEBUG")
	#undef DEBUG
#endif

#if DEBUG
	#define MOZ_DEBUG 1
	#define MOZ_DIAGNOSTIC_ASSERT_ENABLED 1
#endif

#if CCL_PLATFORM_MAC
	#define XP_MACOSX 1
#elif CCL_PLATFORM_WINDOWS
	#define XP_WIN 1
#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
	#define XP_LINUX 1
#elif CCL_PLATFORM_IOS
	#define XP_IOS 1
#else
	#error unknown platform
#endif

#define NIGHTLY_BUILD 1

// SpiderMonky does not use our debug heap
#undef malloc
#undef realloc
#undef free

#include "jsapi.h"
#include "jsfriendapi.h"

#include "js/Initialization.h"
#include "js/Array.h"
#include "js/Class.h"
#include "js/CompilationAndEvaluation.h"
//#include "js/Debug.h"
#include "js/EnvironmentChain.h"
#include "js/ErrorReport.h"
#include "js/GCAPI.h"
#include "js/HeapAPI.h"
#include "js/Object.h"
#include "js/Proxy.h"
#include "js/String.h"
#include "js/SourceText.h"
#include "js/BigInt.h"

#include "js/friend/ErrorMessages.h"

#include "js/experimental/TypedData.h"

#include "mozilla/ThreadLocal.h"

#if !defined(ENABLE_DEBUG_SPIDERMONKEY)
#pragma pop_macro ("DEBUG")
#endif

#pragma pop_macro ("ForEach")

#endif // _jsinclude_h
