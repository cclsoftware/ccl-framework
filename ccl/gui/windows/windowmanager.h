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
// Filename    : ccl/gui/windows/windowmanager.h
// Description : Window Manager
//
//************************************************************************************************

#ifndef _ccl_windowmanager_h
#define _ccl_windowmanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/themeelements.h"

#include "ccl/app/params.h"
#include "ccl/app/paramcontainer.h"

namespace CCL {

class Theme;
class WindowClass;
class Window;
class View;
class Settings;
interface IActivatable;
interface IApplication;

/** Window class reference. */
typedef const WindowClass& WindowClassRef;

//************************************************************************************************
// WindowClass
//************************************************************************************************

class WindowClass: public Object,
				   public IWindowClass
{
public:
	DECLARE_CLASS_ABSTRACT (WindowClass, Object)

	WindowClass ();
	~WindowClass ();

	Theme* getTheme () const;
	void setTheme (Theme* theme);

	IUnknown* getController () const;					///< get associated controller (resolves controllerUrl)
	bool isActive ();									///< tells if the class is active
	StringID getViewStateID () const;					///< get identifier for storing gui states

	PROPERTY_MUTABLE_CSTRING (id, ID)					///< general identifier
	PROPERTY_STRING  (title, Title)						///< window class title visible to user
	PROPERTY_STRING  (formName, FormName)				///< form name
	PROPERTY_STRING  (groupID, GroupID)					///< associates this with a frame in a perspective
	PROPERTY_STRING  (controllerUrl, ControllerUrl)		///< object table url of associated controller
	PROPERTY_STRING  (cmdCategory, CommandCategory)		///< command category
	PROPERTY_STRING  (cmdName, CommandName)				///< command name
	PROPERTY_BOOL    (defaultVisible, DefaultVisible)	///< true if window should be visible by default
	PROPERTY_BOOL    (allowMultiple, AllowMultiple)		///< true if multiple window instances allowed
	PROPERTY_MUTABLE_CSTRING (workspaceID, WorkspaceID) ///< in which workspace this should appear
	PROPERTY_MUTABLE_CSTRING (storageID, StorageID)		///< optional alternative id for storing gui states (instead of id)

	// IWindowClass
	StringID CCL_API getClassID () const override { return getID (); }
	void CCL_API setVerifier (IWindowClassVerifier* verifier) override;
	void CCL_API setCommand (StringID category, StringID name) override;
	void CCL_API getCommand (MutableCString& category, MutableCString& name) const override;

	CLASS_INTERFACE (IWindowClass, Object)

protected:
	friend class WindowManager;
	ObjectArray openParams;

private:
	Theme* theme;
	IWindowClassVerifier* verifier;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// WindowSystem
//************************************************************************************************

class WindowSystem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (WindowSystem, Object)

	virtual bool openWindow (WindowClassRef windowID) = 0;
	virtual bool closeWindow (WindowClassRef windowID) = 0;
	virtual bool replaceWindow (WindowClassRef oldID, WindowClassRef newID) = 0;
	virtual bool centerWindow (WindowClassRef windowID) = 0;
	virtual bool canReuseWindow (WindowClassRef oldClass) = 0;
	virtual bool canOpenWindow (WindowClassRef windowID) = 0;
	virtual bool isWindowOpen (WindowClassRef windowID) = 0;
	virtual void storeWindowStates (Settings& settings) = 0;
	virtual void restoreWindowStates (Settings& settings) = 0;
};

//************************************************************************************************
// WindowManager
//************************************************************************************************

class WindowManager: public Object,
					 public AbstractController,
					 public IWindowManager,
					 public ICommandHandler,
					 public IParamObserver,
					 public CommandDispatcher<WindowManager>,
					 public Singleton<WindowManager>
{
public:
	DECLARE_CLASS (WindowManager, Object)
	DECLARE_COMMANDS (WindowManager)
	DECLARE_METHOD_NAMES (WindowManager)

	WindowManager ();
	~WindowManager ();

	void setWindowSystem (WindowSystem* windowSystem);

	Iterator* getClasses () const;
	const WindowClass* getClass (StringID id) const;
	void registerClass (WindowClass* windowClass);
	void unregisterClass (WindowClass* windowClass);
	bool isClassRegistered (WindowClass* windowClass) const;

	// Workspace instances
	void registerWorkspaceInstance (StringID workspaceID, StringID instanceID, IActivatable* activatable = nullptr);
	void unregisterWorkspaceInstance (StringID workspaceID, StringID instanceID);
	void onWorkspaceInstanceActivated (StringID workspaceID, StringID instanceID);

	void onWindowStateChanged (const WindowClass& windowClass, bool open);
	void onWindowStateChanged (const WindowClass& windowClass, StringID instanceID, bool open);

	void setMenuBarView (View* content); // takes ownership
	void setStatusBarView (View* content); // takes ownership
	void setNavigationBarView (View* content); // takes ownership
	void setLeftMarginView (View* content); // takes ownership
	void setRightMarginView (View* content); // takes ownership
	View* getApplicationContainerView () const;
	
	// IWindowManager
	IView* CCL_API createApplicationView (const Rect& bounds) override;
	IWindow* CCL_API createApplicationWindow (tbool show = true) override;
	IMenuBar* CCL_API createApplicationMenuBar (tbool variant) override;
	void CCL_API initWindowlessApplication () override;
	tbool CCL_API isWindowOpen (StringID windowClassId) override;
	tbool CCL_API openWindow (StringID windowClassId, tbool toggle = false, IAttributeList* arguments = nullptr) override;
	tbool CCL_API closeWindow (StringID windowClassId, tbool forceNow = false) override;
	tbool CCL_API replaceWindow (StringID oldClassID, StringID newClassID) override;
	tbool CCL_API centerWindow (StringID windowClassId) override;
	tbool CCL_API canOpenWindow (StringID windowClassId) override;
	tbool CCL_API isWindowOpen (IWindowClass* windowClass) override;
	tbool CCL_API openWindow (IWindowClass* windowClass, tbool toggle = false, IAttributeList* arguments = nullptr) override;
	tbool CCL_API closeWindow (IWindowClass* windowClass, tbool forceNow = false) override;
	tbool CCL_API replaceWindow (IWindowClass* oldClass, IWindowClass* newClass) override;
	tbool CCL_API centerWindow (IWindowClass* windowClass) override;
	tbool CCL_API canReuseWindow (IWindowClass* oldClass) override;
	tbool CCL_API suspendActivation (tbool state) override;
	IWindowClass* CCL_API registerClass (StringID windowClassId, StringRef formName, StringRef controllerUrl, StringRef groupID, StringID workspaceID, StringID themeID, StringID storageID = nullptr) override;
	void CCL_API unregisterClass (IWindowClass* windowClass) override;
	IWindowClass* CCL_API findWindowClass (StringID windowClassId) override;
	IParameter* CCL_API getOpenParameter (IWindowClass* windowClass) override;
	IAliasParameter* CCL_API getVisibilityAliasParameter (StringID externalClassId) override;
	void CCL_API storeWindowStates () override;
	void CCL_API restoreWindowStates () override;

	bool shouldActivateWindows () const;

	PROPERTY_POINTER (const WindowClass, currentWindowClass, CurrentWindowClass)
	PROPERTY_POINTER (IAttributeList, currentArguments, CurrentArguments)

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	// IController
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index)	const override;
	IParameter* CCL_API findParameter (StringID name) const override;
	IParameter* CCL_API getParameterByTag (int tag) const override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	
	CLASS_INTERFACES (Object)

protected:
	mutable WindowSystem* windowSystem;
	ObjectArray windowClasses;
	ObjectArray workspaces;
	ParamContainer paramList;
	int nextParamId;
	bool autoActivate;
	ViewPtr app;
	ViewPtr container;
	ViewPtr menuBar;
	ViewPtr statusBar;
	ViewPtr navigationBar;
	ViewPtr leftMargin;
	ViewPtr rightMargin;
	
	class WorkspaceItem;
	class WorkspaceInstanceItem;

	// Commands
	bool onResetWindows (CmdArgs args);
	bool onToggleFullscreen (CmdArgs args);

	WindowSystem& getWindowSystem () const;
	WindowClass* lookupClass (IUnknown* controller) const;

	static void makeParamName (MutableCString& name, const WindowClass& windowClass, StringID instanceID);
	static void parseParamName (MutableCString& windowID, MutableCString& instanceID, StringID name);
	void addOpenParam (WindowClass& windowClass, StringID instanceID);
	void removeOpenParam (WindowClass& windowClass, StringID instanceID);
	Parameter* getOpenParam (const WindowClass& windowClass, StringID instanceID) const;
	Parameter* getActiveOpenParam (const WindowClass& windowClass) const;
	WorkspaceItem* getWorkspaceItem (StringID workspaceID) const;
	
	View* createBarView (const Rect& bounds, ThemeMetricID metricID);
	View* createApplicationViewInternal (IApplication* application, const Rect& bounds);
	void setBarViewInternal (IView* target, View* content);
	
	enum OpenMode { kOpen = 0, kClose, kToggle };
	tbool openCloseWindow (const WindowClass& wc, int mode);
	tbool closeWindow (const WindowClass& wc, tbool forceNow);
	tbool centerWindow (const WindowClass& wc);
	tbool canOpenWindow (const WindowClass& wc);
	tbool checkClosePopup (const WindowClass& wc, bool openWindow);
	void sizeViews ();
};

//************************************************************************************************
// DesktopWindowSystem
//************************************************************************************************

class DesktopWindowSystem: public WindowSystem
{
public:
	// WindowSystem
	DesktopWindowSystem ();

	bool openWindow (WindowClassRef windowID) override;
	bool closeWindow (WindowClassRef windowID) override;
	bool replaceWindow (WindowClassRef oldID, WindowClassRef newID) override;
	bool centerWindow (WindowClassRef windowID) override;
	bool canReuseWindow (WindowClassRef oldClass) override;
	bool canOpenWindow (WindowClassRef windowID) override;
	bool isWindowOpen (WindowClassRef windowID) override;
	void storeWindowStates (Settings& settings) override;
	void restoreWindowStates (Settings& settings) override;

protected:
	Window* getExistingWindow (WindowClassRef windowID) const;
	Window* createNewWindow (WindowClassRef windowID) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

bool inline WindowClass::isActive ()
{ return !verifier || verifier->isWindowClassActive (); }

StringID inline WindowClass::getViewStateID () const
{ return storageID.isEmpty () ? id : storageID; }

inline bool WindowManager::shouldActivateWindows () const
{ return autoActivate; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_windowmanager_h
