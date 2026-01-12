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
// Filename    : ccl/app/application.cpp
// Description : Application Component
//
//************************************************************************************************

#include "ccl/app/application.h"
#include "ccl/app/applicationspecifics.h"
#include "ccl/app/debugmenu.h"
#include "ccl/app/options/commandoption.h"

#include "ccl/public/app/signals.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/filefilter.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/icommandeditor.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Application")
	XSTRING (StartupText, "Welcome to %(1)!")
	XSTRING (ThemeLoadFailed, "Failed to load application theme!")
	XSTRING (ScanningPlugIns, "Scanning Plug-Ins...")
	XSTRING (ScanningScripts, "Scanning Scripts...")
	XSTRING (ScanningLanguagePacks, "Scanning Languages...")
	XSTRING (ApplicationAlreadyRunning, "An instance of $APPNAME is already running.")
	XSTRING (AskApplicationRestart, "Do you want to restart $APPNAME now?")
	XSTRING (ApplicationCanNotShutdown, "$APPNAME can not shutdown right now!")
	XSTRING (ApplicationRestartRequired, "Please restart $APPNAME for the changes to take effect.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (Application)
	DEFINE_COMMAND ("File",			"Quit",					Application::onQuit)
	DEFINE_COMMAND ("Help",			"About",				Application::onAbout)
	DEFINE_COMMAND_("Help",			"Contents",				Application::onHelp, CommandFlags::kGlobal)
	DEFINE_COMMAND_("Help",			"Context Help",			Application::onHelp, CommandFlags::kGlobal)
	DEFINE_COMMAND ("Help",			"Keyboard Shortcuts",	Application::onCommandsHelp)
	DEFINE_COMMAND ("Help",			"Website",				Application::goOnline)
	DEFINE_COMMAND ("Help",			"Open Settings Folder",	Application::onOpenSettingsFolder)
	DEFINE_COMMAND ("Application",	"Options",				Application::onShowOptions)
	DEFINE_COMMAND ("Navigation",	"Back",					Application::onNavigationBack)
	DEFINE_COMMAND_ARGS	("Application", "Keyboard Shortcuts", Application::onShowCommands, 0, "InitialCategory,InitialCommand")
END_COMMANDS (Application)

//************************************************************************************************
// WindowSettingsSaver
//************************************************************************************************

class WindowSettingsSaver: public SettingsSaver
{
public:
	// SettingsSaver
	void restore (Settings&) override { System::GetWindowManager ().restoreWindowStates (); }
	void flush (Settings&) override { System::GetWindowManager ().storeWindowStates (); }
};

//************************************************************************************************
// ApplicationStrings
//************************************************************************************************

StringRef ApplicationStrings::AlreadyRunning () { return XSTR (ApplicationAlreadyRunning); }
StringRef ApplicationStrings::CanNotShutdown () { return XSTR (ApplicationCanNotShutdown); }
StringRef ApplicationStrings::RestartRequired () { return XSTR (ApplicationRestartRequired); }

//************************************************************************************************
// Application
//************************************************************************************************

Application* Application::theApplication = nullptr;
Application* Application::getApplication ()
{
	return theApplication;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Application::getWebLink (StringID id, String* title)
{
	static XmlSettings webLinks ("Weblinks");
	static bool restored = false;
	if(!restored)
	{
		#if 1 // allow local file to override built-in resource
		Url localPath;
		System::GetSystem ().getLocation (localPath, System::kAppSupportFolder);
		localPath.descend ("weblinks.xml");
		if(System::GetFileSystem ().fileExists (localPath))
			webLinks.setPath (localPath);
		else
		#endif
			webLinks.setPath (Url ("resource:///weblinks.xml"));

		restored = webLinks.restore ();
		ASSERT (restored == true)
	}

	Url* url = webLinks.getAttributes ("AppLinks").getObject<Url> (id);
	ASSERT (url != nullptr)
	if(url)
	{
		if(title)
		{
			if(auto uwt = ccl_cast<UrlWithTitle> (url))
				*title = uwt->getTitle ();
		}

		return UrlFullString (*url, true);
	}
	else
		return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Application, Component)
DEFINE_STRINGID_MEMBER_ (Application, kAppMenuName, "applicationMenu")
IMPLEMENT_COMMANDS (Application, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

Application::Application (StringID appID, StringRef companyName, StringRef appName,
						  StringID appPackageID, StringRef appVersion, int versionInt)
: Component (CCLSTR (kComponentName), appName),
  singleInstance (true),
  specifics (nullptr),
  debugMenu (nullptr)
{
	// Note: Component name is fixed. Application instance can be accessed by skin
	// from object table via "object://{appID}/Application" or "object://hostapp/Application"

	ASSERT (theApplication == nullptr)
	theApplication = this;

	auto& root = RootComponent::instance ();
	root.setApplicationID (appID);
	root.setApplicationPackageID (appPackageID);
	root.setTitle (appName);
	root.setCompanyName (companyName);
	root.setApplicationVersion (appVersion);
	root.addComponent (this);

	System::GetAlertService ().setTitle (appName);
	System::GetCommandTable ().addHandler (&root);
	System::GetObjectTable ().registerObject (root.asUnknown (), kNullUID, appID, IObjectTable::kIsHostApp);

	// init location for application settings
	System::GetSystem ().setApplicationName (companyName, appName, versionInt);

	// init global file filter condition
	FileFilter::getGlobalConditions ().setEntry (FileFilter::kAppIdentityKey, String (appID));

	// add specifics + debug menu
	addComponent (specifics = ApplicationSpecifics::createInstance ());
	addComponent (debugMenu = NEW DebugMenuComponent);

	// register for application signals
	signalSlots.advise (SignalSource (Signals::kApplication).getAtom (), Signals::kRequestRestart, this, &Application::onRestartRequested);
	signalSlots.advise (SignalSource (Signals::kApplication).getAtom (), Signals::kRequestQuit, this, &Application::onQuitRequested);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Application::~Application ()
{
	// unregister from application signals
	signalSlots.unadvise (SignalSource (Signals::kApplication).getAtom ());

	ASSERT (theApplication == this)
	theApplication = nullptr;

	auto& root = RootComponent::instance ();
	System::GetCommandTable ().removeHandler (&root);

	root.unloadTheme ();
	root.unloadStrings ();

	System::GetObjectTable ().unregisterObject (root.asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::setBuildInformation (StringRef appNameAndVersion, StringRef appAdditionalVersion)
{
	paramList.addString ("appNameAndVersion")->fromString (appNameAndVersion);
	paramList.addString ("appAdditionalVersion")->fromString (appAdditionalVersion);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::setLegalInformation (const IUrl* defaultPath1, const IUrl* defaultPath2, StringRef _searchPattern)
{
	IParameter* param = paramList.addString ("licenseText");
	param->setReadOnly (true);

	Vector<Url> folders;
	if(defaultPath1 && !defaultPath1->isEmpty ())
		folders.add (Url (*defaultPath1));
	if(defaultPath2 && !defaultPath2->isEmpty ())
		folders.add (Url (*defaultPath2));

	if(folders.isEmpty ())
	{
		Url licenseFolder;
		System::GetSystem ().getLocation (licenseFolder, System::kAppDeploymentFolder);
		licenseFolder.descend ("license", Url::kFolder);
		folders.add (licenseFolder);
	}

	String searchPattern (_searchPattern);
	if(searchPattern.isEmpty ())
		searchPattern = "*.txt";

	bool result = false;
	for(const Url& folder : folders)
		ForEachFile (File::findFiles (folder, searchPattern), path)
			String licenseText = TextUtils::loadString (*path);
			if(!licenseText.isEmpty ())
			{
				result = true;
				String existingText;
				param->toString (existingText);
				if(!existingText.isEmpty ())
				{
					String mergedText (existingText);
					mergedText << "\n\n";
					mergedText << licenseText;
					param->fromString (mergedText);
				}
				else
					param->fromString (licenseText);
			}
		EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Application::getApplicationID () const
{
	auto& root = RootComponent::instance ();
	return root.getApplicationID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Application::getApplicationPackageID () const
{
	auto& root = RootComponent::instance ();
	return root.getApplicationPackageID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::initVariables (Attributes& variables) const
{
	TranslationVariables::setBuiltinVariables (variables);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::loadStrings ()
{
	#if !CCL_STATIC_LINKAGE
	if(LocalString::hasTable () == false) // loadStrings() might have been called already
	#endif
	{
		Attributes variables;
		initVariables (variables);

		auto& root = RootComponent::instance ();
		root.loadStrings (&variables);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::beforeInitialize ()
{
	// load settings
	Settings::instance ().init (getTitle ());
	Settings::instance ().isBackupEnabled (true);
	Settings::instance ().restore ();

	Settings::instance ().addSaver (NEW WindowSettingsSaver);

	// load strings
	loadStrings ();

	// load commands
	System::GetCommandTable ().loadCommands (ResourceUrl ("commands.xml"), ICommandTable::kOverwriteExisting);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::loadTheme (UrlRef defaultPath, const IUrl* searchPath1, const IUrl* searchPath2)
{
	auto& root = RootComponent::instance ();
	bool result = root.loadTheme (defaultPath, searchPath1, searchPath2);
	#if CCL_PLATFORM_DESKTOP
	if(!result)
		Alert::error (XSTR (ThemeLoadFailed));
	#endif	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::scanLanguagePacks (UrlRef defaultPath)
{
	Url url (defaultPath);
	if(url.isEmpty () || !System::GetFileSystem ().fileExists (url))
		System::GetLocaleManager ().getLanguagesFolder (url);

	if(System::GetFileSystem ().fileExists (url))
	{
		System::GetLogger ().reportEvent (XSTR (ScanningLanguagePacks));
		return System::GetLocaleManager ().scanLanguagePacks (url) > 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::getDefaultPlugInFolder (Url& defaultPath) const
{
	GET_BUILD_FOLDER_LOCATION (defaultPath)
	if(!defaultPath.isEmpty ())
		defaultPath.descend (CCLSTR ("Plugins"), Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::scanPlugIns (IProgressNotify* progress)
{
	Url defaultPath;
	getDefaultPlugInFolder (defaultPath);
	
	return scanPlugIns (defaultPath, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::scanPlugIns (UrlRef defaultPath, IProgressNotify* progress)
{
	System::GetLogger ().reportEvent (XSTR (ScanningPlugIns));
	if(progress)
		progress->setProgressText (XSTR (ScanningPlugIns));

	Url url (defaultPath);
	if(url.isEmpty () || !System::GetFileSystem ().fileExists (url))
		System::GetSystem ().getLocation (url, System::kAppPluginsFolder);

	if(!System::GetFileSystem ().fileExists (url))
		return false;

	// respect optional plug-in filter for this application
	FileFilter pluginFilter;
	IUrlFilter* theFilter = pluginFilter.loadFromFile (ResourceUrl ("pluginfilter.xml")) ? &pluginFilter : nullptr;
	return System::GetPlugInManager ().scanFolder (url, CodeResourceType::kNative, PlugScanOption::kRecursive, nullptr, theFilter) > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::scanFrameworkPlugIns ()
{
	FileFilter filter;
	if(!filter.loadFromFile (ResourceUrl ("cclplugins.xml")))
		return false;

	Url url;
	getDefaultPlugInFolder (url);
	if(url.isEmpty () || !System::GetFileSystem ().fileExists (url))
		System::GetSystem ().getLocation (url, System::kAppPluginsFolder);

	if(!System::GetFileSystem ().fileExists (url))
		return false;

	return System::GetPlugInManager ().scanFolder (url, CodeResourceType::kNative, PlugScanOption::kRecursive, nullptr, &filter) > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::scanScripts (UrlRef defaultPath, IProgressNotify* progress)
{
	System::GetLogger ().reportEvent (XSTR (ScanningScripts));
	if(progress)
		progress->setProgressText (XSTR (ScanningScripts));

	Url url (defaultPath);
	if(url.isEmpty () || !System::GetFileSystem ().fileExists (url))
	{
		System::GetSystem ().getLocation (url, System::kAppSupportFolder);
		url.descend (CCLSTR ("Scripts"), Url::kFolder);
	}

	if(!System::GetFileSystem ().fileExists (url))
		return false;

	return System::GetPlugInManager ().scanFolder (url, CodeResourceType::kScript) > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* Application::createWindow (bool show)
{
	return System::GetWindowManager ().createApplicationWindow (show);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::startup ()
{
	System::GetLogger ().reportEvent (String ().appendFormat (XSTR (StartupText), getTitle ()));

	// register commands
	CommandRegistry::registerWithCommandTable ();

	// scan framework plug-ins (optional)
	scanFrameworkPlugIns ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::uiInitialized ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::beforeQuit ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::shutdown ()
{
	// save settings
	Settings::instance ().flush ();

	// empty clipboard
	System::GetClipboard ().empty ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Application::initialize (IUnknown* context)
{
	if(!startup ())
		return kResultFalse;

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Application::terminate ()
{
	tresult result = SuperClass::terminate ();
	ASSERT (result == kResultOk)

	return shutdown () ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Application::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kUIInitialized)
	{
		auto& root = RootComponent::instance ();
		if(root.isQuitting () || root.isRestartRequested ())
			return;

		uiInitialized ();
	}
	else if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IParameter> param (subject);
		UnknownPtr<IMenu> menu (msg[0].asUnknown ());
		if(param && menu)
		{
			extendMenu (*menu, param->getName ());

			// add debug menu
			#if DEBUG && !CCL_PLATFORM_MAC
			if(param->getName () == kAppMenuName)
			{
				if(menu->countItems () > 0) // don't add debug menu when intentionally left empty
					if(IMenu* subMenu = menu->createMenu ())
					{
						debugMenu->buildMenu (*subMenu, true);
						menu->addSeparatorItem ();
						menu->addMenu (subMenu);
					}
			}
			#endif
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API Application::findChild (StringRef id) const
{
	// make options available to skin
	static const String kUserOptionPrefix ("UserOption.");
	if(id.startsWith (kUserOptionPrefix))
	{
		String optionName = id.subString (kUserOptionPrefix.length ());
		if(IUserOption* userOption = UserOptionManager::instance ().findOptionByName (optionName))
			return UnknownPtr<IObjectNode> (userOption);
	}
	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Application::processCommandLine (ArgsRef args)
{
	if(args.count () >= 2)
	{
		Url path;
		if(Url::isUrlString (args[1]))
			path.setUrl (args[1]);
		else
			path.fromDisplayString (args[1]);

		openFile (path);
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Application::openFile (UrlRef path)
{
	return System::GetFileTypeRegistry ().getHandlers ().openFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API Application::createDragHandler (const DragEvent& event, IView* view)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* Application::createAboutView ()
{
	return getTheme ()->createView ("AboutApplication", this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::showAbout ()
{
	static bool aboutDialogOpen = false;
	if(aboutDialogOpen) // suppress if dialog already open
		return;

	if(IView* view = createAboutView ())
	{
		aboutDialogOpen = true;
		Promise (DialogBox ()->runDialogAsync (view)).then ([&] (IAsyncOperation& op)
			{
				aboutDialogOpen = false;
			});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::onRestartRequested (MessageRef msg)
{
	auto requestRestart = [this] ()
	{
		// try to close modal dialogs first
		if(!System::GetDesktop ().closeModalWindows ())
			return;

		auto& root = RootComponent::instance ();
		root.setRestartRequested (true);
		requestQuit ();
	};

	bool confirmed = msg.getArgCount () > 1 ? msg[1].asBool () : false;
	if(confirmed)
		requestRestart ();
	else
	{
		String text;
		if(msg.getArgCount () > 0) // caller can provide additional information
			text << msg[0].asString () << "\n\n";
		text << XSTR (AskApplicationRestart);

		Promise (Alert::askAsync (text)).then ([requestRestart] (IAsyncOperation& operation)
		{
			if(operation.getResult ().asInt () == Alert::kYes)
				requestRestart ();
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::onQuitRequested (MessageRef msg)
{
	UnknownPtr<IVariant> result;
	if(msg.getArgCount () >= 1)
		result = msg[0].asUnknown ();

	if(result)
		result->assign (false);

	// try to close modal dialogs first
	if(!System::GetDesktop ().closeModalWindows ())
		return;

	tbool quiteDone = requestQuit ();
	if(result)
		result->assign (quiteDone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Application::getApplicationTitle () const
{
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* CCL_API Application::getApplicationTheme () const
{
	return getTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuBar* CCL_API Application::createMenuBar ()
{
	return loadMenuBar ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuBar* Application::loadMenuBar (bool variant)
{
	IMenuBar* menuBar = System::GetWindowManager ().createApplicationMenuBar (variant);
	if(!menuBar)
		return nullptr;

	auto& root = RootComponent::instance ();
	menuBar->loadMenus (ResourceUrl ("menubar.xml"), this, root.getStringTable ());

	if(menuBar->countMenus () == 0)
	{
		menuBar->release ();
		return nullptr;
	}

	// add debug menu
	#if DEBUG
	if(IMenu* firstMenu = menuBar->getMenu (0)) // don't add debug menu when intentionally left empty
		if(IMenu* dbgMenu = firstMenu->createMenu ())
		{
			debugMenu->buildMenu (*dbgMenu);
			menuBar->addMenu (dbgMenu);
		}
	#endif

	return menuBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Application::addApplicationMenu ()
{
	paramList.addMenu (kAppMenuName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Application::extendMenu (IMenu& parent, StringID name)
{
	if(name == kAppMenuName)
	{
		// default behavior is to load in-place application menu from resource
		auto& root = RootComponent::instance ();
		parent.loadItems (ResourceUrl ("appmenu.xml"), nullptr, this, root.getStringTable ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Application::requestQuit ()
{
	auto& root = RootComponent::instance ();
	if(root.isQuitting ())
		return true;

	// close popups first (ensure we leave the modal callstack of a popup before trying to quit)
	// requestQuit will be called again via onQuit
	if(System::GetDesktop ().closePopupAndDeferCommand (this, CommandMsg ("File", "Quit")))
		return true;

	root.setQuitRequested (true);
	if(!root.canTerminate ())
	{
		root.setQuitRequested (false);
		root.setRestartRequested (false); // reset state
		return false;
	}

	beforeQuit ();

	ccl_forceGC (); // ensure that all window references are gone

	System::GetGUI ().quit ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Application::isQuitRequested () const
{
	auto& root = RootComponent::instance ();
	return root.isQuitRequested ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Application::checkCommandCategory (CStringRef category) const
{
	if(category == "File" || category == "Help" || category == "Application" || category == "Navigation")
		return true;

	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onQuit (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	requestQuit ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onAbout (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	showAbout ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onOpenSettingsFolder (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		Url url;
		System::GetSystem ().getLocation (url, System::kAppSettingsFolder);
		System::GetSystemShell ().openUrl (url);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onShowOptions (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	UserOptionManager::instance ().runDialog ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onShowCommands (CmdArgs args)
{
	// disabled if read-only command schemes present
	if(UserOptionManager::instance ().findOptionByName (CommandSchemeOption::Name ()))
		return false;

	if(!args.checkOnly ())
	{
		MutableCString initialCategory, initialCommand;
		CommandAutomator::Arguments (args).getCString ("InitialCategory", initialCategory);
		CommandAutomator::Arguments (args).getCString ("InitialCommand", initialCommand);
		
		// try to open inside options dialog first
		UserOptionList* optionList = nullptr;
		if(IUserOption* option = UserOptionManager::instance ().findOptionByName (CommandEditorOption::Name (), &optionList))
		{
			CommandEditorOption* commandEditorOption = unknown_cast<CommandEditorOption> (option);
			if(commandEditorOption && !initialCategory.isEmpty () && !initialCommand.isEmpty ())
				commandEditorOption->setInitialCommand (initialCategory, initialCommand);
			UserOptionManager::instance ().runDialog (optionList, option);
			if(commandEditorOption)
				commandEditorOption->setInitialCommand (nullptr, nullptr);
		}
		else
		{
			AutoPtr<ICommandEditor> editor = ccl_new<ICommandEditor> (ClassID::CommandEditor);
			ASSERT (editor)

			if(!initialCategory.isEmpty () && !initialCommand.isEmpty ())
			{
				CommandDescription commandDescription;
				commandDescription.category = initialCategory;
				commandDescription.name = initialCommand;
				editor->init (commandDescription);
			}

			if(editor->run () == kResultOk)
				CommandSaver::store (); // store user commands
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onCommandsHelp (CmdArgs args)
{
	// if read-only command schemes present, we link to a documentation file
	if(UserOptionManager::instance ().findOptionByName (CommandSchemeOption::Name ()))
	{
		if(!args.checkOnly ())
			System::GetHelpManager ().showLocation (CCLSTR ("Keyboard Shortcuts"));
		return true;
	}
	else
	{
		if(!args.checkOnly ())
			CommandEditorOption::showCurrentCommandsText ();
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::goOnline (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	System::GetSystemShell ().openUrl (Url (getWebsite ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onHelp (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(args.name == "Context Help")
			System::GetHelpManager ().showContextHelp ();
		else if(args.name == "Contents")
			System::GetHelpManager ().showLocation (CCLSTR ("Contents"));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::onNavigationBack (CmdArgs args)
{
	return false; // only to help overriding in derived application classes
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Application::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "Configuration")
	{
		// accessible via "Host.{appID}.find ('Application').Configuration"
		var = Configuration::Registry::instance ().asUnknown ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
