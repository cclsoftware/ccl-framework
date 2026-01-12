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
// Filename    : ccl/app/debugmenu.cpp
// Description : Debug Menu
//
//************************************************************************************************

#include "ccl/app/debugmenu.h"

#include "ccl/base/memorypool.h"
#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/storage/iconfiguration.h"
#include "ccl/public/system/iconsole.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/app/signals.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF		hint for xstring tool to skip this section

BEGIN_COMMANDS (DebugMenuComponent)
	DEFINE_COMMAND_("Debug", "Stop Debugging",			DebugMenuComponent::onStopDebugging, CommandFlags::kGlobal)
	DEFINE_COMMAND_("Debug", "Crash",					DebugMenuComponent::onCrash, CommandFlags::kGlobal)
	DEFINE_COMMAND ("Debug", "Memory Check",			DebugMenuComponent::onMemCheck)
	DEFINE_COMMAND ("Debug", "Force GC",				DebugMenuComponent::onMemCheck)
	DEFINE_COMMAND ("Debug", "Dump Script Context",		DebugMenuComponent::onMemCheck)
	DEFINE_COMMAND ("Debug", "Dump Memory Pool",		DebugMenuComponent::onMemCheck)
	DEFINE_COMMAND ("Debug", "Dump Memory Files",		DebugMenuComponent::onMemCheck)
	DEFINE_COMMAND ("Debug", "Save Settings",			DebugMenuComponent::onSaveSettings)
#if DEBUG
END_COMMANDS (DebugMenuComponent)
#else
END_COMMANDS_UNREGISTERED // do not register in release build!
#endif

//************************************************************************************************
// DebugMenuComponent
//************************************************************************************************

String DebugMenuComponent::kComponentName ("DebugMenu");
DebugMenuComponent* DebugMenuComponent::getInstance (Component* c)
{
	return c ? c->getComponent<DebugMenuComponent> (kComponentName) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DebugMenuComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DebugMenuComponent::DebugMenuComponent ()
: Component (kComponentName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DebugMenuComponent::buildMenu (IMenu& menu, bool extend)
{
	menu.setMenuAttribute (IMenu::kMenuName, CCLSTR ("Debug"));
	menu.setMenuAttribute (IMenu::kMenuTitle, CCLSTR ("Debug"));

	menu.addCommandItem (CCLSTR ("Stop Debugging"), CSTR ("Debug"), CSTR ("Stop Debugging"));
	menu.addSeparatorItem ();

	menu.addCommandItem (CCLSTR ("Save Settings"), CSTR ("Debug"), CSTR ("Save Settings"));
	menu.addSeparatorItem ();

	menu.addCommandItem (CCLSTR ("Dump Command Table"), CSTR ("CommandTable"), CSTR ("Dump"));
	menu.addCommandItem (CCLSTR ("Dump Available Keys"), CSTR ("CommandTable"), CSTR ("Dump Available Keys"));
	menu.addSeparatorItem ();

	menu.addCommandItem (CCLSTR ("Memory Check"), CSTR ("Debug"), CSTR ("Memory Check"));
	menu.addCommandItem (CCLSTR ("Force GC"), CSTR ("Debug"), CSTR ("Force GC"));
	menu.addCommandItem (CCLSTR ("Dump Script Context"), CSTR ("Debug"), CSTR ("Dump Script Context"));
	menu.addCommandItem (CCLSTR ("Dump Memory Pool"), CSTR ("Debug"), CSTR ("Dump Memory Pool"));
	menu.addCommandItem (CCLSTR ("Dump Memory Files"), CSTR ("Debug"), CSTR ("Dump Memory Files"));

	menu.addCommandItem (CCLSTR ("Crash"), CSTR ("Debug"), CSTR ("Crash"));

	if(extend)
	{
		Message msg (CCL::Signals::kExtendDebugMenu, &menu);
		SignalSource (CCL::Signals::kDebug).signal (msg);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (DebugMenuComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugMenuComponent::onStopDebugging (CmdArgs args)
{
	if(!args.checkOnly ())
		System::DebugExitProcess (0);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugMenuComponent::onCrash (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		int div = 0;
		div = 100 / div;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugMenuComponent::onMemCheck (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(args.name == "Memory Check")
		{
			Debugger::checkHeap ();
		}
		else if(args.name == "Force GC")
		{
			ccl_forceGC ();
		}
		else if(args.name == "Dump Script Context")
		{
			System::GetScriptingManager ().dump ();
		}
		else if(args.name == "Dump Memory Pool")
		{
			MemoryPool::dumpAll ();
		}
		else if(args.name == "Dump Memory Files")
		{
			MemoryUrl path (nullptr, nullptr, Url::kFolder);
			IFileSystem& fileSystem = System::GetFileSystem ();

			Debugger::println ("=== Memory File System ===");
			int i = 0;
			ForEachFile (fileSystem.newIterator (path), p)
				FileInfo info;
				fileSystem.getFileInfo (info, *p);

				MutableCString binName (p->getHostName ());
				MutableCString entryName (p->getPath ());

				Debugger::printf ("%03d: %s %s %.2lf KB\n", i++, binName.str (), entryName.str (), (double)info.fileSize / 1024.);
			EndFor

			FileInfo totalInfo;
			fileSystem.getFileInfo (totalInfo, path);
			Debugger::printf ("%d entries, %.2lf KB utilized\n", i, (double)totalInfo.fileSize / 1024.);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DebugMenuComponent::onSaveSettings (CmdArgs args)
{
	if(!args.checkOnly ())
		Settings::autoSaveAll ();
	return true;
}

//************************************************************************************************
// ScriptErrorReporter
//************************************************************************************************

ScriptErrorReporter* ScriptErrorReporter::getInstance (Component& parent)
{
	return parent.getComponent<ScriptErrorReporter> ("ScriptErrorReporter");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ScriptErrorReporter, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptErrorReporter::ScriptErrorReporter ()
: Component ("ScriptErrorReporter")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptErrorReporter::reportEvent (const Alert::Event& e)
{
	static bool alertPending = false;
	bool useAlert = System::IsInMainThread () && !alertPending;

	auto makeText = [&] (bool forAlert)
	{
		String separator = forAlert ? "\n" : " ";
		String sectionSeparator = forAlert ? "\n\n" : ": ";

		String text;
		text << "Scripting ";
		switch(e.type)
		{
		case Alert::kError : text << "Error"; break;
		case Alert::kWarning : text << "Warning"; break;
		}

		text << sectionSeparator;
		text << "File: " << e.fileName << separator << "Line: " << e.lineNumber << sectionSeparator;
		text << e.message;
		return text;
	};

	// always write to console (will also apppear in crash log)
	System::GetConsole ().reportEvent (Alert::Event (makeText (false), e.type));

	if(useAlert)
	{
		ScopedVar<bool> scope (alertPending, true);
		Alert::error (makeText (true));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptErrorReporter::setReportOptions (Severity minSeverity, int eventFormat)
{}
