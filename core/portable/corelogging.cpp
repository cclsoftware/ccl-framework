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
// Filename    : core/portable/corelogging.cpp
// Description : Logging Utilities
//
//************************************************************************************************

#include "core/public/corestringbuffer.h"

#include "core/portable/corelogging.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// LogSink
//************************************************************************************************

LogSink::LogSink ()
: minLogLevel (kSeverityInfo)
{
}

//************************************************************************************************
// Logger
//************************************************************************************************

DEFINE_STATIC_SINGLETON (Logger)

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::addSink (LogSink* sink)
{
	ASSERT (sink)
	ASSERT (!logSinkList.isFull ())
	logSinkList.add (sink);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::removeSink (LogSink* sink)
{
	logSinkList.remove (sink);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::writeMessage (Severity severity, CStringPtr message, va_list args)
{
	CString128 buffer;
	buffer.appendFormatArgs (message, args);

	for(LogSink* sink : logSinkList)
	{
		if(static_cast<int> (severity) <= static_cast<int> (sink->getMinLogLevel ()))
			sink->write (severity, buffer);
	}
}

//************************************************************************************************
// Logging
//************************************************************************************************

void Logging::fatal (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance().writeMessage (kSeverityFatal, message, args);
	va_end (args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::error (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance ().writeMessage (kSeverityError, message, args);
	va_end (args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::warning (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance ().writeMessage (kSeverityWarning, message, args);
	va_end (args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::info (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance ().writeMessage (kSeverityInfo, message, args);
	va_end (args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::debug (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance ().writeMessage (kSeverityDebug, message, args);
	va_end (args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logging::trace (CStringPtr message, ...)
{
	va_list args;
	va_start (args, message);
	Logger::instance ().writeMessage (kSeverityTrace, message, args);
	va_end (args);
}
