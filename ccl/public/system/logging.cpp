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
// Filename    : ccl/public/system/logging.cpp
// Description : Logging
//
//************************************************************************************************

#include "ccl/public/system/logging.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/systemservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define LOGGING_DEFINE_CSTRING_FUNCTION(name,severity) \
void Logging::name (CStringPtr message, ...) { va_list args; va_start (args, message); Logging::print (severity, message, args); va_end (args); }

#define LOGGING_DEFINE_STRING_FUNCTION(name,severity) \
void Logging::name (StringRef msg) {Logging::print (severity, msg); } \
void Logging::name (StringRef msg, VariantRef arg1) { Variant args[1] = {arg1}; Logging::print (severity, msg, args, 1); } \
void Logging::name (StringRef msg, VariantRef arg1, VariantRef arg2) { Variant args[2] = {arg1, arg2}; Logging::print (severity, msg, args, 2); } \
void Logging::name (StringRef msg, VariantRef arg1, VariantRef arg2, VariantRef arg3) { Variant args[3] = {arg1, arg2, arg3}; Logging::print (severity, msg, args, 3); } \
void Logging::name (StringRef msg, VariantRef arg1, VariantRef arg2, VariantRef arg3, VariantRef arg4) { Variant args[4] = {arg1, arg2, arg3, arg4}; Logging::print (severity, msg, args, 4); }

///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace CCL;

//************************************************************************************************
// Logging
//************************************************************************************************

LOGGING_DEFINE_CSTRING_FUNCTION (fatalf, kSeverityFatal)
LOGGING_DEFINE_CSTRING_FUNCTION (errorf, kSeverityError)
LOGGING_DEFINE_CSTRING_FUNCTION (warningf, kSeverityWarning)
LOGGING_DEFINE_CSTRING_FUNCTION (infof, kSeverityInfo)
LOGGING_DEFINE_CSTRING_FUNCTION (debugf, kSeverityDebug)
LOGGING_DEFINE_CSTRING_FUNCTION (tracef, kSeverityTrace)

///////////////////////////////////////////////////////////////////////////////////////////////////

LOGGING_DEFINE_STRING_FUNCTION (fatal, kSeverityFatal)
LOGGING_DEFINE_STRING_FUNCTION (error, kSeverityError)
LOGGING_DEFINE_STRING_FUNCTION (warning, kSeverityWarning)
LOGGING_DEFINE_STRING_FUNCTION (info, kSeverityInfo)
LOGGING_DEFINE_STRING_FUNCTION (debug, kSeverityDebug)
LOGGING_DEFINE_STRING_FUNCTION (trace, kSeverityTrace)

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::print (Severity severity, CStringPtr message, va_list args)
{
	Alert::Event e (severity, String (MutableCString ().appendFormatArgs (message, args)));
	System::GetLogger ().reportEvent (e);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::print (Severity severity, StringRef message)
{
	Alert::Event e (severity, message);
	System::GetLogger ().reportEvent (e);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::print (Severity severity, StringRef message, Variant args[], int count)
{
	Alert::Event e (severity, String ().appendFormat (message, args, count));
	System::GetLogger ().reportEvent (e);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#undef LOGGING_DEFINE_CSTRING_FUNCTION
#undef LOGGING_DEFINE_STRING_FUNCTION
