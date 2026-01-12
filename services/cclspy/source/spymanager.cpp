//************************************************************************************************
//
// CCL Spy
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
// Filename    : spymanager.cpp
// Description : ccl spy gui management: window management, commands, skin reload
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "spymanager.h"
#include "spycomponent.h"
#include "plugversion.h"

#include "ccl/app/components/consolecomponent.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/app/signals.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/storage/iconfiguration.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Spy;

//************************************************************************************************
// SpyManager
//************************************************************************************************

SpyManager::SpyManager ()
: settings (nullptr),
  spyMenuAdded (false),
  debugSink (Signals::kDebug)
{
	debugSink.setObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpyManager::~SpyManager ()
{
	if(settings)
		settings->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyManager::onWindowEvent (WindowEvent& windowEvent)
{
	if(windowEvent.eventType == WindowEvent::kClose)
	{
		windowEvent.window.removeHandler (this);

		if(settings)
		{
			if(spyView)
			{
				FormBox form (spyView);
				if(form.getForm ())
					if(SpyComponent* spy = unknown_cast<SpyComponent> (form.getController ()))
						spy->save (settings->getAttributes ("Spy"));
			}
			if(!System::GetGUI ().isQuitting ())
				settings->getAttributes ("SpyService").set (CSTR ("showWindow"), 0);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SpyManager::initialize (IUnknown* context)
{
	ISubject::addObserver (&System::GetGUI (), this);

	if(!settings)
	{
		settings = &Settings::instance ();
		settings->init (CCLSTR (PLUG_ID), 1);
		settings->restore ();
		settings->retain ();
	}

	// enable skin warnings
	System::GetFrameworkConfiguration ().setValue ("GUI.Skin", "skinWarningsEnabled", true);

	ResourceUrl commandsUrl (CCLSTR ("commands.xml"));
	System::GetCommandTable ().loadCommands (commandsUrl, ICommandTable::kKeepExisting);
	System::GetCommandTable ().addHandler (this);
	ISubject::addObserver (&System::GetCommandTable (), this);

	// hook into debug menu (if available)
	hookIntoMenuBar ();

	// enable signal sink
	debugSink.enable (true);

	auto createSpyCommand = [] (StringID name, int flags)
	{
		CommandDescription desc (CSTR (CCL_SPY_COMMAND_CATEGORY), name, String (CCL_SPY_COMMAND_CATEGORY), String (name), flags);
		desc.englishName = name;
		return desc;
	};

	static const CommandDescription spyCommands[] =
	{
		createSpyCommand ("Reload Skin", CommandFlags::kGlobal),
		createSpyCommand ("Reload Skin Quick", CommandFlags::kGlobal),
		createSpyCommand (CCL_SPY_COMMAND_NAME, CommandFlags::kGlobal),
		createSpyCommand ("Switch Orientation", 0),
		createSpyCommand ("Move View Left", 0),
		createSpyCommand ("Move View Right", 0),
		createSpyCommand ("Move View Up", 0),
		createSpyCommand ("Move View Down", 0),
		createSpyCommand ("Show Parent", CommandFlags::kGlobal),
		createSpyCommand ("Hilite View", CommandFlags::kGlobal)
	};

	for(int i = 0; i < ARRAY_COUNT (spyCommands); i++)
		System::GetCommandTable ().registerCommand (spyCommands[i]);

	// show spy window
	bool showWindow = settings->getAttributes ("SpyService").getBool (CSTR ("showWindow"));
	if(showWindow)
		openWindow ();

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SpyManager::terminate ()
{
	debugSink.enable (false);

	ISubject::removeObserver (&System::GetGUI (), this);

	System::GetCommandTable ().removeHandler (this);
	ISubject::removeObserver (&System::GetCommandTable (), this);

	if(settings)
		settings->flush ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyManager::extendDebugMenu (IMenu* menu)
{
	MenuInserter inserter (menu, 0);
	menu->addCommandItem (CCLSTR ("Reload Skin Quick"), CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Reload Skin Quick"));
	menu->addCommandItem (CCLSTR ("Reload Skin"), CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Reload Skin"));
	menu->addCommandItem (CCLSTR (CCL_SPY_COMMAND_NAME), CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR (CCL_SPY_COMMAND_NAME));
	menu->addCommandItem (CCLSTR ("Switch Orientation"), CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Switch Orientation"));
	menu->addSeparatorItem ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyManager::hookIntoMenuBar ()
{
	if(spyMenuAdded)
		return;

	if(IMenuBar* menuBar = System::GetDesktop ().getApplicationMenuBar ())
		if(IMenu* menu = menuBar->findMenu (CCLSTR ("Debug")))
		{
			spyMenuAdded = true;
			extendDebugMenu (menu);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SpyManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IApplication::kUIInitialized)
	{
		// second chance to hook into debug menu
		hookIntoMenuBar ();
	}
	else if(msg == ICommandTable::kCommandsLoaded)
	{
		ResourceUrl commandsUrl (CCLSTR ("commands.xml"));
		System::GetCommandTable ().loadCommands (commandsUrl, ICommandTable::kKeepExisting);
	}
	else if(msg == Signals::kExtendDebugMenu)
	{
		UnknownPtr<IMenu> menu (msg[0].asUnknown ());
		if(menu)
			extendDebugMenu (menu);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyManager::checkCommandCategory (CStringRef category) const
{
	return category == CCL_SPY_COMMAND_CATEGORY;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyManager::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == CCL_SPY_COMMAND_CATEGORY)
	{
		if(msg.name == CCL_SPY_COMMAND_NAME)
			return msg.checkOnly () ? true : openWindow ();

		tbool keepImages = msg.name == "Reload Skin Quick";
		if(keepImages || msg.name == "Reload Skin")
			return msg.checkOnly () ? true : reloadSkin (keepImages);

		if(msg.name == "Switch Orientation")
		{
			if(!msg.checkOnly ())
			{
				OrientationType orientation = Styles::kLandscape;
				UnknownPtr<IView> window = System::GetDesktop ().getApplicationWindow ();
				if(window)
				{
					Rect size (window->getSize ());
					orientation = size.getWidth () > size.getHeight () ? Styles::kLandscape : Styles::kPortrait;
				}
				orientation = orientation == Styles::kPortrait ? Styles::kLandscape : Styles::kPortrait;
				simulateOrientationChange (orientation);
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyManager::simulateOrientationChange (OrientationType orientation)
{
	if(!System::GetGUI ().isAllowedInterfaceOrientation (orientation))
		return;
	
	CCL_PROFILE_START (switchOrientation)

	System::GetGUI ().setInterfaceOrientation (orientation);

	UnknownPtr<IView> window = System::GetDesktop ().getApplicationWindow ();
	if(window)
	{
		Rect size (window->getSize ());
		window->setSize (Rect (size.left, size.top, Point (size.getHeight (), size.getWidth () )));
	}

	CCL_PROFILE_STOP (switchOrientation)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpyManager::openWindow ()
{
	if(!spyView)
	{
		AutoPtr<SpyComponent> spy (NEW SpyComponent);

		spyView = RootComponent::instance ().getTheme ()->createView ("SpyView", spy->asUnknown ());
		if(spyView)
		{
			IWindow* w = FormBox (spyView).openWindow ();
			if(w)
			{
				w->addHandler (this);
				if(settings)
				{
					settings->getAttributes ("SpyService").set (CSTR ("showWindow"), 1);
					spy->load (settings->getAttributes ("Spy"));
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpyManager::reloadSkin (tbool keepImages)
{
	ScopedVar<bool> reloadScope (SpyComponent::reloadingSkin, true);

	AutoPtr<ConsoleComponent> console = NEW ConsoleComponent;
	console->setDirectUpdate (true);
	System::GetLogger ().addOutput (console);

	// show reload panel
	IView* reloadForm = RootComponent::instance ().getTheme ()->createView ("SkinReload", console->asUnknown ());
	if(reloadForm)
	{
		FormBox form (reloadForm);
		IWindow* window = System::GetDesktop ().getApplicationWindow ();
		form.openWindow ();
		if(window)
			window->activate (); // for seethru style...
	}

	// reload
	System::GetThemeManager ().reloadAll (keepImages);

	System::GetLogger ().removeOutput (console);

	// close reload panel
	if(reloadForm)
	{
		FormBox (reloadForm).closeWindow ();
		reloadForm->release ();
	}

	// rebuild current perspective of hostapp workspace (if any)
	MutableCString oldPerspectiveID;
	IWorkspace* appWorkspace = nullptr;
	IWorkspace* spyWorkspace = nullptr;
	IRootComponent::Description hostDescription;
	if(RootComponent::instance ().getHostAppDescription (hostDescription))
	{
		spyWorkspace = System::GetWorkspaceManager ().getWorkspace ("cclspy:SkinRefresh");
		appWorkspace = System::GetWorkspaceManager ().getWorkspace (MutableCString (hostDescription.appID));
		if(spyWorkspace && appWorkspace)
		{
			if(IPerspective* refreshPerspective = spyWorkspace->clonePerspective ("cclspy:SkinRefresh"))
			{
				// "lend" our perspective to the app workspace
				oldPerspectiveID = appWorkspace->getSelectedPerspectiveID ();
				if(!oldPerspectiveID.isEmpty ())
				{
					appWorkspace->selectPerspective (refreshPerspective);
		
					System::GetGUI ().flushUpdates ();
				}
			}
		}
		else if(ViewBox appWindow = System::GetDesktop ().getApplicationWindow ())
		{
			// fallback if no application workspace is used: recreate primary application form
			ViewBox appView (appWindow.getChildren ().getFirstView ());
			if(appView.getClassID () == ClassID::Form && MutableCString (appView.getName ()) == IWindowManager::kApplicationFormName)
			{
				Rect bounds (0, 0, appView.getSize ().getSize ());
				if(IView* newAppView = System::GetWindowManager ().createApplicationView (bounds))
				{
					appWindow.getChildren ().remove (appView);
					appView->release ();

					appWindow.getChildren ().add (newAppView);
				}
			}
		}
	}

	if(!oldPerspectiveID.isEmpty ())
		appWorkspace->selectPerspective (oldPerspectiveID);

	return true;
}
