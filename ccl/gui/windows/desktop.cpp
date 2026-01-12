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
// Filename    : ccl/gui/windows/desktop.cpp
// Description : Desktop Management
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/appwindow.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/dialogs/progressdialog.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/guiservices.h"

#include <math.h>

namespace CCL {

//************************************************************************************************
// CommandDeferrer
/** Deferes the execution of a command until a window is destroyed .*/
//************************************************************************************************

class CommandDeferrer: public Object
{
public:
	CommandDeferrer (IWindow* window, ICommandHandler* cmdHandler, const CommandMsg& cmd);

	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	SharedPtr<ICommandHandler> cmdHandler;
	CommandMsg cmd;
	MutableCString cmdCategory;
	MutableCString cmdName;
	SharedPtr<IUnknown> invoker;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IDesktop& CCL_API System::CCL_ISOLATED (GetDesktop) ()
{
	return Desktop;
}

//************************************************************************************************
// CommandDeferrer
//************************************************************************************************

CommandDeferrer::CommandDeferrer (IWindow* window, ICommandHandler* cmdHandler, const CommandMsg& cmd)
: cmdHandler (cmdHandler),
  cmd (cmd),
  cmdCategory (cmd.category),
  cmdName (cmd.name)
{
	// we need strings with own memory, lifetime of original CommandMsg is limited
	this->cmd.category = cmdCategory;
	this->cmd.name = cmdName;
	// we need to make sure the invoker is kept alive while deferring
	this->invoker = cmd.invoker;

	UnknownPtr<ISubject> (window)->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandDeferrer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		// popup was destroyed: defer ...
		subject->removeObserver (this);
		(NEW Message (CSTR ("perform")))->post (this);
	}
	else if(msg == "perform")
	{
		// now execute the command
		cmdHandler->interpretCommand (cmd);
		delete this;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachWindow \
	for(int layer = 0; layer < kNumWindowLayers; layer++) { ListForEach (windows[layer], Window*, w)
#define EndForWindow \
	EndFor }

//************************************************************************************************
// DesktopManager
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DesktopManager, Object)
DEFINE_STRINGID_MEMBER_ (DesktopManager, kWindowAdded, "WindowAdded")

//////////////////////////////////////////////////////////////////////////////////////////////////

DesktopManager::DesktopManager ()
: windowlessApplication (false),
  globalMenuBar (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::setWindowLayer (Window* window, WindowLayer layer)
{
	window->setLayer (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::addWindow (Window* window, WindowLayer layer)
{
	setWindowLayer (window, layer);
	windows[(int)layer].append (window);

	signal (Message (kWindowAdded, window->asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::removeWindow (Window* window)
{	
	ForEachWindow
		if(w == window)
		{
			windows[layer].remove (w);
			return;
		}
	EndForWindow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DesktopManager::countWindows () const
{
	int count = 0;
	for(int i = 0; i < kNumWindowLayers; i++)
		count += windows[i].count ();
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::getWindow (int _index) const
{
	int index = 0;
	ForEachWindow
		if(index == _index)
			return w;
		index++;
	EndForWindow
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* DesktopManager::getWindowByName (StringRef name) const
{
	ForEachWindow
		if(w->getName () == name)
			return w;
	EndForWindow
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::getWindowByOwner (IUnknown* controller) const
{
	ForEachWindow
		if(isEqualUnknown (controller, w->getController ()))
			return w;
	EndForWindow
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopManager::getActiveWindow () const
{
	ForEachWindow
		if(w->isActive ())
			return w;
	EndForWindow
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::closePopupAndDeferCommand (ICommandHandler* handler, const CommandMsg& cmd)
{
	auto popup = ccl_cast<Dialog> (getTopWindow (kPopupLayer)); // only modal
	if(popup)
	{
		UnknownPtr<IPopupSelectorClient> popupClient (popup->asUnknown ());
		if(popupClient)
		{
			NEW CommandDeferrer (popup, handler, cmd);
			popup->close ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::closeModalWindows ()
{
	ASSERT (!isProgressMode ())
	if(isProgressMode ())
		return false;

	// first create a local copy of the list of modal windows
	ObjectList modalWindows;
	modalWindows.objectCleanup (true);
	ListForEachReverse (windows[kDialogLayer], Window*, w)
		modalWindows.add (return_shared (w));
	EndFor

	bool result = true;
	ListForEachObject (modalWindows, Window, w)
		if(w->isInCloseEvent () || w->isInDestroyEvent ())
			continue;
		if(!w->close ())
			result = false;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::closeTopModal (int dialogResult)
{
	auto* dialog = ccl_cast<Dialog> (getTopWindow (kDialogLayer));
	if(dialog)
	{
		dialog->setDialogResult (dialogResult);
		return dialog->close ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DesktopManager::flushUpdatesWithProgressWindows (IView* caller)
{
	ProgressDialog::flushAll (unknown_cast<View> (caller));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isProgressMode () const
{
	return ProgressDialog::getFirstInstance () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isInMenuLoop () const
{
	ForEachWindow
		if(w->isInMenuLoop ())
			return true;
	EndForWindow
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isInModalMode () const
{
	return windows[kDialogLayer].isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isInTextInput () const
{
	return EditBox::isAnyEdtiting ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::isInMode (int modeFlags) const
{
	if((modeFlags & kProgressMode) && isProgressMode ())
		return true;
	if((modeFlags & kMenuLoopMode) && isInMenuLoop ())
		return true;
	if((modeFlags & kModalMode) && isInModalMode ())
		return true;
	if((modeFlags & kTextInputMode) && isInTextInput ())
		return true;
	if((modeFlags & kPopupMode) && isPopupActive ())
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::getDialogParentWindow ()
{
	if(IWindow* topModal = getTopWindow (kDialogLayer))
		if(Window* modalWindow = unknown_cast<Window> (topModal))
			if(!modalWindow->inDestroyEvent)
				return topModal;

	if(IWindow* activeWindow = getActiveWindow ())
		return activeWindow;

	return getLastWindow (); // top-most
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::getApplicationWindow ()
{
	ForEachWindow
		if(ccl_cast<ApplicationWindow> (w))
			return w;
	EndForWindow
    
    // mobile platforms typically create a ChildWindow instead of an ApplicationWindow
    if(GUI.getApplicationType () == GUI.kMobileApplication)
        return ccl_cast<ChildWindow> (getFirstWindow ()); // back-most (first added)

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuBar* CCL_API DesktopManager::getApplicationMenuBar ()
{
	if(Window* appWindow = unknown_cast<Window> (getApplicationWindow ()))
		return appWindow->getMenuBar ();
	return globalMenuBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopManager::getFirstWindow () const
{
	ForEachWindow
		return w;
	EndForWindow
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopManager::getLastWindow () const
{
	Window* _w = nullptr;
	ForEachWindow
		_w = w;
	EndForWindow
	return _w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DesktopManager::getZIndex (const Window& window) const
{
	int index = 0;
	ForEachWindow
		if(w == &window)
			return index;
		index++;
	EndForWindow
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::hasFullScreenWindow () const
{
	ForEachWindow
		if(w->isFullscreen ())
			return true;
	EndForWindow
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::closeAll ()
{
	// first create a local list of all windows
	ObjectList allWindows;
	allWindows.objectCleanup (true);
	ForEachWindow
		allWindows.add (return_shared (w));
	EndForWindow

	bool result = true;
	ListForEachObject (allWindows, Window, w)
		if(w->isInCloseEvent () || w->isInDestroyEvent ())
			continue;
		if(!w->close ())
			result = false;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DesktopManager::redrawAll ()
{
	ForEachWindow
		w->invalidate ();
	EndForWindow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* DesktopManager::getTopWindow (WindowLayer layer) const
{
	return windows[layer].getLast ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DesktopManager::getStackDepth (WindowLayer layer) const
{
	return windows[layer].count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isPopupActive () const
{
	if(IWindow* topModal = getTopWindow (kPopupLayer))
	{
		UnknownPtr<IPopupSelectorClient> popupClient (topModal);
		if(popupClient)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DesktopManager::findNearestMonitor (RectRef rect) const
{
	int count = countMonitors ();
	if(count <= 1)
		return 0;
		
	int centerX = rect.left + rect.getWidth () / 2;
	int centerY = rect.top + rect.getHeight () / 2;
	
	int nearestMonitor = 0;
	double maxPolarRadius = 10000000;
	for(int i = 0; i < count; i++)
	{
		Rect monitorRect;
		if(getMonitorSize (monitorRect, i, true))
		{
			int monitorCenterX = monitorRect.left + monitorRect.getWidth () / 2;
			int monitorCenterY = monitorRect.top + monitorRect.getHeight () / 2;
			
			double polarRadius = sqrt (powf (float(centerX - monitorCenterX), 2) * powf (float(centerY - monitorCenterY), 2));
			if(polarRadius < maxPolarRadius)
			{
				nearestMonitor = i;
				maxPolarRadius = polarRadius;
			}
		}
	}
	return nearestMonitor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::getVirtualScreenSize (Rect& rect, bool useWorkArea) const
{
	// combine all monitor rects
	rect.setReallyEmpty ();

	int count = countMonitors ();
	for(int i = 0; i < count; i++)
	{
		Rect monitorRect;
		if(getMonitorSize (monitorRect, i, useWorkArea))
			rect.join (monitorRect);
	}
	return !rect.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesktopManager::isRectVisible (RectRef screenRect) const
{
	Rect rect (screenRect);
	
	// contract, but avoid negative size (Rect::intersect would fail)
	enum { kShrink = 40 };
	if(rect.getWidth () > 2 * kShrink) 	
	{
		rect.left += kShrink;
		rect.right -= kShrink;
	}
	if(rect.getHeight () > 2 * kShrink)
	{
		rect.top += kShrink;
		rect.bottom -= kShrink;
	}

	int count = countMonitors ();
	for(int i = 0; i < count; i++)
	{
		Rect monitorRect;
		if(getMonitorSize (monitorRect, i, true))
		{
			if(rect.intersect (monitorRect))
				return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::setGlobalMenuBar (IMenuBar* menuBar)
{
	globalMenuBar = menuBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::onActivateWindow (Window* w, bool state)
{
#if DEBUG_LOG
	CCL_PRINT ("Window ")
	CCL_PRINT (w->getTitle ())
	CCL_PRINT (" ")
	if(state)
		CCL_PRINTLN ("activated")
	else
		CCL_PRINTLN ("deactivated")
#endif

	// maintain z-order: move topmost window to back
	if(state && windows[w->getLayer ()].remove (w))
	{
		auto* nonModalPopup = ccl_cast<NonModalPopupSelectorWindow> (getTopWindow (kPopupLayer));

		windows[w->getLayer ()].append (w);

		// but keep a NonModalPopupSelectorWindow child above its parent
		if(nonModalPopup
			&& nonModalPopup->getParentWindow () == w
			&& nonModalPopup->getLayer () == w->getLayer ())
		{
			if(windows[nonModalPopup->getLayer ()].remove (nonModalPopup))
				windows[nonModalPopup->getLayer ()].append (nonModalPopup);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesktopManager::onAppActivate (bool state)
{
	if(state == false)
	{
		// close popup selector
		if(IWindow* popup = getTopWindow (kPopupLayer))
		{
			UnknownPtr<IPopupSelectorClient> popupClient (popup);
			if(popupClient)
				popup->close ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::findWindow (PointRef _screenPos, int flags)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API DesktopManager::findWindowUnderCursor (int flags)
{
	Point p;
	System::GetGUI ().getMousePosition (p);
	return findWindow (p, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DesktopManager::countMonitors () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DesktopManager::getMainMonitor () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DesktopManager::findMonitor (PointRef where, tbool defaultToPrimary) const
{
	return defaultToPrimary ? 0 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::getMonitorSize (Rect& rect, int index, tbool useWorkArea) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API DesktopManager::getMonitorScaleFactor (int index) const
{
	return 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (DesktopManager)
	DEFINE_METHOD_NAME ("getApplicationWindow")
	DEFINE_METHOD_ARGR ("closeModalWindows", "", "bool")
	DEFINE_METHOD_ARGR ("closeTopModal", "dialogResult: int", "bool")
END_METHOD_NAMES (DesktopManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DesktopManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getApplicationWindow")
	{
		returnValue.takeShared (getApplicationWindow ());
		return true;
	}
	else if(msg == "closeModalWindows")
	{
		returnValue = closeModalWindows ();
		return true;
	}
	else if(msg == "closeTopModal")
	{
		int result = msg.getArgCount () > 0 ? msg[0].asInt () : DialogResult::kCancel;
		returnValue = closeTopModal (result);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
