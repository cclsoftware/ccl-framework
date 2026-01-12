//************************************************************************************************
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
// Filename    : ccl/public/text/istringprivate.h
// Description : IString private data
//
//************************************************************************************************

#ifndef _ccl_istringprivate_h
#define _ccl_istringprivate_h

#include "ccl/public/base/platform.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// IStringPrivateData
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Reference to internal string data, used to display something meaningful in Debugger.
	Never ever modify this value directly or build any code relying on it being of a certain type! 
	\ingroup text_string */

#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#define IStringPrivateData char*
#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
typedef const char16_t** IStringPrivateData;
#elif CCL_PLATFORM_WINDOWS
typedef const __wchar_t** IStringPrivateData;
#else
typedef void* IStringPrivateData;
#endif

#endif // _ccl_istringprivate_h
