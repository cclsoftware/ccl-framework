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
// Filename    : ccl/gui/windows/windowmanager.cpp
// Description : Window Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/gui.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/appwindow.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/commands.h"

#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/layout/workspace.h"
#include "ccl/gui/controls/colorbox.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum WindowManagerTags
	{
		kAliasParam = 1,
		kOpenWindow = 100
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (WindowManager)
	DEFINE_COMMAND ("View", "Reset Window Positions",  WindowManager::onResetWindows)
	DEFINE_COMMAND_ARGS ("View", "Fullscreen", WindowManager::onToggleFullscreen, 0, "State")
END_COMMANDS (WindowManager)

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWindowManager& CCL_API System::CCL_ISOLATED (GetWindowManager) ()
{
	return WindowManager::instance ();
}

//************************************************************************************************
// WindowClass
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WindowClass, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClass::WindowClass ()
: theme (nullptr),
  verifier (nullptr),
  defaultVisible (false),
  allowMultiple (false)
{
	openParams.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClass::~WindowClass ()
{
	setTheme (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme* WindowClass::getTheme () const
{
	return theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowClass::setTheme (Theme* _theme)
{
	theme = _theme;
	// Note: When created by skin, retaining theme causes a circular reference!!!
	//take_shared (theme, _theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WindowClass::getController () const
{
	if(controllerUrl.isEmpty () == false)
	{
		Url url (controllerUrl);
		IUnknown* controller = System::GetObjectTable ().getObjectByUrl (url);
		SOFT_ASSERT (controller, MutableCString (String ("WindowClass::getController (") << id << "): " << controllerUrl).str ())
		return controller;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowClass::setVerifier (IWindowClassVerifier* verifier)
{
	this->verifier = verifier;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowClass::setCommand (StringID category, StringID name)
{
	setCommandCategory (String (category));
	setCommandName (String (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowClass::getCommand (MutableCString& category, MutableCString& name) const
{
	category = getCommandCategory ();
	name = getCommandName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowClass::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "group")
	{
		var = getGroupID ();
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// WindowSystem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WindowSystem, Object)

//************************************************************************************************
// WindowManager::WorkspaceInstanceItem
//************************************************************************************************

class WindowManager::WorkspaceInstanceItem: public Object
{
public:
	PROPERTY_MUTABLE_CSTRING (instanceID, InstanceID)
	PROPERTY_SHARED_AUTO (IActivatable, activatable, Activatable)
};

//************************************************************************************************
// WindowManager::WorkspaceItem
//************************************************************************************************

class WindowManager::WorkspaceItem: public ObjectList
{
public:
	WorkspaceItem ()
	{ objectCleanup (true); }

	PROPERTY_MUTABLE_CSTRING (workspaceID, WorkspaceID)
	PROPERTY_MUTABLE_CSTRING (activeInstance, ActiveInstance)

	WorkspaceInstanceItem* getInstance (StringID instanceID) const
	{
		ForEach (*this, WorkspaceInstanceItem, instance)
			if(instance->getInstanceID () == instanceID)
				return instance;
		EndFor
		return nullptr;
	}
};

//************************************************************************************************
// WindowManager
//************************************************************************************************

CCL_KERNEL_TERM_LEVEL (WindowManager, kFrameworkLevelSecond)
{
	if(WindowManager* wm = WindowManager::peekInstance ())
	{
		System::GetObjectTable ().unregisterObject (wm->asUnknown ());

		if(CommandTable* ct = CommandTable::peekInstance ())
			ct->removeHandler (wm);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (WindowManager, Object)
DEFINE_SINGLETON (WindowManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowManager::WindowManager ()
: windowSystem (nullptr),
  nextParamId (0),
  autoActivate (true),
  currentWindowClass (nullptr),
  currentArguments (nullptr)
{
	paramList.setController (this);
	windowClasses.objectCleanup (true);
	workspaces.objectCleanup (true);

	CommandTable::instance ().addHandler (this);
	System::GetObjectTable ().registerObject (asUnknown (), kNullUID, "WindowManager");
	SignalSource::addObserver (Signals::kGUI, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowManager::~WindowManager ()
{
	cancelSignals ();
	if(windowSystem)
		windowSystem->release ();
	SignalSource::removeObserver (Signals::kGUI, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowManager::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IWindowManager)
	QUERY_INTERFACE (ICommandHandler)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IController)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setWindowSystem (WindowSystem* _windowSystem)
{
	take_shared<WindowSystem> (windowSystem, _windowSystem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowSystem& WindowManager::getWindowSystem () const
{
	if(!windowSystem)
		windowSystem = NEW DesktopWindowSystem;
	return *windowSystem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* WindowManager::getClasses () const
{
	return windowClasses.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WindowClass* WindowManager::getClass (StringID id) const
{
	WindowClass* foundClass = nullptr;

	ForEach (windowClasses, WindowClass, w)
		if(id == w->getID () && w->isActive ())
		{
			// prefer class from current theme, but use first other class as fallback
			if(!ThemeSelector::currentTheme || w->getTheme () == ThemeSelector::currentTheme)
				return w;
			else if(!foundClass)
				foundClass = w;
		}
	EndFor
	return foundClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClass* WindowManager::lookupClass (IUnknown* controller) const
{
	// there might be multiple IUnknown's, 
	// we have to compare the "basic" one...
	ForEach (windowClasses, WindowClass, w)
		if(isEqualUnknown (w->getController (), controller))
			return w;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::registerClass (WindowClass* windowClass)
{
	ASSERT (windowClass != nullptr)
	if(windowClass == nullptr)
		return;

	CCL_PRINTF ("WindowManager::registerClass %x %s (%s)\n", windowClass, MutableCString (windowClass->getID ()).str (), MutableCString (windowClass->getWorkspaceID ()).str ());

	windowClasses.add (windowClass);
	addOpenParam (*windowClass, windowClass->getID ());
	
	// add params for existing workspace instances
	if(!windowClass->getWorkspaceID ().isEmpty ())
		if(WorkspaceItem* workspaceItem = getWorkspaceItem (windowClass->getWorkspaceID ()))
			ForEach (*workspaceItem, WorkspaceInstanceItem, instance)
				addOpenParam (*windowClass, instance->getInstanceID ());
			EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::unregisterClass (WindowClass* windowClass)
{
	ASSERT (windowClass != nullptr)
	if(windowClass == nullptr)
		return;

	CCL_PRINTF ("WindowManager::unregisterClass %x %s (%s)\n", windowClass, MutableCString (windowClass->getID ()).str (), MutableCString (windowClass->getWorkspaceID ()).str ());
	// remove all params for this windwclass
	ArrayForEachFast (windowClass->openParams, Parameter, param)
		paramList.remove (param);
		param->release ();
	EndFor

	windowClasses.remove (windowClass);
	windowClass->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowManager::isClassRegistered (WindowClass* windowClass) const
{
	ForEach (windowClasses, WindowClass, w)
		if(w == windowClass)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindowClass* CCL_API WindowManager::registerClass (StringID windowClassId, StringRef formName, StringRef controllerUrl, StringRef groupID, StringID workspaceID, StringID themeID, StringID storageID)
{
	WindowClass* wc = NEW WindowClass;
	wc->setID (windowClassId);
	wc->setFormName (formName);
	wc->setControllerUrl (controllerUrl);
	wc->setGroupID (groupID);
	wc->setWorkspaceID (workspaceID);
	wc->setStorageID (storageID);

	Theme* theme = unknown_cast<Theme> (ThemeManager::instance ().getTheme (themeID));
	ASSERT (theme != nullptr)
	if(!theme)
		theme = &ThemeManager::instance ().getDefaultTheme ();
	wc->setTheme (theme);

	registerClass (wc);
	return wc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::unregisterClass (IWindowClass* windowClass)
{
	unregisterClass (unknown_cast <WindowClass> (windowClass));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindowClass* CCL_API WindowManager::findWindowClass (StringID windowClassId)
{
	return const_cast<WindowClass*> (getClass (windowClassId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API WindowManager::getOpenParameter (IWindowClass* wc)
{
	WindowClass* windowClass = unknown_cast <WindowClass> (wc);
	ASSERT (windowClass)
	return windowClass ? getActiveOpenParam (*windowClass) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::makeParamName (MutableCString& name, const WindowClass& windowClass, StringID instanceID)
{
	name = windowClass.getID ();
	if(!instanceID.isEmpty ())
	{
		name += "@";
		name += instanceID;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::parseParamName (MutableCString& windowID, MutableCString& instanceID, StringID name)
{
	int separator = name.index ('@');
	windowID = name.subString (0, separator);
	if(separator > 0)
		instanceID = name.subString (separator + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::addOpenParam (WindowClass& windowClass, StringID instanceID)
{
	MutableCString name;
	makeParamName (name, windowClass, instanceID);

	Parameter* param = NEW Parameter (name);
	paramList.add (param, Tag::kOpenWindow + nextParamId++);
	param->retain ();
	windowClass.openParams.add (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::removeOpenParam (WindowClass& windowClass, StringID instanceID)
{
	if(Parameter* param = getOpenParam (windowClass, instanceID))
	{
		windowClass.openParams.remove (param);
		param->release ();

		paramList.remove (param);
		param->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* WindowManager::getOpenParam (const WindowClass& windowClass, StringID instanceID) const
{
	MutableCString name;
	makeParamName (name, windowClass, instanceID);

	ArrayForEachFast (windowClass.openParams, Parameter, param)
		if(param->getName () == name)
			return param;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* WindowManager::getActiveOpenParam (const WindowClass& windowClass) const
{
	if(!windowClass.getWorkspaceID ().isEmpty ())
		if(WorkspaceItem* workspaceItem = getWorkspaceItem (windowClass.getWorkspaceID ()))
		{
			MutableCString name;
			makeParamName (name, windowClass, workspaceItem->getActiveInstance ());

			ArrayForEachFast (windowClass.openParams, Parameter, param)
				if(param->getName () == name)
					return param;
			EndFor
			ASSERT (0)
		}

	return (Parameter*)windowClass.openParams.at (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAliasParameter* CCL_API WindowManager::getVisibilityAliasParameter (StringID externalClassId)
{
	StringID name (externalClassId);
	IAliasParameter* p = UnknownPtr<IAliasParameter> (paramList.lookup (name));
	if(!p)
		p = paramList.addAlias (name, Tag::kAliasParam);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowManager::WorkspaceItem* WindowManager::getWorkspaceItem (StringID workspaceID) const
{
	ArrayForEachFast (workspaces, WorkspaceItem, workspaceItem)
		if(workspaceItem->getWorkspaceID () == workspaceID)
			return workspaceItem;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::registerWorkspaceInstance (StringID workspaceID, StringID instanceID, IActivatable* activatable)
{
	WorkspaceItem* workspaceItem = getWorkspaceItem (workspaceID);
	if(!workspaceItem)
	{
		workspaceItem = NEW WorkspaceItem;
		workspaceItem->setWorkspaceID (workspaceID);
		workspaceItem->setActiveInstance (instanceID);
		workspaces.add (workspaceItem);
	}
	WorkspaceInstanceItem* instance = NEW WorkspaceInstanceItem;
	instance->setInstanceID (instanceID);
	instance->setActivatable (activatable);
	workspaceItem->add (instance);

	// add instance parameter to affected window classes
	ArrayForEachFast (windowClasses, WindowClass, w)
		if(w->getWorkspaceID () == workspaceID)
			addOpenParam (*w, instanceID);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::unregisterWorkspaceInstance (StringID workspaceID, StringID instanceID)
{
	WorkspaceItem* workspaceItem = getWorkspaceItem (workspaceID);
	ASSERT (workspaceItem)
	WorkspaceInstanceItem* instance = workspaceItem ? workspaceItem->getInstance (instanceID) : nullptr;
	ASSERT (instance)
	if(instance)
	{
		workspaceItem->remove (instance);
		instance->release ();
		if(workspaceItem->isEmpty ())
		{
			workspaces.remove (workspaceItem);
			workspaceItem->release ();
		}
	}

	// remove instance parameter from affected window classes
	ArrayForEachFast (windowClasses, WindowClass, w)
		if(w->getWorkspaceID () == workspaceID)
			removeOpenParam (*w, instanceID);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::onWorkspaceInstanceActivated (StringID workspaceID, StringID instanceID)
{
	WorkspaceItem* workspaceItem = getWorkspaceItem (workspaceID);
	ASSERT (workspaceItem)
	if(workspaceItem)
		workspaceItem->setActiveInstance (instanceID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowManager::countParameters () const
{
	return paramList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API WindowManager::getParameterAt (int index) const
{
	return paramList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API WindowManager::getParameterByTag (int tag) const
{
	return paramList.byTag (tag);
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API WindowManager::findParameter (StringID name) const
{
	ArrayForEachFast (windowClasses, WindowClass, w)
		if(w->getID () == name)
			return getActiveOpenParam (*w);
	EndFor

	return paramList.findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowManager::checkClosePopup (const WindowClass& wc, bool openWindow)
{
	if(!openWindow)
	{
		// check if we really need to close  popups: only if the closing window is parent of a popup
		if(IWindow* closingWindow = Desktop.getWindowByOwner (wc.getController ()))
		{
			// start with topmost (modal) popup window (that would be closed by Desktop::closePopupAndDeferCommand)
			IWindow* topWindow = ccl_cast<Dialog> (Desktop.getTopWindow (kPopupLayer)); 
			IPopupSelectorWindow* rootPopup = UnknownPtr<IPopupSelectorWindow> (topWindow);
			if(rootPopup)
			{
				// for nested popup windows, traverse "parent" chain up to the "root" popup (whose parent is not a popup)
				UnknownPtr<IPopupSelectorWindow> parentPopup;
				while(parentPopup = rootPopup->getParentWindow ())
					rootPopup = parentPopup;
			}

			// if parent window is not the closing one, we can leave the popup(s) open
			IWindow* parentWindow = rootPopup ? rootPopup->getParentWindow () : nullptr;
			if(parentWindow && parentWindow != closingWindow)
				return false;
		}
	}

	// close any popup selector first and if that is necessary, defer opening / closing wc
	CommandMsg cmd (openWindow ? CSTR ("openWindow") : CSTR ("closeWindow"), wc.getID ());
	return Desktop.closePopupAndDeferCommand (this, cmd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::isWindowOpen (StringID windowClassId)
{
	const WindowClass* wc = getClass (windowClassId);
	return wc ? getWindowSystem ().isWindowOpen (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::isWindowOpen (IWindowClass* windowClass)
{
	const WindowClass* wc = unknown_cast <WindowClass> (windowClass);
	return wc ? getWindowSystem ().isWindowOpen (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowManager::openCloseWindow (const WindowClass& wc, int mode)
{
	WindowSystem& windowSystem = getWindowSystem ();
	bool mustClose = (mode == kClose) || (mode == kToggle && windowSystem.isWindowOpen (wc));
	
	if(checkClosePopup (wc, !mustClose))
		return true;

	if(mustClose)
		return windowSystem.closeWindow (wc);

	signal (Message (kBeforeOpenWindow, Variant (wc.getClassID ())));

	bool opened = windowSystem.openWindow (wc); // even if already open, brings it to front
	if(!opened && !isWindowOpen (const_cast<WindowClass*> (&wc)))
		onWindowStateChanged (wc, false); // reset param if window cannot be opened
	return opened;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::openWindow (StringID windowClassId, tbool toggle, IAttributeList* arguments)
{
	const WindowClass* wc = getClass (windowClassId);
	ASSERT (wc)
	ScopedVar<const WindowClass*> scope1 (currentWindowClass, wc);
	ScopedVar<IAttributeList*> scope2 (currentArguments, arguments);
	return wc ? openCloseWindow (*wc, toggle ? kToggle : kOpen) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::openWindow (IWindowClass* windowClass, tbool toggle, IAttributeList* arguments)
{
	const WindowClass* wc = unknown_cast <WindowClass> (windowClass);
	ASSERT (wc)
	ScopedVar<const WindowClass*> scope1 (currentWindowClass, wc);
	ScopedVar<IAttributeList*> scope2 (currentArguments, arguments);
	return wc ? openCloseWindow (*wc, toggle ? kToggle : kOpen) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowManager::closeWindow (const WindowClass& wc, tbool forceNow)
{
	if(checkClosePopup (wc, false))
		if(!forceNow) // force: check closing popup, but close window immediately
		return true;

	ScopedVar<const WindowClass*> scope (currentWindowClass, &wc);
	return getWindowSystem ().closeWindow (wc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowManager::centerWindow (const WindowClass& wc)
{
	ScopedVar<const WindowClass*> scope (currentWindowClass, &wc);
	return getWindowSystem ().centerWindow (wc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::closeWindow (StringID windowClassId, tbool forceNow)
{
	const WindowClass* wc = getClass (windowClassId);
	if(wc)
	{
		tbool result = closeWindow (*wc, forceNow);
		if(!result && isWindowOpen (const_cast<WindowClass*> (wc)))
			onWindowStateChanged (*wc, true); // restore param value if close failed
		return result;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::closeWindow (IWindowClass* windowClass, tbool forceNow)
{
	const WindowClass* wc = unknown_cast <WindowClass> (windowClass);
	return wc ? closeWindow (*wc, forceNow) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::replaceWindow (StringID oldClassID, StringID newClassID)
{
	ActivationSuspender activationSuspender (*this, GUI.isApplicationActive () == false);
	
	const WindowClass* wcOld = getClass (oldClassID);
	const WindowClass* wcNew = getClass (newClassID);
	return (wcOld && wcNew) ? getWindowSystem ().replaceWindow (*wcOld, *wcNew) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::centerWindow (StringID windowClassId)
{
	const WindowClass* wc = getClass (windowClassId);
	return wc ? getWindowSystem ().centerWindow (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::replaceWindow (IWindowClass* oldClass, IWindowClass* newClass)
{
	ActivationSuspender activationSuspender (*this, GUI.isApplicationActive () == false);

	const WindowClass* wcOld = unknown_cast <WindowClass> (oldClass);
	const WindowClass* wcNew = unknown_cast <WindowClass> (newClass);
	return (wcOld && wcNew) ? getWindowSystem ().replaceWindow (*wcOld, *wcNew) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::centerWindow (IWindowClass* windowClass)
{
	const WindowClass* wc = unknown_cast <WindowClass> (windowClass);
	return wc ? centerWindow (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::canReuseWindow (IWindowClass* oldClass)
{
	const WindowClass* wc = unknown_cast <WindowClass> (oldClass);
	return wc ? getWindowSystem ().canReuseWindow (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::canOpenWindow (StringID windowClassId)
{
	const WindowClass* wc = getClass (windowClassId);
	return wc ? canOpenWindow (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::suspendActivation (tbool state)
{
	tbool wasSupended = autoActivate == 0;
	autoActivate = state == 0;
	return wasSupended;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowManager::canOpenWindow (const WindowClass& wc)
{
	return getWindowSystem ().canOpenWindow (wc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::storeWindowStates ()
{
	if(windowSystem)
		windowSystem->storeWindowStates (Window::getWindowSettings ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::restoreWindowStates ()
{
	if(windowSystem)
		windowSystem->restoreWindowStates (Window::getWindowSettings ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setBarViewInternal (IView* target, View* content)
{
	if(target)
	{
		target->getChildren ().removeAll ();
		if(content)
		{
			bool horizontal = target == statusBar || target == navigationBar || target == menuBar;
			if( horizontal && (content->getSizeMode () & (View::kAttachLeft | View::kAttachRight)) == (View::kAttachLeft | View::kAttachRight)
				|| ((content->getSizeMode () & (View::kAttachTop | View::kAttachBottom)) == (View::kAttachTop | View::kAttachBottom)) )
			{
				Rect size (content->getSize ());
				if(target == menuBar)
				{
					// content defines height
					size.setWidth (target->getSize ().getWidth ());
					target->setSize (Rect (target->getSize ()).setHeight (size.getHeight ()));
				}
				else
					size.setSize (target->getSize ().getSize ());
				content->getSizeLimits ().makeValid (size);
				content->setSize (size);
			}
			
			target->getChildren ().add (content);
			if(target == menuBar)
				sizeViews ();
		}
		else if(target == menuBar)
		{
			target->setSize (Rect (target->getSize ()).setHeight (0));
			sizeViews ();
		}
	}
	else if(content)
		content->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setMenuBarView (View* content)
{
	setBarViewInternal (menuBar, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setStatusBarView (View* content)
{
	setBarViewInternal (statusBar, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setNavigationBarView (View* content)
{
	setBarViewInternal (navigationBar, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setLeftMarginView (View* content)
{
	setBarViewInternal (leftMargin, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::setRightMarginView (View* content)
{
	setBarViewInternal (rightMargin, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* WindowManager::getApplicationContainerView () const
{
	return unknown_cast<View> (container);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* WindowManager::createBarView (const Rect& bounds, ThemeMetricID metricID)
{
	Coord length = 0;
	if(NativeThemePainter::instance ().getSystemMetric (length, metricID))
	{
		int sizeMode = 0;
		Rect barSize;
		switch(metricID)
		{
		case ThemeElements::kSystemStatusBarHeight :
			barSize (0, 0, bounds.getWidth (), length);
			sizeMode = View::kAttachLeft | View::kAttachRight | View::kAttachTop;
			break;
		case ThemeElements::kSystemNavigationBarHeight :
			barSize (0, bounds.getHeight () - length, bounds.getWidth (), bounds.getHeight ());
			sizeMode = View::kAttachLeft | View::kAttachRight | View::kAttachBottom ;
			break;
		case ThemeElements::kSystemMarginLeft :
			barSize (0, 0, length, bounds.getHeight ());
			sizeMode = View::kAttachTop | View::kAttachBottom | View::kAttachLeft;
			break;
		case ThemeElements::kSystemMarginRight :
			barSize (bounds.getWidth () - length, 0, bounds.getWidth (), bounds.getHeight ());
			sizeMode = View::kAttachTop | View::kAttachBottom | View::kAttachRight;
			break;
		default:
			ASSERT (0);
		}

		View* bar = NEW View (barSize); // bar is transparent by default
		bar->setSizeMode (sizeMode);
		
		SizeLimit barLimits;
		barLimits.setUnlimited ();
		if(metricID == ThemeElements::kSystemStatusBarHeight || metricID == ThemeElements::kSystemNavigationBarHeight)
			barLimits.setFixedHeight (length);
		else
			barLimits.setFixedWidth (length);
		
		return bar;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* WindowManager::createApplicationViewInternal (IApplication* application, const Rect& bounds)
{
	if(!application)
		return nullptr;

	// 1.) check if there is an application workspace
	StringID appID = application->getApplicationID ();
	if(Workspace* appWorkspace = unknown_cast<Workspace> (WorkspaceSystem::instance ().getWorkspace (appID)))
		if(View* view = appWorkspace->createWorkspaceView (bounds))
			return view;

	// 2.) try application as IViewFactory
	ITheme* theme = application->getApplicationTheme ();
	if(theme)
	{
		IView* view = theme->createView (IWindowManager::kApplicationFormName, application);
		return unknown_cast<View> (view);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API WindowManager::createApplicationView (const Rect& _bounds)
{
	IApplication* application = GUI.getApplication ();
	ThemeSelector themeSelector (application ? unknown_cast<Theme> (application->getApplicationTheme ()) : nullptr);
	
	Rect bounds (_bounds);

	View* menuBarView = nullptr;

	if(ApplicationWindow::isUsingCustomMenuBar ())
	{
		menuBarView = NEW View (Rect (0, 0, bounds.getWidth (), 0)); // empty until content is set
		menuBarView->setSizeMode (View::kAttachLeft | View::kAttachRight | View::kAttachTop);
		menuBar = menuBarView;
	}

	View* statusBarView = createBarView (bounds, ThemeElements::kSystemStatusBarHeight);
	statusBar = statusBarView;
	
	View* navigationBarView = createBarView (bounds, ThemeElements::kSystemNavigationBarHeight);
	navigationBar = navigationBarView;

	View* leftMarginView = createBarView (bounds, ThemeElements::kSystemMarginLeft);
	leftMargin = leftMarginView;

	View* rightMarginView = createBarView (bounds, ThemeElements::kSystemMarginRight);
	rightMargin = rightMarginView;
	
	View* appView = createApplicationViewInternal (application, bounds);
	app = appView;
	if(!appView)
		return nullptr;

	appView->setSizeMode (View::kAttachAll);
	bounds = appView->getSize ();
	
	if(statusBarView || navigationBarView || leftMarginView || rightMarginView || menuBarView)
	{
		ImageView* containerView = NEW ImageView (nullptr, bounds, 0, appView->getTitle ());
		container = containerView;
		containerView->setSize (bounds);
		containerView->setSizeMode (View::kAttachAll);
		if(menuBarView)
			containerView->addView (menuBarView);
		if(statusBarView)
			containerView->addView (statusBarView);
		containerView->addView (appView);
		if(navigationBarView)
			containerView->addView (navigationBarView);
		if(leftMarginView)
			containerView->addView (leftMarginView);
		if(rightMarginView)
			containerView->addView (rightMarginView);

		// move background from application view to container
		if(ImageView* appImageView = ccl_cast<ImageView> (appView))
		{
			containerView->setVisualStyle (appImageView->getVisualStyleDirect ());
			containerView->setStyle (appImageView->getStyle ());

			appImageView->setVisualStyle (nullptr);
			View::StyleModifier (*appImageView).setCustomStyle (Styles::kImageViewAppearanceColorize, false);
		}
		
		appView = containerView;
	}
	
	sizeViews ();
	
	return appView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API WindowManager::createApplicationWindow (tbool show)
{
	Window* window = nullptr;

	StyleFlags windowStyle (0, ApplicationWindow::kDefaultStyle);
	Rect windowSize (0, 0, ApplicationWindow::kDefaultWidth, ApplicationWindow::kDefaultHeight);
	String title;

	// *** Create application view ***
	View* appView = unknown_cast<View> (createApplicationView (windowSize));
	if(appView)
	{
		windowSize = appView->getSize ();
		title = appView->getTitle ();
	}

	Form* form = ccl_cast<Form> (appView);
	if(!form && appView)
		form = ccl_cast<Form> (appView->getLast ()); // (wrapped in container)

	if(form)
	{
		StyleRef formStyle = form->getWindowStyle ();
		windowStyle.common = formStyle.common; // common styles allow background transparency, etc.

		if(formStyle.custom != 0) // only if defined explicitely
			windowStyle.custom = formStyle.custom;
	}

	// *** Create window ***
	window = NEW ApplicationWindow (GUI.getApplication (), windowSize, windowStyle, title);

	if(appView)
	{
		// note: window may now be smaller than windowSize, depending on available screen space
		windowSize = window->getSize ();
		windowSize.moveTo (Point ());
		appView->setSize (windowSize);
		appView->setSizeMode (View::kAttachAll);

		window->addView (appView);
	}

	if(appView && appView->hasVisualStyle ()) // note: if appView is wrapped in a container, visual style is moved to the container
		window->setVisualStyle (appView->getVisualStyleDirect ());
	else
		window->onVisualStyleChanged (); // make sure standard window title bar color is applied (if configured)

	if(form)
	{
		window->setController (form->getController ());
		window->setName (form->getName ());
		window->setHelpIdentifier (form->getHelpIdentifier ());
	}

	if(window && show)
		window->show ();

	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuBar* CCL_API WindowManager::createApplicationMenuBar (tbool variant)
{
	if(ApplicationWindow::isUsingCustomMenuBar ())
		return variant ? NEW ExtendedVariantMenuBar : NEW ExtendedMenuBar;
	else
		return ccl_new<IMenuBar> (variant ? ClassID::VariantMenuBar : ClassID::MenuBar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::initWindowlessApplication ()
{
	Desktop.setWindowlessApplication (true);

	#if CCL_PLATFORM_MAC // needed to replace global application menu
	if(IApplication* application = GUI.getApplication ())
	{
		static AutoPtr<IMenuBar> menuBar = application->createMenuBar ();
		Desktop.setGlobalMenuBar (menuBar);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::checkCommandCategory (CStringRef category) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::interpretCommand (const CommandMsg& msg)
{
	String category (msg.category);
	String name (msg.name);

	CommandAutomator::Arguments automatorArgs (msg);
	MutableCString workspaceID (automatorArgs.getString ("workspaceID"));

	ForEach (windowClasses, WindowClass, w)
		if(!w->getCommandCategory ().isEmpty ())
		{
			if(w->getCommandCategory () == category && w->getCommandName () == name
				&& (workspaceID.isEmpty () || w->getWorkspaceID () == workspaceID))
			{
				CCL_PRINT ("Found command ")
				CCL_PRINT (w->getCommandCategory ())
				CCL_PRINT (" ")
				CCL_PRINTLN (w->getCommandName ())

				WindowSystem& windowSystem = getWindowSystem ();
				bool canOpen = windowSystem.canOpenWindow (*w);
				
				if(msg.checkOnly ())
				{
					UnknownPtr<IMenuItem> menuItem (msg.invoker);
					if(menuItem)
					{
						bool checked = canOpen && windowSystem.isWindowOpen (*w);
						menuItem->setItemAttribute (IMenuItem::kItemChecked, checked);
					}
				}
				else
				{
					int mode = kToggle;

					bool isOpen;
					if(automatorArgs.getBool ("State", isOpen))
						mode = isOpen ? kOpen : kClose;

					if(canOpen)
						openCloseWindow (*w, mode);
				}

				if(canOpen)
					return true;
			}
		}
	EndFor

	UnknownPtr<ICommandHandler> handler (getWindowSystem ().asUnknown ());
	if(handler && handler->interpretCommand (msg))
		return true;

	if(!msg.checkOnly ())
	{
		// internal commands, from checkClosePopup
		if(msg.category == "openWindow")
			return openWindow (msg.name);
		else if (msg.category == "closeWindow")
			return closeWindow (msg.name);
	}

	return CommandDispatcher<WindowManager>::dispatchCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::paramChanged (IParameter* param)
{
	if(param->getTag () >= Tag::kOpenWindow)
	{
		//CCL_PRINTF ("WindowManager::paramChanged (%s): %s\n", param->getName ().str (), param->getValue () ? "open" : "close");
		Message* m = NEW Message (param->getValue () ? "open" : "close", String (param->getName ()));
		m->post (this);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowManager::notify (ISubject* subject, MessageRef msg)
{
	bool isOpen = (msg == "open");
	if(isOpen || msg == "close")
	{
		MutableCString windowID, instanceID;
		parseParamName (windowID, instanceID, MutableCString (msg[0].asString ()));
		
		if(const WindowClass* wc = getClass (windowID))
		{
			if(!wc->getWorkspaceID ().isEmpty ())
			{
				// activate workspace instance before
				WorkspaceItem* workspaceItem = getWorkspaceItem (wc->getWorkspaceID ());
				WorkspaceInstanceItem* instance = workspaceItem ? workspaceItem->getInstance (instanceID) : nullptr;
				if(instance && instance->getActivatable ())
					instance->getActivatable ()->activate ();
			}

			if(isOpen)
				openWindow (const_cast<WindowClass*> (wc));
			else
			{
				tbool closed = closeWindow (const_cast<WindowClass*> (wc));
				if(!closed && isWindowOpen (const_cast<WindowClass*> (wc)))
					onWindowStateChanged (*wc, true); // reset param if window cannot be closed
			}
		}
	}
	else if (msg == Signals::kSystemMetricsChanged)
		sizeViews ();
	else if (msg == Signals::kOrientationChanged)
    {
        // close popups, except when "sheet style" is used
        Window* window = Desktop.getTopWindow (kPopupLayer);
        if(window && !window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
        {
            UnknownPtr<IPopupSelectorWindow> popup (ccl_as_unknown (window));
            if(popup)
                popup->closePopup ();
        }

		signal (Message (kPropertyChanged, String ("isPortraitOrientation")));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::onWindowStateChanged (const WindowClass& windowClass, bool open)
{
	if(Parameter* param = getActiveOpenParam (windowClass))
		param->setValue (open);

	CCL_PRINTF ("onWindowStateChanged (%s): %s\n", MutableCString (windowClass.getID ()).str (), open ? "opened" : "closed");
	signal (Message (open ? kWindowOpened : kWindowClosed, Variant (windowClass.getClassID ())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::onWindowStateChanged (const WindowClass& windowClass, StringID instanceID, bool open)
{
	if(Parameter* param = getOpenParam (windowClass, instanceID))
		param->setValue (open);

	CCL_PRINTF ("onWindowStateChanged (%s): %s\n", MutableCString (windowClass.getID ()).str (), open ? "opened" : "closed");
	signal (Message (open ? kWindowOpened : kWindowClosed, Variant (windowClass.getClassID ())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowManager::onResetWindows (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		// collect all windows first (because setSize could change the order in Desktop)
		ObjectList windows;
		for(int i = 0, num = Desktop.countWindows (); i < num; i++)
			if(Window* window = unknown_cast<Window> (Desktop.getWindow (i)))
				windows.add (window);

		IWindow* appWindow = Desktop.getApplicationWindow ();

		Rect monitorSize;
		Desktop.getMonitorSize (monitorSize, Desktop.getMainMonitor (), true);

		enum { kHMargin = 50, kVMargin = 20, kSpacing = 25 };
		Point pos (monitorSize.getLeftTop () + Point (kHMargin, kVMargin));

		ForEach (windows, Window, window)
			Rect windowSize (window->getSize ());
			if(window == appWindow)
			{
				// if application window is not visible: center on main monitor
				if(!Desktop.isRectVisible (windowSize))
					window->setSize (windowSize.center (monitorSize));
			}
			else
			{
				// position other windows across main monitor 
				window->moveWindow (pos);

				pos += Point (kSpacing, kSpacing);
				if(pos.y > monitorSize.bottom - kVMargin)
					pos.y = monitorSize.top + kVMargin;
				if(pos.x > monitorSize.right - kHMargin)
					pos.x = monitorSize.left + kHMargin;
			}
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowManager::onToggleFullscreen (CmdArgs args)
{
	Window* window = Desktop.getActiveWindow ();
	if(window && window->getStyle ().isCustomStyle (Styles::kWindowBehaviorFullscreen))
	{
		if(!args.checkOnly ())
		{
			bool state = true;
			if(CommandAutomator::Arguments (args).getBool ("state", state))
				window->setFullscreen (state);
			else
			{
				// toggle (try to switch on / off - we don't know the current state before)
				for(tbool state : { true, false })
				{
					tbool oldState = window->setFullscreen (state);
					if(oldState != state)
						break;
				}
			}
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowManager::sizeViews ()
{
	if(!app || !container)
		return;
	
	Rect containerBounds = container->getSize ();
	Rect appBounds = containerBounds;
	
	if(menuBar)
	{
		int barHeight = menuBar->getSize ().getHeight (); // take height from content
		Rect menuBarSize (0, 0, Point (containerBounds.getWidth (), barHeight));
		menuBar->setSize (menuBarSize);
		appBounds.top += barHeight;
	}
	if(statusBar)
	{
		int barHeight = 0;
		NativeThemePainter::instance ().getSystemMetric (barHeight, ThemeElements::kSystemStatusBarHeight);
		Rect statusBarSize (0, 0, Point (containerBounds.getWidth (), barHeight));
		statusBar->setSize (statusBarSize);
		appBounds.top += barHeight;
	}
	if(leftMargin)
	{
		int barWidth = 0;
		NativeThemePainter::instance ().getSystemMetric (barWidth, ThemeElements::kSystemMarginLeft);
		Rect leftMarginSize (0, 0, Point (barWidth, containerBounds.getHeight ()));
		leftMargin->setSize (leftMarginSize);
		appBounds.left += barWidth;
	}
	if(navigationBar)
	{
		int barHeight = 0;
		NativeThemePainter::instance ().getSystemMetric (barHeight, ThemeElements::kSystemNavigationBarHeight);
		Rect navigationBarSize (0, containerBounds.bottom - barHeight, Point (containerBounds.getWidth (), barHeight));
		navigationBar->setSize (navigationBarSize);
		appBounds.bottom -= barHeight;
	}
	if(rightMargin)
	{
		int barWidth = 0;
		NativeThemePainter::instance ().getSystemMetric (barWidth, ThemeElements::kSystemMarginRight);
		Rect rightMarginSize (containerBounds.right - barWidth, 0 , Point (barWidth, containerBounds.getHeight ()));
		rightMargin->setSize (rightMarginSize);
		appBounds.right -= barWidth;
	}
	
	app->setSize (appBounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (WindowManager)
	DEFINE_METHOD_ARGR ("isWindowOpen", "classID: string", "bool")
	DEFINE_METHOD_ARGR ("openWindow", "classID: string, toggle: bool = false", "bool")
	DEFINE_METHOD_ARGR ("closeWindow", "classID: string", "bool")
	DEFINE_METHOD_ARGR ("centerWindow", "classID: string", "bool")
	DEFINE_METHOD_ARGR ("findParameter", "name: string", "Parameter")
END_METHOD_NAMES (WindowManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "isWindowOpen")	
	{
		MutableCString id (msg[0].asString ());
		returnValue = isWindowOpen (id);
		return true;
	}
	else if(msg == "openWindow")
	{
		MutableCString id (msg[0].asString ());
		bool toggle = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		returnValue = openWindow (id, toggle);
		return true;
	}
	else if(msg == "closeWindow")
	{
		MutableCString id (msg[0].asString ());
		returnValue = closeWindow (id);
		return true;
	}
	else if(msg == "centerWindow")
	{
		MutableCString id (msg[0].asString ());
		returnValue = centerWindow (id);
		return true;
	}
	else if(msg == "findParameter")
	{
		IParameter* p = findParameter (MutableCString (msg[0].asString ()));
		returnValue.takeShared (p);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isPortraitOrientation")
	{
		auto orientation = GUI.getInterfaceOrientation ();
		var = (orientation == Styles::kPortrait) ? true : false;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DesktopWindowSystem
//************************************************************************************************

DesktopWindowSystem::DesktopWindowSystem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopWindowSystem::getExistingWindow (WindowClassRef windowID) const
{
	return unknown_cast<Window> (Desktop.getWindowByOwner (windowID.getController ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopWindowSystem::createNewWindow (WindowClassRef windowID) const
{
	Form* form = nullptr;
	Theme* theme = windowID.getTheme ();
	ASSERT (theme != nullptr)
	if(theme)
		form = unknown_cast<Form> (theme->createView (MutableCString (windowID.getFormName ()), windowID.getController ()));
	return form ? form->open () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::openWindow (WindowClassRef windowID)
{
	Window* window = nullptr;
	if(windowID.isAllowMultiple ()) // if multiple instances allowed...
	{
		window = createNewWindow (windowID); // ...always create a new one
		if(window && !window->isVisible ())
			window->show ();
	}
	else
	{
		// if window already exists, activate it...
		window = getExistingWindow (windowID);
		if(window)
			window->activate ();
		else
		{
			window = createNewWindow (windowID); // ...else create a new one
			if(window)
			{
				if(!window->isVisible ())
					window->show ();
				else
					window->activate ();
			}
		}
	}
	return window != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::closeWindow (WindowClassRef windowID)
{
	Window* w = getExistingWindow (windowID);
	if(!w)
		return true;
	return w->close () ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::replaceWindow (WindowClassRef oldClass, WindowClassRef newClass)
{
	if(closeWindow (oldClass))
		return openWindow (newClass);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::centerWindow (WindowClassRef windowID)
{
	Window* w = getExistingWindow (windowID);
	if(!w)
		return true;

	w->center ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::canReuseWindow (WindowClassRef oldClass)
{
	return isWindowOpen (oldClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::canOpenWindow (WindowClassRef windowID)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopWindowSystem::isWindowOpen (WindowClassRef windowID)
{
	return getExistingWindow (windowID) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopWindowSystem::storeWindowStates (Settings& settings)
{
	IterForEach (WindowManager::instance ().getClasses (), WindowClass, w)
		bool visible = isWindowOpen (*w);

		String id (CCLSTR ("WindowState"));
		id.append (CCLSTR ("/"));
		id.appendASCII (w->getID ());

		Attributes& a = settings.getAttributes (id);
		a.set ("visible", visible);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopWindowSystem::restoreWindowStates (Settings& settings)
{
	IterForEach (WindowManager::instance ().getClasses (), WindowClass, w)
		String id (CCLSTR ("WindowState"));
		id.append (CCLSTR ("/"));
		id.appendASCII (w->getID ());

		CCL_PRINT ("WindowManager::restoreWindowStates restoring window: ");
		CCL_PRINT (id);
		CCL_PRINT ("\n");

		Attributes& a = settings.getAttributes (id);
		bool visible = w->isDefaultVisible ();
		if(a.contains ("visible"))
			visible = a.getBool ("visible");
		if(visible)
			openWindow (*w);
	EndFor
}
