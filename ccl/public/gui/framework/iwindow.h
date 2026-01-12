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
// Filename    : ccl/public/gui/framework/iwindow.h
// Description : Window Interface
//
//************************************************************************************************

#ifndef _ccl_iwindow_h
#define _ccl_iwindow_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/framework/guievent.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

interface IView;
interface IMenuBar;
interface IWindow;
interface IWindowEventHandler;

struct CommandMsg;
interface ICommandHandler;

//************************************************************************************************
// WindowEvent
/** Window event class.
	\ingroup gui_event */
//************************************************************************************************

struct WindowEvent: GUIEvent
{
	enum EventType
	{
		kClose,				///< windows is about to close
		kDestroy,			///< window was destroyed
		kActivate,			///< window was activated
		kDeactivate,		///< window was deactivated
		kMaximize,			///< window was maximized
		kUnmaximize,		///< window was restored from maximized state
		kFullscreenEnter,	///< window entered fullscreen state
		kFullscreenLeave	///< window left fullscreen state
	};

	IWindow& window;

	WindowEvent (IWindow& window,
				 EventType type = kClose)
	: GUIEvent (kWindowEvent, type, 0),
	  window (window)
	{}
};

//************************************************************************************************
// IWindowBase
/** Interface for window-like high-level views (popup windows, frames in a workspace). 
	\ingroup gui_view */
//************************************************************************************************

interface IWindowBase: IUnknown
{
	/** Activate window. */
	virtual void CCL_API activate () = 0;

	/** Check if window is currently active. */
	virtual tbool CCL_API isActive () const = 0;

	DECLARE_IID (IWindowBase)
};

DEFINE_IID (IWindowBase, 0x572e83ba, 0xed5b, 0x46b2, 0xac, 0x65, 0xf2, 0xac, 0x43, 0xfc, 0xa0, 0x0)

//************************************************************************************************
// IWindow
/** Window interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IWindow: IWindowBase
{	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Window attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Get system-specific window reference
		Windows: HWND
		macOS: NSView
		iOS: UIViewController
		Linux: WindowContext (defined in ilinuxspecifics.h)
		Android: not used */
	virtual void* CCL_API getSystemWindow () const = 0;
	
	/** Check if system-specific window reference is valid. */
	virtual tbool CCL_API isSystemWindowValid () const = 0;
	
	/** Get window title. */
	virtual StringRef CCL_API getWindowTitle () const = 0;

	/** Set window title. */
	virtual void CCL_API setWindowTitle (StringRef title) = 0;

	/** Set if window graphic updates should be collected, returns old state. */
	virtual tbool CCL_API setCollectGraphicUpdates (tbool state) = 0;
	
	/** Set if window size updates should be collected, returns old state. */
	virtual tbool CCL_API setCollectSizeUpdates (tbool state) = 0;

	/** Get the points to pixels scaling factor. */
	virtual float CCL_API getContentScaleFactor () const = 0;
		
	/** Set the points to pixels scaling factor. This works for child windows only on Windows platform. */
	virtual tbool CCL_API setContentScaleFactor (float factor) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Window actions
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Show window. */
	virtual void CCL_API show () = 0;
	
	/** Hide window. */
	virtual void CCL_API hide () = 0;
	
	/** Set the window to maximized state (true) or restore original size (false). */
	virtual void CCL_API maximize (tbool state) = 0;

	/** Returns true if window is maximized. */
	virtual tbool CCL_API isMaximized () const = 0;

	/** Returns true if window is minimized. */
	virtual tbool CCL_API isMinimized () const = 0;

	/** Set the size, that is used in restored state */
	virtual void CCL_API setUserSize (RectRef size) = 0;

	/** Get the size, that is used in restored state */
	virtual void CCL_API getUserSize (Rect& size) const = 0;
 
	/** Check if window is currently visible. */
	virtual tbool CCL_API isVisible () const = 0;

	/** Force to redraw window client area. */
	virtual void CCL_API redraw () = 0;

	/** Center window on main screen. */
	virtual void CCL_API center () = 0;

	/** Try to close window. */
	virtual tbool CCL_API close () = 0;

	/** Show context menu at given position in window coords, or at focusView if wasKeyPressed. */
	virtual void CCL_API popupContextMenu (PointRef where, tbool wasKeyPressed = false) = 0;

	/** Move the window to the given screen coordinates. */
	virtual void CCL_API moveWindow (PointRef pos) = 0;
	
	/** Returns true if window is in fullscreen. */
	virtual tbool CCL_API isFullscreen () const = 0;

	/** Enter or leave full screen for this window. Returns previous state. */
	virtual tbool CCL_API setFullscreen (tbool state) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Event handler
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Register event handler. */
	virtual void CCL_API addHandler (IWindowEventHandler* handler) = 0;
	
	/** Remove event handler. */
	virtual void CCL_API removeHandler (IWindowEventHandler* handler) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Controller
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get controller object associated with this window. */
	virtual IUnknown* CCL_API getController () const = 0;

	/** Set controller object for this window. */
	virtual tbool CCL_API setController (IUnknown* controller) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Other
	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Get view currently focused in window. */
	virtual IView* CCL_API getFocusIView () const = 0;

	/** Get outer size of system window including non-client area. */
	virtual void CCL_API getFrameSize (Rect& size) const = 0;
	
	struct UpdateCollector;
	struct SizeChangeCollector;

	DECLARE_IID (IWindow)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Additional properties (IObject) and notifications
	//////////////////////////////////////////////////////////////////////////////////////////////

	DEFINE_ENUM (StatusBarStyle)
	{
		kLightContent,
		kDarkContent
	};

	DECLARE_STRINGID_MEMBER (kRepresentedFile) 			///< property: [IUrl] file represented by window (macOS only)
	DECLARE_STRINGID_MEMBER (kDocumentDirty) 			///< property: [bool] at least one document represented in this window is dirty
	DECLARE_STRINGID_MEMBER (kSystemView) 	   			///< property: [UIntPtr] pointer to system NSView object (macOS only)
	DECLARE_STRINGID_MEMBER (kStatusBarStyle)			///< property: [StatusBarStyle] defines whether the status bar is optimized for light or dark content

	DECLARE_STRINGID_MEMBER (kFirstResponderChanged)	///< message: (macOS only) the first responder of the NSWindow has changed; args[0]: new first responder (NSView*)
	DECLARE_STRINGID_MEMBER (kFocusViewChanged)			///< message: the focus view has changed. This message is deferred to only be fired once if focus is killed and then immediately set to a new focus view
	DECLARE_STRINGID_MEMBER (kSystemWindowChanged)		///< message: system window has been created, destroyed, or the internal state of the system window has changed
};

DEFINE_IID (IWindow, 0x431d0e7b, 0xe2c1, 0x4ad9, 0xbd, 0xfa, 0x3a, 0xd, 0x42, 0x4b, 0x67, 0xd0)
DEFINE_STRINGID_MEMBER (IWindow, kRepresentedFile, "representedFile")
DEFINE_STRINGID_MEMBER (IWindow, kDocumentDirty, "documentDirty")
DEFINE_STRINGID_MEMBER (IWindow, kSystemView, "systemView")
DEFINE_STRINGID_MEMBER (IWindow, kStatusBarStyle, "statusBarStyle")
DEFINE_STRINGID_MEMBER (IWindow, kFirstResponderChanged, "firstResponderChanged")
DEFINE_STRINGID_MEMBER (IWindow, kFocusViewChanged, "focusViewChanged")
DEFINE_STRINGID_MEMBER (IWindow, kSystemWindowChanged, "systemWindowChanged")

//************************************************************************************************
// IWindow::UpdateCollector
/** Helper class to collect window updates.
	\ingroup gui_view */
//************************************************************************************************

struct IWindow::UpdateCollector
{
	IWindow* window;
	tbool oldState;

	UpdateCollector (IWindow* window, bool state = true)
	: window (window),
		oldState (window ? window->setCollectGraphicUpdates (state) : false)
	{}

	~UpdateCollector ()
	{ if(window) window->setCollectGraphicUpdates (oldState); }
};

//********************************************************************************************
// IWindow::SizeChangeCollector
/** Helper class to collect size changes.
	\ingroup gui_view */
//********************************************************************************************

struct IWindow::SizeChangeCollector
{
	IWindow* window;
	tbool oldState;

	SizeChangeCollector (IWindow* window, bool state = true)
	: window (window),
	  oldState (window ? window->setCollectSizeUpdates (state) : false)
	{}

	~SizeChangeCollector ()
	{ if(window) window->setCollectSizeUpdates (oldState); }
};

//************************************************************************************************
// IWindowEventHandler
/** Handler for window events. 
	\ingroup gui_view */
//************************************************************************************************

interface IWindowEventHandler: IUnknown
{
	/** Handle window event. */
	virtual tbool CCL_API onWindowEvent (WindowEvent& windowEvent) = 0;

	DECLARE_IID (IWindowEventHandler)
};

DEFINE_IID (IWindowEventHandler, 0x4ca3917, 0x2a84, 0x4882, 0x8d, 0xd2, 0x67, 0x37, 0xe2, 0xa9, 0xb2, 0xcb)

//************************************************************************************************
// IDesktop
/** Desktop interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IDesktop: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Windows
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get main application window. */
	virtual IWindow* CCL_API getApplicationWindow () = 0;

	/** Get main application menu bar. */
	virtual IMenuBar* CCL_API getApplicationMenuBar () = 0;

	/** Get current window to be used as parent for modal dialogs. */
	virtual IWindow* CCL_API getDialogParentWindow () = 0;

	/** Count windows. */
	virtual int CCL_API countWindows () const = 0;

	/** Get window by z-index. The topmost window is last. */
	virtual IWindow* CCL_API getWindow (int index) const = 0;
	
	enum FindWindowFlags
	{
		kEnforceOcclusionCheck = 1 << 0	///< force check for occluding windows (off by default on macOS)
	};

	/** Get window at given screen position. */
	virtual IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0) = 0;
	
	/** Get window under cursor. */
	virtual IWindow* CCL_API findWindowUnderCursor (int flags = 0) = 0;

	/** Get window for given controller. */
	virtual IWindow* CCL_API getWindowByOwner (IUnknown* controller) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** If there is a popup selector on top, close it, defer the command and return true. */
	virtual tbool CCL_API closePopupAndDeferCommand (ICommandHandler* handler, const CommandMsg& cmd) = 0;

	/** Try to close the topmost modal dialog with the given dialogResult (see DialogResult::ResultCodes). */
	virtual tbool CCL_API closeTopModal (int dialogResult) = 0;

	/** Try to close all modal dialog windows. */
	virtual tbool CCL_API closeModalWindows () = 0;

	/** Flush updates and handle events for any open progress windows. */
	virtual void CCL_API flushUpdatesWithProgressWindows (IView* caller = nullptr) = 0;

	enum ModeFlags 
	{
		kProgressMode   = 1 << 0,  ///< is any progress window open
		kMenuLoopMode   = 1 << 1,  ///< is any window is in a menu loop
		kModalMode      = 1 << 2,  ///< is any dialog open
		kTextInputMode  = 1 << 3,  ///< is any text input being performed
		kPopupMode      = 1 << 4,  ///< is popup window active
		kAnyMode		= 0xffffffff
	};
	
	/** Returns true if any of the passed mode flags are apply. */
	virtual tbool CCL_API isInMode (int modeFlags) const = 0;
	
	/** Redraw all windows. */
	virtual void CCL_API redrawAll () = 0;
	
	struct UpdateCollector;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Monitors
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Returns number of active display monitors. */
	virtual int CCL_API countMonitors () const = 0;
	
	/** Returns the main monitor index. */
	virtual int CCL_API getMainMonitor () const = 0;
	
	/** Find monitor at given position.*/
	virtual int CCL_API findMonitor (PointRef where, tbool defaultToPrimary) const = 0;

	/** Returns the size of the given monitor. */
	virtual tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const = 0;	

	/** Get the points to pixels scaling factor of given monitor. */
	virtual float CCL_API getMonitorScaleFactor (int index) const = 0;

	DECLARE_IID (IDesktop)
};

DEFINE_IID (IDesktop, 0x20cb720d, 0x87b, 0x41eb, 0x99, 0x24, 0xdd, 0x8b, 0x55, 0x88, 0xee, 0x80)

//************************************************************************************************
// IDesktop::UpdateCollector
//************************************************************************************************

struct IDesktop::UpdateCollector
{
	Vector<IWindow*> toReset;

	UpdateCollector (IDesktop& desktop)
	: toReset (desktop.countWindows ())
	{		
		for(int i = 0; i < desktop.countWindows (); i++)
		{
			IWindow* w = desktop.getWindow (i);
			if(w && w->setCollectGraphicUpdates (true) == false)
				toReset.add (w);			
		}
	}
	
	~UpdateCollector ()
	{
		for(int i = 0; i < toReset.count (); i++)
			toReset.at (i)->setCollectGraphicUpdates (false);
	}
};

} // namespace CCL

#endif // _ccl_iwindow_h
