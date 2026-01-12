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
// Filename    : ccl/gui/windows/desktop.h
// Description : Desktop Management
//
//************************************************************************************************

#ifndef _ccl_desktop_h
#define _ccl_desktop_h

#include "ccl/gui/windows/window.h"

#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//************************************************************************************************
// DesktopManager
//************************************************************************************************

class DesktopManager: public Object,
					  public IDesktop
{
public:
	DECLARE_CLASS_ABSTRACT (DesktopManager, Object)
	DECLARE_METHOD_NAMES (DesktopManager)

	DesktopManager ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Windows
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual void addWindow (Window* window, WindowLayer layer);
	void removeWindow (Window* window);

	int CCL_API countWindows () const override;
	IWindow* CCL_API getWindow (int index) const override;
	IWindow* getWindowByName (StringRef name) const;
	IWindow* CCL_API getWindowByOwner (IUnknown* controller) const override;

	Window* getActiveWindow () const;
	Window* getFirstWindow () const;		///< back-most
	Window* getLastWindow () const;			///< top-most
	int getZIndex (const Window& window) const;
	bool hasFullScreenWindow () const;
	
	PROPERTY_BOOL (windowlessApplication, WindowlessApplication)
	IWindow* CCL_API getApplicationWindow () override;
	IMenuBar* CCL_API getApplicationMenuBar () override;
	IWindow* CCL_API getDialogParentWindow () override;
	IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0) override;
	IWindow* CCL_API findWindowUnderCursor (int flags = 0) override;

	void onActivateWindow (Window* w, bool state);
	virtual void onAppActivate (bool state);

	Window* getTopWindow (WindowLayer layer) const;
	int getStackDepth (WindowLayer layer) const;
	bool isPopupActive () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Monitors
	//////////////////////////////////////////////////////////////////////////////////////////////

	int CCL_API countMonitors () const override;
	int CCL_API getMainMonitor () const override;
	int CCL_API findMonitor (PointRef where, tbool defaultToPrimary) const override;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const override;
	float CCL_API getMonitorScaleFactor (int index) const override;
	
	int findNearestMonitor (RectRef rect) const;
	bool getVirtualScreenSize (Rect& rect, bool useWorkArea) const;	///< of all monitors
	bool isRectVisible (RectRef screenRect) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	//////////////////////////////////////////////////////////////////////////////////////////////

	tbool CCL_API closePopupAndDeferCommand (ICommandHandler* handler, const CommandMsg& cmd) override;
	tbool CCL_API closeModalWindows () override;
	tbool CCL_API closeTopModal (int dialogResult) override;
	void CCL_API flushUpdatesWithProgressWindows (IView* caller = nullptr) override;
	tbool CCL_API isInMode (int modeFlags) const override;
	void CCL_API redrawAll () override;

	bool isProgressMode () const;
	bool isInMenuLoop () const;
	bool isInModalMode () const;
	bool isInTextInput () const;
	bool closeAll ();
	
	void setGlobalMenuBar (IMenuBar* menuBar);

	DECLARE_STRINGID_MEMBER (kWindowAdded) ///< arg[0]: Window that was added to desktop
	
	CLASS_INTERFACE (IDesktop, Object)

protected:
	LinkedList<Window*> windows[kNumWindowLayers];
	IMenuBar* globalMenuBar;

	void setWindowLayer (Window* window, WindowLayer layer);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

extern DesktopManager& Desktop;	///< global Desktop instance

} // namespace CCL

#endif // _ccl_desktop_h
