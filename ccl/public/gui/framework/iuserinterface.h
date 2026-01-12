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
// Filename    : ccl/public/gui/framework/iuserinterface.h
// Description : UI Management Interface
//
//************************************************************************************************

#ifndef _ccl_iuserinterface_h
#define _ccl_iuserinterface_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/gui/framework/keycodes.h"

#include "ccl/meta/generated/cpp/gui-constants-generated.h"

namespace CCL {

struct KeyState;
struct GUIEvent;

interface ITimer;
interface ITimerTask;
interface IUIEventHandler;
interface IWindow;
interface IView;
interface IApplication;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Styles
{
	/** GUI Orientations. */
	DEFINE_ENUM (OrientationType)
	{
		kAnyOrientation = kOrientationTypeAnyOrientation,
		kLandscape = kOrientationTypeLandscape,
		kPortrait = kOrientationTypePortrait
	};

	/** Restrictions on GUI Orientations. */
	DEFINE_ENUM (OrientationTypeFlags)
	{
		kLandscapeAllowed = kOrientationFlagLandscapeAllowed,
		kPortraitAllowed  = kOrientationFlagPortraitAllowed
	};

	/** Platform Style IDs. */
	DEFINE_ENUM (PlatformStyleID)
	{
		kButtonOrder,
		kRoundedWindowCorners,
		kCustomMenuBar
	};

	/** Platform button order. */
	DEFINE_ENUM (ButtonOrder)
	{
		kAffirmativeButtonLeft,
		kAffirmativeButtonRight
	};
}

using CCL::Styles::OrientationType;
using CCL::Styles::OrientationTypeFlags;
using CCL::Styles::PlatformStyleID;

//************************************************************************************************
// IApplicationProvider
/** Application provider interface.
	\ingroup gui */
//************************************************************************************************

interface IApplicationProvider: IUnknown
{
	/** Called before main event loop starts. */
	virtual tbool CCL_API onInit () = 0;

	/** Called when process quits via exit(), i.e. control doesn't return to main function. */
	virtual void CCL_API onExit () = 0;

	/** Access application object. */
	virtual IApplication* CCL_API getApplication () = 0;

	DECLARE_IID (IApplicationProvider)
};

DEFINE_IID (IApplicationProvider, 0x1d79eea2, 0xa266, 0x47d2, 0xa9, 0x80, 0xf4, 0x5, 0x98, 0x3b, 0x9e, 0xc6)

//************************************************************************************************
// IUserInterface
/** UI management interface - Access singleton instance via System::GetGUI().
	\ingroup gui */
//************************************************************************************************

interface IUserInterface: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Startup/Shutdown/Activation
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Startup GUI (allocates system resources, etc.).
		Applications have to pass their module reference, plug-ins and services sharing CCL
		with the host application have to pass null. */
	virtual tbool CCL_API startup (ModuleRef module = nullptr, IApplicationProvider* appProvider = nullptr) = 0;

	/** Shutdown GUI. */
	virtual void CCL_API shutdown () = 0;

	/** Application types. */
	DEFINE_ENUM (ApplicationType)
	{
		kDesktopApplication,	///< desktop application
		kMobileApplication		///< mobile application
	};

	/** Get application type (desktop or mobile), can be defined in cclgui.config. */
	virtual ApplicationType CCL_API getApplicationType () const = 0;

	/** Return application activation state. */
	virtual tbool CCL_API isApplicationActive () const = 0;

	/** Give control to existing instance of same application or bring application into foreground. */
	virtual tbool CCL_API activateApplication (tbool startupMode, ArgsRef args) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Event Handling
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Run event loop, returns ExitCode. */
	virtual int CCL_API runEventLoop () = 0;

	/** Handle graphical updates, timers, etc. with optional wait. */
	virtual tbool CCL_API flushUpdates (tbool wait = true) = 0;

	/** Handle mouse events for given window. */
	virtual tbool CCL_API flushWindowEvents (IWindow* window) = 0;

	/**	Close all windows and quit event loop.
		If event loop is not yet running, this call sets the exit code. */
	virtual void CCL_API quit (int exitCode = 0) = 0;

	/** Check if event loop is about to quit. */
	virtual tbool CCL_API isQuitting (int* exitCode = nullptr) const = 0;

	/** Add global event handler. */
	virtual void CCL_API addHandler (IUIEventHandler* handler) = 0;

	/** Remove global event handler. */
	virtual void CCL_API removeHandler (IUIEventHandler* handler) = 0;

	/** Return time of the last event processed (in seconds). */
	virtual double CCL_API getLastEventTime () = 0;

	/** Simulate event on system level. */
	virtual tresult CCL_API simulateEvent (const GUIEvent& event) = 0;

	struct InputStats
	{
		int mouseCount = 0;			///< number of mouse down events
		int touchCount = 0;			///< number of touch interactions
		int penCount = 0;			///< number of pen interactions
		int dropCount = 0;			///< number of successful drag & drop operations
		int contextMenuCount = 0;	///< number of context menu events
		int keyCommandCount = 0;	///< number of executed key commands

		InputStats& operator -= (const InputStats& other)
		{
			mouseCount -= other.mouseCount;
			touchCount -= other.touchCount;
			penCount -= other.penCount;
			dropCount -= other.dropCount;
			contextMenuCount -= other.contextMenuCount;
			keyCommandCount -= other.keyCommandCount;
			return *this;
		}
	};

	/** Get user input statistics (events since application start). */
	virtual void CCL_API getInputStats (InputStats& stats) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Keyboard and Mouse
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get current keystate. */
	virtual void CCL_API getKeyState (KeyState& keys) const = 0;

	/** Get most recent keystate as retrieved from event. */
	virtual const KeyState& CCL_API getLastKeyState () const = 0;

	/** Detect current state of a key asynchronously. Returns kResultTrue, kResultFalse or an error code. */
	virtual tresult CCL_API detectKeyPressed (VirtualKey vkey, uchar character) const = 0;

	/** Get mouse position in screen coordinates. */
	virtual Point& CCL_API getMousePosition (Point& pos) const = 0;

	/** Set mouse position in screen coordinates. */
	virtual void CCL_API setMousePosition (const Point& pos) = 0;

	/** Get maximum time between the clicks of a double click in seconds. */
	virtual double CCL_API getDoubleClickDelay () const = 0;

	/** Establish / remove the wait cursor. */
	virtual void CCL_API setWaitCursor (tbool state) = 0;

	/** Check if the wait cursor is active. */
	virtual tbool CCL_API isWaitCursor () const = 0;

	/** Get time (in seconds) of the last user activity (mouse moves, commands, etc.). */
	virtual double CCL_API getLastUserActivity () const = 0;

	/** Tell the system that user input happened other than mouse or keyboard. */
	virtual void CCL_API updateUserActivity (int flags = 0) = 0;

	/** Check if a drag & drop session is currently active. */
	virtual tbool CCL_API isDragActive () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Timer
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create timer with given period in milliseconds. */
	virtual ITimer* CCL_API createTimer (unsigned int period) const = 0;

	/** Add task to idle timer. */
	virtual void CCL_API addIdleTask (ITimerTask* task) = 0;

	/** Remove task from idle timer. */
	virtual void CCL_API removeIdleTask (ITimerTask* task) = 0;

	/** Application activity mode. */
	enum class ActivityMode: int32
	{
		kNormal,		///< normal application mode (default)
		kBackground,	///< application wants to finish background tasks before being suspended
		kAlwaysOn		///< prevents the application from automatic suspension (no system sleep)
	};
	
	/** Type of activity .  */
	enum class ActivityType: int32
	{
		kBasic,	    ///< static / basic application activity mode
		kNetwork,	///< activity with network transfer
		kOther	    ///< other temporary activity
	};

	/** Set application activity mode. Implementation depends on underlying operating system.
		The optional timeout (only supported for ActivityType::kBasic) schedules an automatic reset to the previous mode. */
	virtual tresult CCL_API setActivityMode (ActivityMode mode, ActivityType type = ActivityType::kBasic, int64 timeout = 0) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Tooltip
	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Hide the global tooltip popup. */
	virtual void CCL_API hideTooltip () = 0;

	/** Show or update the tooltip for this view now. */
	virtual void CCL_API retriggerTooltip (IView* view) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Device Orientation
	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Current GUI orientation, landscape or portrait. */
	virtual	OrientationType CCL_API getInterfaceOrientation () const = 0;

	/** Set current GUI orientation. */
	virtual void CCL_API setInterfaceOrientation (OrientationType orientation) = 0;

	/** Check if a GUI orientation is allowed. */
	virtual tbool CCL_API isAllowedInterfaceOrientation (OrientationType orientation) const = 0;

	/** Contrain GUI orientation to landscape or portrait only. */
	virtual void CCL_API setAllowedInterfaceOrientations (OrientationTypeFlags orientations) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Styles
	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Get platform style property. */
	virtual tbool CCL_API getPlatformStyle (Variant& value, PlatformStyleID styleId) const = 0;

	DECLARE_IID (IUserInterface)

	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Shortcut to check if (virtual) key is currently pressed. */
	inline bool isKeyPressed (VirtualKey vkey) const { return detectKeyPressed (vkey, 0) == kResultTrue; }
};

DEFINE_IID (IUserInterface, 0xc63c93df, 0x6c59, 0x4ff5, 0xb2, 0xd1, 0xd9, 0x41, 0xaa, 0x24, 0xbd, 0x97)

//************************************************************************************************
// IUIEventHandler
/** Global GUI event handler interface.
	\ingroup gui_event */
//************************************************************************************************

interface IUIEventHandler: CCL::IUnknown
{
	/** Handle global user interface event. */
	virtual tbool CCL_API handleEvent (IWindow* window, const GUIEvent& event) = 0;

	DECLARE_IID (IUIEventHandler)
};

DEFINE_IID (IUIEventHandler, 0xa10d0290, 0x7632, 0x4ae6, 0x99, 0x31, 0x9e, 0x2, 0xf4, 0x43, 0xd4, 0x9f)

//************************************************************************************************
// WaitCursor
/** Helper for setting the wait cursor.
	\ingroup gui */
//************************************************************************************************

class WaitCursor
{
public:
	WaitCursor (IUserInterface& gui, bool state = true)
	: gui (gui),
	  oldState (gui.isWaitCursor ())
	{
		gui.setWaitCursor (state);
	}

	~WaitCursor ()
	{
		gui.setWaitCursor (oldState);
	}

private:
	tbool oldState;
	IUserInterface& gui;
};

} // namespace CCL

#endif // _ccl_iuserinterface_h
