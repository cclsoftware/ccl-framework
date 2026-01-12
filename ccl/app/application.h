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
// Filename    : ccl/app/application.h
// Description : Application Component
//
//************************************************************************************************

#ifndef _ccl_application_h
#define _ccl_application_h

#include "ccl/app/component.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/imenu.h"

namespace CCL {

interface IWindow;
interface IProgressNotify;
class ApplicationSpecifics;
class DebugMenuComponent;

//************************************************************************************************
// ApplicationStrings
//************************************************************************************************

namespace ApplicationStrings
{
	StringRef AlreadyRunning ();
	StringRef CanNotShutdown ();
	StringRef RestartRequired ();
}

//************************************************************************************************
// Application
//************************************************************************************************

class Application: public Component,
				   public IApplication,
				   public IMenuExtension,
				   public CommandDispatcher<Application>
{
public:
	DECLARE_CLASS (Application, Component)

	Application (StringID appID = nullptr, StringRef companyName = nullptr, StringRef appName = nullptr,
				 StringID appPackageID = nullptr, StringRef appVersion = nullptr, int versionInt = 0);
	~Application ();

	PROPERTY_BOOL (singleInstance, SingleInstance)	///< single instance application?
	PROPERTY_STRING (website, Website)

	/** Get application instance. */
	static Application* getApplication ();

	/** Called before Kernel is initialized. */
	virtual void beforeInitialize ();

	/** Put application startup code in here. */
	virtual bool startup ();

	/** Called after UI is initialized. */
	virtual void uiInitialized ();

	/** Called before application exits. */
	virtual void beforeQuit ();

	/** Put application shutdown code in here. */
	virtual bool shutdown ();

	/** Show about dialog. */
	virtual void showAbout ();

	/** Application restart has been requested via global signal. */
	virtual void onRestartRequested (MessageRef msg);

	/** Application quit has been requested via global signal. */
	virtual void onQuitRequested (MessageRef msg);

	// IApplication
	StringID CCL_API getApplicationID () const override;
	StringID CCL_API getApplicationPackageID () const override;
	StringRef CCL_API getApplicationTitle () const override;
	ITheme* CCL_API getApplicationTheme () const override;
	IMenuBar* CCL_API createMenuBar () override;
	void CCL_API processCommandLine (ArgsRef args) override;
	tbool CCL_API openFile (UrlRef path) override;
	IDragHandler* CCL_API createDragHandler (const DragEvent& event, IView* view) override;
	tbool CCL_API requestQuit () override;
	tbool CCL_API isQuitRequested () const override;

	// Component
	tresult CCL_API initialize (IUnknown*) override;
	tresult CCL_API terminate () override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// Commands
	DECLARE_COMMANDS (Application)
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	virtual bool onQuit (CmdArgs);
	virtual bool onAbout (CmdArgs);
	virtual bool onShowOptions (CmdArgs args);
	virtual bool onShowCommands (CmdArgs args);
	virtual bool onOpenSettingsFolder (CmdArgs args);
	virtual bool onCommandsHelp (CmdArgs args);
	virtual bool goOnline (CmdArgs);
	virtual bool onHelp (CmdArgs);
	virtual bool onNavigationBack (CmdArgs args);

	/** Access to application specifics. */
	template <class T> T* getSpecifics () { return ccl_cast<T> (specifics); }

	/** Get URL from weblinks.xml. */
	static String getWebLink (StringID id, String* title = nullptr);

	DECLARE_STRINGID_MEMBER (kAppMenuName) ///< in-place application menu name
	
	CLASS_INTERFACE2 (IApplication, IMenuExtension, Component)
	
protected:
	static Application* theApplication;

	/** Set application build information typically shown in about screen. */
	void setBuildInformation (StringRef appNameAndVersion, StringRef appAdditionalVersion = nullptr);

	/** Load legal notice from files in deployment folder. */
	bool setLegalInformation (const IUrl* defaultPath1, const IUrl* defaultPath2 = nullptr, StringRef searchPattern = nullptr);

	/** Load translated strings, done in beforeInitialize(). */
	void loadStrings ();
	
	/** Define translation variables. */
	virtual void initVariables (Attributes& variables) const;

	/** Load application theme. */
	bool loadTheme (UrlRef defaultPath, const IUrl* searchPath1 = nullptr, const IUrl* searchPath2 = nullptr);

	/** Load menu bar. */
	IMenuBar* loadMenuBar (bool variant = false);

	/** Add in-place application menu. */
	void addApplicationMenu ();

	/** Scan for language packs. */
	bool scanLanguagePacks (UrlRef defaultPath);

	/** Scan for plug-ins. */
	bool scanPlugIns (IProgressNotify* progress = nullptr);
	bool scanPlugIns (UrlRef defaultPath, IProgressNotify* progress = nullptr);
	bool scanFrameworkPlugIns ();
	void getDefaultPlugInFolder (Url& defaultPath) const;

	/** Scan for scripts. */
	bool scanScripts (UrlRef defaultPath, IProgressNotify* progress = nullptr);

	/** Create application window. */
	IWindow* createWindow (bool show = true);
	IView* createAboutView ();

	// IMenuExtension
	void CCL_API extendMenu (IMenu& menu, StringID name) override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

private:
	ApplicationSpecifics* specifics;
	DebugMenuComponent* debugMenu;
};

} // namespace CCL

#endif // _ccl_application_h
