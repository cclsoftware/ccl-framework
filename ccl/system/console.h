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
// Filename    : ccl/system/console.h
// Description : System Console
//
//************************************************************************************************

#ifndef _cclsystem_console_h
#define _cclsystem_console_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/system/iconsole.h"

namespace CCL {

//************************************************************************************************
// StandardConsole
/** Console class using stdio. */
//************************************************************************************************

class StandardConsole: public Unknown,
					   public System::IConsole
{
public:
	StandardConsole ();

	// System::IConsole
	tbool CCL_API redirect (IConsole* console) override;
	tbool CCL_API writeLine (StringRef text) override;
	tbool CCL_API writeLine (const char* text) override;
	tbool CCL_API readLine (String& text) override;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity _minSeverity, int _eventFormat) override;

	CLASS_INTERFACE2 (IConsole, IReporter, Unknown)

protected:
	IConsole* userConsole;
	Severity minSeverity;
	int eventFormat;
};

//************************************************************************************************
// NativeConsole
/** Extended console using OS-specific APIs. */
//************************************************************************************************

class NativeConsole: public StandardConsole
{
public:
	static NativeConsole& instance ();
};

} // namespace CCL

#endif // _cclsystem_console_h
