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
// Filename    : ccl/system/console.cpp
// Description : System Console
//
//************************************************************************************************

#include "ccl/system/console.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT System::IConsole& CCL_API System::CCL_ISOLATED (GetConsole) ()
{
	return NativeConsole::instance ();
}

//************************************************************************************************
// StandardConsole
//************************************************************************************************

StandardConsole::StandardConsole ()
: userConsole (nullptr),
  minSeverity (kSeverityTrace),
  eventFormat (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardConsole::redirect (IConsole* console)
{
	userConsole = console;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardConsole::writeLine (StringRef text)
{
	return writeLine (MutableCString (text, Text::kUTF8));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardConsole::writeLine (const char* text)
{
	if(userConsole)
		return userConsole->writeLine (text);
	else
	{
		::printf ("%s", text);
		::printf (ENDLINE);
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardConsole::readLine (String& text)
{
	if(userConsole)
		return userConsole->readLine (text);
	else
	{
		::fflush (stdout);
		char buffer[STRING_STACK_SPACE_MAX] = {0};
		if(::fgets (buffer, ARRAY_COUNT (buffer), stdin) == nullptr)
			return false;
		text = String (Text::kUTF8, buffer);
		text.truncate (text.index (String::getLineEnd (Text::kSystemLineFormat)));
		text.truncate (text.index (String::getLineEnd (Text::kLFLineFormat)));
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StandardConsole::reportEvent (const Alert::Event& e)
{
	if(userConsole)
		userConsole->reportEvent (e);
	else
	{
		if(e.severity <= minSeverity)
			writeLine (e.format (eventFormat));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StandardConsole::setReportOptions (Severity _minSeverity, int _eventFormat)
{
	if(userConsole)
		userConsole->setReportOptions (_minSeverity, _eventFormat);
	else
	{
		minSeverity = _minSeverity;
		eventFormat = _eventFormat;
	}
}

//************************************************************************************************
// NativeConsole
//************************************************************************************************

NativeConsole& NativeConsole::instance ()
{
	static NativeConsole theConsole;
	return theConsole;
}
