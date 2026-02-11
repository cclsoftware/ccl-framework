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
// Filename    : ccl/gui/gui.h
// Description : User Interface Management
//
//************************************************************************************************

#ifndef _ccl_gui_h
#define _ccl_gui_h

#include "ccl/base/object.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

class View;
class Window;
class MouseCursor;
interface IWindow;
interface ITooltipPopup;

//************************************************************************************************
// UserInterface
/** User Interface management class. */
//************************************************************************************************

class UserInterface: public Object,
					 public IUserInterface,
					 public ITimerTask
{
public:
	DECLARE_CLASS_ABSTRACT (UserInterface, Object)

	UserInterface ();
	~UserInterface ();

	bool isStarted () const;
	bool isStartedByMainModule () const;
	tbool CCL_API startup (ModuleRef module, IApplicationProvider* appProvider = nullptr) override;
	void CCL_API shutdown () override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Event Handling
	//////////////////////////////////////////////////////////////////////////////////////////////

	int CCL_API runEventLoop () override;
	tbool CCL_API flushUpdates (tbool wait = true) override;
	void CCL_API quit (int exitCode = 0) override;
	tbool CCL_API isQuitting (int* exitCode = nullptr) const override;
	void onExit ();

	virtual void runModalLoop (IWindow* window, tbool& loopTerminated);
	tbool CCL_API flushWindowEvents (IWindow* window) override;

	void CCL_API addHandler (IUIEventHandler* handler) override;
	void CCL_API removeHandler (IUIEventHandler* handler) override;
	bool tryGlobal (Window* window, const GUIEvent& event);

	IApplication* getApplication () const;
	ApplicationType CCL_API getApplicationType () const override;
	tbool CCL_API isApplicationActive () const override;
	tbool CCL_API activateApplication (tbool startupMode, ArgsRef args) override;
	void onAppStateChanged (StringID notificationId);

	IApplicationProvider* getApplicationProvider () const;
	tresult CCL_API simulateEvent (const GUIEvent& event) override;
	void CCL_API getInputStats (InputStats& stats) const override;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Device Orientation
	//////////////////////////////////////////////////////////////////////////////////////////////////

	OrientationType CCL_API getInterfaceOrientation () const override;
	void CCL_API setInterfaceOrientation (OrientationType orientation) override;
	tbool CCL_API isAllowedInterfaceOrientation (OrientationType orientation) const override;
	void CCL_API setAllowedInterfaceOrientations (OrientationTypeFlags orientations) override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Platform Styles
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_BOOL (roundedWindowCornersSupported, RoundedWindowCornersSupported)
	PROPERTY_BOOL (customMenuBarSupported, CustomMenuBarSupported)
	tbool CCL_API getPlatformStyle (Variant& value, PlatformStyleID styleId) const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Keyboard
	//////////////////////////////////////////////////////////////////////////////////////////////

	void CCL_API getKeyState (KeyState& keys) const override;
	const KeyState& CCL_API getLastKeyState () const override;
	void setLastKeyState (const KeyState& keys);
	tresult CCL_API detectKeyPressed (VirtualKey vkey, uchar character) const override;
	bool translateKey (const KeyEvent& key, Window* window = nullptr) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Mouse
	//////////////////////////////////////////////////////////////////////////////////////////////

	Point& CCL_API getMousePosition (Point& pos) const override;
	void CCL_API setMousePosition (const Point& pos) override;

	virtual bool detectDrag (View* view, const Point& where);
	virtual bool detectDoubleClick (View* view, const Point& where);
	virtual void resetDoubleClick ();
	virtual void tryDoubleClick ();

	double CCL_API getDoubleClickDelay () const override;

	MouseCursor* getCursor () const;
	void setCursor (MouseCursor* cursor, View* view = nullptr);
	void updateCursor ();
	virtual void resetCursor ();
	void CCL_API setWaitCursor (tbool state) override;
	tbool CCL_API isWaitCursor () const override;

	double CCL_API getLastUserActivity () const override;
	void CCL_API updateUserActivity (int flags = 0) override;
	tbool CCL_API isDragActive () const override;

	virtual void processMouseMove (bool fromTimer);
    void setLastMousePos (const Point& pos);

	void trackUserInput (const GUIEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Views
	//////////////////////////////////////////////////////////////////////////////////////////////

	void onActivateWindow (Window* window, bool state);
	bool onMouseMove (Window* window, const MouseEvent& event, bool force = false);
	bool onDragEvent (Window* window, const DragEvent& event);
	void viewDestroyed (View* view);
	View* getMouseView () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Tootips
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_VARIABLE (int, tooltipDelay, TooltipDelay)
	void CCL_API hideTooltip () override;
	void CCL_API retriggerTooltip (IView* view) override;

	class TooltipSuspender;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Timers
	//////////////////////////////////////////////////////////////////////////////////////////////

	double CCL_API getLastEventTime () override;
	void setLastEventTime (double t);

	ITimer* CCL_API createTimer (unsigned int period) const override;
	void CCL_API addIdleTask (ITimerTask* task) override;
	void CCL_API removeIdleTask (ITimerTask* task) override;
	void CCL_API onTimer (ITimer* = nullptr) override; ///< [ITimerTask]

	ITimer& getIdleTimer ();
	int64 getCurrentIdleTime () const;
	void blockTimer (bool state);
	bool isTimerBlocked () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Activity mode
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	tresult CCL_API setActivityMode (ActivityMode mode, ActivityType type = ActivityType::kBasic, int64 timeout = 0) override;
	virtual void realizeActivityMode (ActivityMode mode); // to implement in subclass
	virtual void onNetworkActivity (bool state);  // to implement in subclass 

	class TimerBlocker;

	static const String kAffirmativeButtonLeftID;
	static const String kAffirmativeButtonRightID;

	CLASS_INTERFACE2 (ITimerTask, IUserInterface, Object)

protected:
	enum Constants
	{
		kIdlePeriod   =  10,	///< idle time period
		kUpdateDelay  =  10,	///< delay between event updates
		kTooltipDelay = 400		///< delay before tooltip is displayed
	};

	bool startedByMainModule;
	int startupCount;
	ITimer* timer;
	bool quitDone;
	int exitCode;
	bool eventLoopRunning;
	unsigned int lastUpdateTime;
	bool timerBlocked;
	bool commandsBlocked;
	bool appActive;
	ActivityMode basicActivityMode;
	int tempActivityCounter;
	int networkActivityCounter;
	int64 activityModeResetTime;
	ActivityMode activityModeToReset;

	double lastEventTime;
	Point lastMousePos;
	KeyState lastKeys;
	int64 lastMouseMoveTime;
	int64 lastUserActivity;
	int64 currentIdleTime;
	mutable const KeyEvent* currentKeyEvent;
	enum { kDoubleClickReset = -1, kDoubleClickFalse, kDoubleClickTrue, kDoubleClickPending } doubleClicked;
	InputStats inputStats;

	MouseCursor* cursor;
	tbool waitCursorMode;
	OrientationType interfaceOrientation;
	OrientationTypeFlags allowedInterfaceOrientations;

	View *mouseView, *cursorView, *dragView, *tooltipView;
	ITooltipPopup* tooltipPopup;
	Point lastTooltipPos;
	bool tooltipWasHidden;
	bool tooltipSuspended;

	CCL::LinkedList<IUIEventHandler*> handlers;
	IApplicationProvider* appProvider;
	ApplicationType applicationType;
	Styles::ButtonOrder buttonOrder;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	virtual bool finishStartup ();
	virtual bool startupPlatform (ModuleRef module);
	virtual void shutdownPlatform ();
	virtual void quitPlatform ();

	virtual void updateNativeUserActivity ();

	virtual void allowedOrientationsChanged () {}

	bool handleMouseEnter (Window* window, const MouseEvent& event);
	bool handleTooltip (Window* window, const MouseEvent& event);
	void showTooltip ();
	bool isTooltipAllowed (Window* window) const;
	friend class TooltipSuspender;
};

extern UserInterface& GUI;	///< global GUI instance

//************************************************************************************************
// UserInterface::TimerBlocker
//************************************************************************************************

class UserInterface::TimerBlocker
{
public:
	TimerBlocker ();
	~TimerBlocker ();

private:
	bool wasBlocked;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline View* UserInterface::getMouseView () const { return mouseView; }
inline void UserInterface::setLastMousePos (const Point& pos) { lastMousePos = pos; }
inline OrientationType CCL_API UserInterface::getInterfaceOrientation () const { return interfaceOrientation; }
inline IApplication* UserInterface::getApplication () const { return appProvider ? appProvider->getApplication () : nullptr; }
inline IApplicationProvider* UserInterface::getApplicationProvider () const { return appProvider; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_gui_h
