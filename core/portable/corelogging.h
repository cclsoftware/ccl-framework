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
// Filename    : core/portable/corelogging.h
// Description : Logging Utilities
//
//************************************************************************************************

#ifndef _corelogging_h
#define _corelogging_h

#include "core/public/coremacros.h"
#include "core/public/coretypes.h"
#include "core/public/corevector.h"

#include "core/portable/coresingleton.h"

#include <cstdarg>

namespace Core {
namespace Portable {

//************************************************************************************************
// LogSink
//************************************************************************************************

class LogSink
{
public:
	LogSink ();

	virtual void write (Severity severity, CStringPtr message) = 0;

	PROPERTY_VARIABLE (Severity, minLogLevel, MinLogLevel)
};

//************************************************************************************************
// Logger
//************************************************************************************************

class Logger: public StaticSingleton<Logger>
{
public:
	void addSink (LogSink* sink);
	void removeSink (LogSink* sink);
	void writeMessage (Severity severity, CStringPtr message, va_list args);

private:
	FixedSizeVector<LogSink*, 8> logSinkList;
};

//************************************************************************************************
// Logging
//************************************************************************************************

namespace Logging
{
	void fatal (CStringPtr message, ...);
	void error (CStringPtr message, ...);
	void warning (CStringPtr message, ...);
	void info (CStringPtr message, ...);
	void debug (CStringPtr message, ...);
	void trace (CStringPtr message, ...);
}

} // namespace Portable
} // namespace Core

#endif // _corelogging_h
