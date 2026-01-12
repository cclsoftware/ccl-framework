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
// Filename    : ccl/app/components/consolecomponent.h
// Description : Console Component
//
//************************************************************************************************

#ifndef _consolecomponent_h
#define _consolecomponent_h

#include "ccl/app/component.h"

#include "ccl/public/system/iconsole.h"

namespace CCL {

class ConsoleListModel;
class LogEvent;

//************************************************************************************************
// ConsoleComponent
//************************************************************************************************

class ConsoleComponent: public Component,
						public System::IConsole
{
public:
	DECLARE_CLASS (ConsoleComponent, Component)

	ConsoleComponent (StringRef name = nullptr);
	~ConsoleComponent ();

	void setActive (bool state = true);
	PROPERTY_BOOL (directUpdate, DirectUpdate)
	PROPERTY_BOOL (scrollOnEvent, ScrollOnEvent)
	PROPERTY_BOOL (showLineNumbers, ShowLineNumbers)

	void scrollDown ();
	void clear ();

	void setViewVisible (bool state); // internal
	bool isViewVisible () const;

	// System::IConsole
	tbool CCL_API writeLine (StringRef text) override;
	tbool CCL_API writeLine (const char* text) override;
	tbool CCL_API readLine (String& text) override;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IConsole, IReporter, Component)

protected:
	ConsoleListModel* listModel;
	bool redirected;
	bool viewVisible;

	void addEvent (LogEvent* logEvent, bool flushEvents);

	// System::IConsole
	tbool CCL_API redirect (IConsole* console) override;
};

} // namespace CCL

#endif // _consolecomponent_h
