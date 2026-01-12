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
// Filename    : ccl/public/system/logging.h
// Description : Logging
//
//************************************************************************************************

#ifndef _ccl_logging_h
#define _ccl_logging_h

#include "ccl/public/base/variant.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare ASCII string logging function with variadic args. */
#define LOGGING_DECLARE_CSTRING_FUNCTION(name) \
static void name (CStringPtr message, ...);

/** Declare Unicode string logging functions with up to four format args. */
#define LOGGING_DECLARE_STRING_FUNCTION(name) \
static void name (StringRef message); \
static void name (StringRef message, VariantRef arg1); \
static void name (StringRef message, VariantRef arg1, VariantRef arg2); \
static void name (StringRef message, VariantRef arg1, VariantRef arg2, VariantRef arg3); \
static void name (StringRef message, VariantRef arg1, VariantRef arg2, VariantRef arg3, VariantRef arg4);

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

//************************************************************************************************
// Logging
//************************************************************************************************

class Logging
{
public:
	// ASCII, printf compatible.
	LOGGING_DECLARE_CSTRING_FUNCTION (fatalf)
	LOGGING_DECLARE_CSTRING_FUNCTION (errorf)
	LOGGING_DECLARE_CSTRING_FUNCTION (warningf)
	LOGGING_DECLARE_CSTRING_FUNCTION (infof)
	LOGGING_DECLARE_CSTRING_FUNCTION (debugf)
	LOGGING_DECLARE_CSTRING_FUNCTION (tracef)

	// Unicode, format args.
	LOGGING_DECLARE_STRING_FUNCTION (fatal)
	LOGGING_DECLARE_STRING_FUNCTION (error)
	LOGGING_DECLARE_STRING_FUNCTION (warning)
	LOGGING_DECLARE_STRING_FUNCTION (info)
	LOGGING_DECLARE_STRING_FUNCTION (debug)
	LOGGING_DECLARE_STRING_FUNCTION (trace)

protected:
	static void print (Severity severity, CStringPtr message, va_list args);
	static void print (Severity severity, StringRef message);
	static void print (Severity severity, StringRef format, Variant args[], int count);
};

} // namespace CCL

#undef LOGGING_DECLARE_CSTRING_FUNCTION
#undef LOGGING_DECLARE_STRING_FUNCTION

#endif // _ccl_logging_h
