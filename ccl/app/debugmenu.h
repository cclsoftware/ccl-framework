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
// Filename    : ccl/app/debugmenu.h
// Description : Debug Menu
//
//************************************************************************************************

#ifndef _ccl_debugmenu_h
#define _ccl_debugmenu_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"

#include "ccl/public/system/alerttypes.h"

namespace CCL {

interface IMenu;

//************************************************************************************************
// DebugMenuComponent
//************************************************************************************************

class DebugMenuComponent: public Component,
						  public CommandDispatcher<DebugMenuComponent>
{
public:
	DECLARE_CLASS (DebugMenuComponent, Component)

	DebugMenuComponent ();

	static DebugMenuComponent* getInstance (Component* c);

	void buildMenu (IMenu& menu, bool extend = false);

	// Commands
	DECLARE_COMMANDS (DebugMenuComponent)
	DECLARE_COMMAND_CATEGORY ("Debug", Component)
	bool onStopDebugging (CmdArgs);
	bool onCrash (CmdArgs);
	bool onMemCheck (CmdArgs);
	bool onSaveSettings (CmdArgs);

protected:
	static String kComponentName;
};

//************************************************************************************************
// ScriptErrorReporter
//************************************************************************************************

class ScriptErrorReporter: public Component,
						   public Alert::IReporter
{
public:
	DECLARE_CLASS (ScriptErrorReporter, Component)

	ScriptErrorReporter ();

	static ScriptErrorReporter* getInstance (Component& parent);

	// Reporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	CLASS_INTERFACE (IReporter, Component)
};

} // namespace CCL

#endif // _ccl_debugmenu_h
