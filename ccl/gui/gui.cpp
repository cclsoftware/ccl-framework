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
// Filename    : ccl/gui/gui.cpp
// Description : User Interface Management
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/keyevent.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/gui/system/accessibility.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/tooltip.h"
#include "ccl/gui/controls/autoscroller.h"
#include "ccl/gui/theme/colorscheme.h"

#include "ccl/base/collections/stringdictionary.h"
#include "ccl/base/storage/filefilter.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (IsFrameworkHostProcess) ()
{
	return GUI.isStartedByMainModule ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IUserInterface& CCL_API System::CCL_ISOLATED (GetGUI) ()
{
	return GUI;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICommandTable& CCL_API System::CCL_ISOLATED (GetCommandTable) ()
{
	return CommandTable::instance ();
}

//************************************************************************************************
// UserInterface::TooltipSuspender
//************************************************************************************************

UserInterface::TooltipSuspender::TooltipSuspender ()
: wasSuspended (GUI.tooltipSuspended)
{
	GUI.tooltipSuspended = true;
	GUI.hideTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterface::TooltipSuspender::~TooltipSuspender ()
{
	GUI.tooltipSuspended = wasSuspended;
}

//************************************************************************************************
// UserInterface::TimerBlocker
//************************************************************************************************

UserInterface::TimerBlocker::TimerBlocker ()
: wasBlocked (GUI.isTimerBlocked ())
{
	GUI.blockTimer (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterface::TimerBlocker::~TimerBlocker ()
{
	GUI.blockTimer (wasBlocked);
}

//************************************************************************************************
// UserInterface
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UserInterface, Object)
static const String kDesktopAppID ("desktopapp");
static const String kMobileAppID ("mobileapp");
const String UserInterface::kAffirmativeButtonLeftID ("affirmative-button-left");
const String UserInterface::kAffirmativeButtonRightID ("affirmative-button-right");
static const String kRoundedWindowCornersID ("rounded-window-corners");
static const String kCustomMenuBarID ("custom-menu-bar");

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterface::UserInterface ()
: startedByMainModule (false),
  startupCount (0),
  timer (nullptr),
  appProvider (nullptr),
  applicationType (kDesktopApplication),
  buttonOrder (Styles::kAffirmativeButtonLeft),
  roundedWindowCornersSupported (false),
  customMenuBarSupported (false),
  quitDone (false),
  exitCode (kExitSuccess),
  eventLoopRunning (false),
  lastUpdateTime (0),
  timerBlocked (false),
  commandsBlocked (false),
  appActive (true),
  lastMouseMoveTime (0),
  lastEventTime (),
  lastUserActivity (0),
  currentIdleTime (0),
  currentKeyEvent (nullptr),
  doubleClicked (kDoubleClickReset),
  mouseView (nullptr),
  cursorView (nullptr),
  dragView (nullptr),
  tooltipDelay (kTooltipDelay),
  tooltipView (nullptr),
  tooltipPopup (nullptr),
  tooltipWasHidden (false),
  tooltipSuspended (false),
  cursor (nullptr),
  interfaceOrientation (Styles::kLandscape),
  allowedInterfaceOrientations (Styles::kLandscapeAllowed | Styles::kPortraitAllowed),
  waitCursorMode (false),
  basicActivityMode (ActivityMode::kNormal),
  tempActivityCounter (0),
  networkActivityCounter (0),
  activityModeResetTime (0),
  activityModeToReset (ActivityMode::kNormal)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterface::~UserInterface ()
{
	cancelSignals ();
	ASSERT (tooltipPopup == nullptr)
	ASSERT (handlers.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::isStarted () const
{
	return startupCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::isStartedByMainModule () const
{
	return startedByMainModule;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterface::ApplicationType UserInterface::getApplicationType () const
{
	return applicationType; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::startup (ModuleRef module, IApplicationProvider* appProvider)
{
	if(startupCount >= 1)
	{
		startupCount++;
		return true;
	}

	NativeGraphicsEngine& graphicsEngine = NativeGraphicsEngine::instance ();

	// Avoid potential modal alerts for command-line apps.
	bool suppressErrors = appProvider == nullptr;
	graphicsEngine.setSuppressErrors (suppressErrors);

	if(!graphicsEngine.startup ())
		return false;

	if(!startupPlatform (module))
		return false;

	// define application type for skin and file filter conditions
	String appTypeString ((applicationType == kMobileApplication) ? kMobileAppID : kDesktopAppID);

	// cclgui.config can override:
	if(Configuration::Registry::instance ().getValue (appTypeString, "GUI", "ApplicationType"))
	{
		if(appTypeString == kMobileAppID)
			applicationType = kMobileApplication;
		else if(appTypeString == kDesktopAppID)
			applicationType = kDesktopApplication;
	}

	Configuration::Registry::instance ().appendValue ("XML.Parsers", "definitions", appTypeString);
	FileFilter::getGlobalConditions ().setEntry ("GUI.apptype", appTypeString);

	// definitions for platform styles
	Configuration::Registry::instance ().appendValue ("XML.Parsers", "definitions", buttonOrder == Styles::kAffirmativeButtonLeft ? kAffirmativeButtonLeftID : kAffirmativeButtonRightID);

	if(roundedWindowCornersSupported)
		Configuration::Registry::instance ().appendValue ("XML.Parsers", "definitions", kRoundedWindowCornersID);

	if(customMenuBarSupported)
		Configuration::Registry::instance ().appendValue ("XML.Parsers", "definitions", kCustomMenuBarID);

	// init appearance mode (optional)
	String appearanceString;
	if(Configuration::Registry::instance ().getValue (appearanceString, "GUI", "AppearanceMode"))
		ColorSchemes::instance ().setMainAppearanceModeFromString (appearanceString);

	addIdleTask (this);
	startupCount++;

	if(module && appProvider)
		this->appProvider = appProvider;

	startedByMainModule = module == System::GetMainModuleRef ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::shutdown ()
{
	if(startupCount <= 0) // startup() failed or not called
		return;

	if(--startupCount > 0)
		return;

	if(timer)
	{
		timer->removeTask (this);
		timer->release ();
		timer = nullptr;
	}

	hideTooltip ();
	setCursor (nullptr);

	if(AccessibilityManager::isEnabled ())
		AccessibilityManager::instance ().shutdown ();

	shutdownPlatform ();

	NativeGraphicsEngine::instance ().shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::finishStartup ()
{
	if(IApplication* application = getApplication ())
		FileFilter::getGlobalConditions ().setEntry (FileFilter::kAppIdentityKey, String (application->getApplicationID ()));

	{
		#if CCL_PLATFORM_DESKTOP
		ErrorContextGuard errorContext;
		#endif
		if(!NativeGraphicsEngine::instance ().verifyFeatureSupport ())
		{
			#if CCL_PLATFORM_DESKTOP
			Alert::errorWithContext (nullptr);
			#endif
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::isQuitting (int* exitCode) const
{ 
	if(exitCode)
		*exitCode = this->exitCode;
	return quitDone; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::quit (int exitCode)
{
	if(quitDone)
		return;

	if(!System::IsInMainThread ())
	{
		(NEW Message ("quit", exitCode))->post (this);
		return;
	}

	quitDone = true;
	this->exitCode = exitCode;

	if(eventLoopRunning)
	{
		// close all Windows
		Desktop.closeAll ();

		quitPlatform ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::onExit ()
{
	if(appProvider)
		appProvider->onExit ();

	shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API UserInterface::getLastEventTime ()
{
	return lastEventTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::setLastEventTime (double t)
{
	lastEventTime = t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::blockTimer (bool state)
{
	timerBlocked = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::isTimerBlocked () const
{
	return timerBlocked;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API UserInterface::createTimer (unsigned int period) const
{
	ASSERT (startupCount > 0)
	return NEW SystemTimer (period);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer& UserInterface::getIdleTimer ()
{
	if(!timer)
		timer = createTimer (kIdlePeriod);
	return *timer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 UserInterface::getCurrentIdleTime () const
{
	return currentIdleTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::addIdleTask (ITimerTask* task)
{
	getIdleTimer ().addTask (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::removeIdleTask (ITimerTask* task)
{
	if(timer) // was: getIdleTimer () => do not recreate timer on early exit!
		timer->removeTask (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::onTimer (ITimer*)
{
	if(isTimerBlocked ())
		return;

	int64 now = System::GetSystemTicks ();
	currentIdleTime = now;
	
	System::GetSignalHandler ().flush ();
	
	processMouseMove (true);

	// check if a tooltip is waiting to be hidden or displayed...
	if(tooltipView)
	{
		if(tooltipPopup)
		{
			int64 timeToHide = tooltipPopup->getTimeToHide ();
			if(timeToHide && now >= timeToHide)
				hideTooltip ();
		}
		else
		{
			if((now - lastMouseMoveTime >= tooltipDelay) && isTooltipAllowed (tooltipView->getWindow ()))
				showTooltip ();
		}
	}

	if(activityModeResetTime > 0 && now >= activityModeResetTime)
	{
		CCL_PRINTF ("timer: reset activityMode: %d\n", activityModeToReset)
		activityModeResetTime = 0;
		setActivityMode (activityModeToReset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* UserInterface::getCursor () const
{
	return cursor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::setCursor (MouseCursor* _cursor, View* view)
{
	if(_cursor == cursor && view == cursorView)
		return;

	// avoid cursor flicker during drag&drop
	if(DragSession::isInternalDragActive ())
		return;

	take_shared<MouseCursor> (cursor, _cursor);
	cursorView = view;
	
	if(cursor)
		cursor->makeCurrent ();
	else
		resetCursor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::updateCursor ()
{
	if(cursor)
		cursor->makeCurrent ();
	else
		resetCursor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::resetCursor ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::setWaitCursor (tbool state)
{
	if(waitCursorMode != state)
	{
		waitCursorMode = state;
		if(state)
		{
			static AutoPtr<MouseCursor> waitCursor (MouseCursor::createCursor (ThemeElements::kWaitCursor));
			setCursor (waitCursor);
		}
		else
			setCursor (nullptr); // reset to default cursor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::isWaitCursor () const 
{ 
	return waitCursorMode; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::translateKey (const KeyEvent& key, Window* window) const
{
	if(currentKeyEvent && currentKeyEvent->isSimilar (key))
		return false; // break recursion

	ScopedVar<const KeyEvent*> scope (currentKeyEvent, &key);

	if(commandsBlocked)
	{
		// defer the key command
		AutoPtr<Boxed::KeyEvent> e (NEW Boxed::KeyEvent (key));
		(NEW Message ("translateKey", Variant (e->asUnknown (), true)))->post (const_cast<UserInterface*> (this));
		return true;
	}

	if(window && window->getMouseHandler())
		return false;


	// Note: Moved focus and active window handling to CommandTable!
	if(CommandTable::instance ().translateKey (key))
	{
		ccl_const_cast (this)->inputStats.keyCommandCount++;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "quit")
	{
		int exitCode = msg[0].asInt ();
		quit (exitCode);
	}
	else if(msg == "translateKey")
	{
		if(Boxed::KeyEvent* key = unknown_cast<Boxed::KeyEvent> (msg[0]))
			translateKey (*key, nullptr);
	}
	else if(msg == "appStateChanged")
	{
		MutableCString notificationId (msg[0].asString ());
		onAppStateChanged (notificationId);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::isApplicationActive () const
{
	return appActive;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::activateApplication (tbool startupMode, ArgsRef args)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::onAppStateChanged (StringID notificationId)
{
	if(!System::IsInMainThread ())
	{
		(NEW Message ("appStateChanged", String (notificationId)))->post (this);
		return;
	}

	if(notificationId == IApplication::kAppActivated || notificationId == IApplication::kAppDeactivated)
	{
		bool state = notificationId == IApplication::kAppActivated;
		if(appActive != state)
		{
			appActive = state;
			CCL_PRINTF ("App %s\n", state ? "activated" : "deactivated")
			
			if(state == false)
				hideTooltip ();

			Desktop.onAppActivate (state);
		}
	}

	Message msg (notificationId);
	if(UnknownPtr<IObserver> app = getApplication ())
		app->notify (nullptr, msg);
	signal (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::setInterfaceOrientation (OrientationType orientation)
{
	if(orientation != interfaceOrientation)
	{
		interfaceOrientation = orientation;
		SignalSource (Signals::kGUI).signal (Message (Signals::kOrientationChanged, orientation));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::isAllowedInterfaceOrientation (OrientationType orientation) const
{
	return (tbool)((1 << orientation) & allowedInterfaceOrientations);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::setAllowedInterfaceOrientations (OrientationTypeFlags orientations)
{
	if(allowedInterfaceOrientations != orientations)
	{
		allowedInterfaceOrientations = orientations;
		allowedOrientationsChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::getPlatformStyle (Variant& value, PlatformStyleID styleId) const
{
	switch(styleId)
	{
	case Styles::kButtonOrder :
		value = buttonOrder;
		return true;

	case Styles::kRoundedWindowCorners :
		value = roundedWindowCornersSupported;
		return true;

	case Styles::kCustomMenuBar :
		value = customMenuBarSupported;
		return true;

	default :
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::getInputStats (InputStats& stats) const
{
	stats = inputStats;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::trackUserInput (const GUIEvent& event)
{
	if(event.eventTime != 0.)
		setLastEventTime (event.eventTime);

	switch(event.eventClass)
	{
	case GUIEvent::kMouseEvent:
		if(event.eventType == MouseEvent::kMouseDown)
			inputStats.mouseCount++;
		break;

	case GUIEvent::kTouchEvent:
		if(event.eventType == TouchEvent::kBegin)
		{
			switch(event.as<TouchEvent> ()->inputDevice)
			{
			case PointerEvent::kPenInput:
				inputStats.penCount++;
				break;

			case PointerEvent::kMouseInput:
				inputStats.mouseCount++;
				break;

			case PointerEvent::kTouchInput:
			default:
				inputStats.touchCount++;
				break;
			}
		}
		break;

	case GUIEvent::kDragEvent:
		if(event.eventType == DragEvent::kDrop)
			inputStats.dropCount++;
		break;

	case GUIEvent::kContextMenuEvent:
		inputStats.contextMenuCount++;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::updateUserActivity (int flags)
{
	int64 now = System::GetSystemTicks ();
	lastUserActivity = now;

	static int64 lastNativeTime = 0;
	if(now - lastNativeTime >= 10000) // 10 seconds
	{
		updateNativeUserActivity ();
		lastNativeTime = now;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API UserInterface::getLastUserActivity () const
{
	// return in seconds for consistency with getLastEventTime()
	return (double)lastUserActivity / 1000.; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::isDragActive () const
{
	return	DragSession::isInternalDragActive () || // internal drag session
			dragView != nullptr; // drag session started by other process
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::addHandler (IUIEventHandler* handler)
{
	handlers.append (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::removeHandler (IUIEventHandler* handler)
{
	handlers.remove (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::tryGlobal (Window* window, const GUIEvent& event)
{
	ListForEach (handlers, IUIEventHandler*, handler)
		if(handler->handleEvent (window, event))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const KeyState& CCL_API UserInterface::getLastKeyState () const
{
	return lastKeys;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::setLastKeyState (const KeyState& keys)
{
	lastKeys = keys;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::onActivateWindow (Window* window, bool state)
{
	if(!state)
	{
		if(mouseView && mouseView->getWindow () == window)
		{
			mouseView->onMouseLeave (MouseEvent (MouseEvent::kMouseLeave));
			mouseView = nullptr;

			setCursor (nullptr);
		}

		if(tooltipPopup && tooltipView && tooltipView->getWindow () == window)
			hideTooltip ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::onMouseMove (Window* window, const MouseEvent& event, bool force)
{
	Point newMousePos (event.where);
	window->clientToScreen (newMousePos);

	#if 1 // suppress mouse move event if not actually moved
	if(!force && newMousePos == lastMousePos)
	{
		#if (0 && DEBUG)
		static int counter = 0;
		Debugger::printf ("Mouse move skipped %d\n", counter++);	
		#endif
		return true;
	}
	#endif

	lastMouseMoveTime = System::GetSystemTicks ();
	lastUserActivity = lastMouseMoveTime;
	lastMousePos = newMousePos;

	if(!window->getMouseHandler () && !isDragActive ())
		handleMouseEnter (window, event);
	handleTooltip (window, event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::handleMouseEnter (Window* window, const MouseEvent& event)
{
	MouseEvent e2 (event);
	e2.eventType = MouseEvent::kMouseEnter;
	View* mouseEnterView = window->enterMouse (e2, mouseView);
	if(mouseView && mouseView != mouseEnterView)
	{
		e2.eventType = MouseEvent::kMouseLeave;
		mouseView->onMouseLeave (e2);
		if(mouseView == cursorView)
			setCursor (nullptr); // reset cursor
	}

	mouseView = mouseEnterView;

	CCL_PRINTF ("mouseView: %s\n", MutableCString (mouseView ? mouseView->getName () : 0).str ());
	return mouseView != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::handleTooltip (Window* window, const MouseEvent& event)
{
	View* newTooltipView = nullptr;
	if(tooltipView)
	{
		MouseEvent e2 (event);
		tooltipView->windowToClient (e2.where);
		if(tooltipView->getWindow () == window && tooltipView->isInsideClient (e2.where))
		{
			newTooltipView = window->findTooltipView (event);
			if(tooltipView == newTooltipView)
				newTooltipView = nullptr;

			if(!newTooltipView)
			{
				if(tooltipPopup && tooltipView->isTooltipTrackingEnabled () && !DragSession::isInternalDragActive ())
				{
					lastTooltipPos = e2.where;
					SharedPtr<ITooltipPopup> lifeGuard (tooltipPopup);
					tooltipView->onTrackTooltip (TooltipEvent (*tooltipPopup, TooltipEvent::kMove, e2.where));
				}
				return true;
			}
		}

		hideTooltip ();
	}

	tooltipView = newTooltipView ? newTooltipView : window->findTooltipView (event);
	tooltipWasHidden = false;
	return tooltipView != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::showTooltip ()
{
	ASSERT (tooltipView != nullptr)
	if(!tooltipView)
		return;

	Point pos (lastMousePos);
	tooltipView->screenToClient (pos);

	bool trackingEnabled = tooltipView->isTooltipTrackingEnabled ();
	if(tooltipWasHidden && (!trackingEnabled || lastTooltipPos == pos))
		return;

	// TODO: trigger global event handler!!!

	if(tooltipPopup)
		tooltipPopup->setText (ComposedTooltip (tooltipView));
	else
		tooltipPopup = TooltipPopup::createTooltipPopup (tooltipView);
#if CCL_PLATFORM_DESKTOP
	ASSERT (tooltipPopup != nullptr)
#endif
	if(!tooltipPopup)
		return;

	tooltipPopup->setDuration (ITooltipPopup::kDefaultDuration);

	if(trackingEnabled)
	{
		lastTooltipPos = pos;
		if(!tooltipView->onTrackTooltip (TooltipEvent (*tooltipPopup, TooltipEvent::kShow, pos)))
			return; // do not show if view return false
	}

	tooltipPopup->show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::hideTooltip ()
{
	if(tooltipView)
	{
		if(tooltipPopup && tooltipView->isTooltipTrackingEnabled ())
		{
			Point pos (lastMousePos);
			tooltipView->screenToClient (pos);
			tooltipView->onTrackTooltip (TooltipEvent (*tooltipPopup, TooltipEvent::kHide, pos));
		}
		tooltipView = nullptr;

		// TODO: trigger global event handler!!!
	}

	if(tooltipPopup)
	{
		tooltipPopup->hide ();
		if(tooltipPopup)
		{
			tooltipPopup->release ();
			tooltipPopup = nullptr;
		}
	}

	tooltipWasHidden = true;
	SignalSource (Signals::kControls).signal (Message (Signals::kHideTooltip));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::retriggerTooltip (IView* _view)
{
	View* view = unknown_cast<View> (_view);

	// force optimized update when tooltip tracking 
	if(tooltipPopup && tooltipView && tooltipView == view && tooltipView->isTooltipTrackingEnabled ())
	{
		SharedPtr<ITooltipPopup> lifeGuard (tooltipPopup);
		tooltipView->onTrackTooltip (TooltipEvent (*tooltipPopup, TooltipEvent::kMove, lastTooltipPos));
		return;
	}
	
	if(tooltipPopup && tooltipView != view)
		hideTooltip ();

	tooltipView = view;
	tooltipWasHidden = false;

	showTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::isTooltipAllowed (Window* window) const
{
	if(!window || tooltipSuspended)
		return false;

	// don't show tooltip when a mousehandler, menu or dragSession is active
	if(window->getMouseHandler () || window->isInMenuLoop () || DragSession::isInternalDragActive ())
		return false;

	// when a modal window exists (includes PopupSelector), don't show tooltips in other windows
	if(IWindow* popup = Desktop.getTopWindow (kPopupLayer))
		if(popup != window)
			return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::onDragEvent (Window* window, const DragEvent& e)
{
	bool result = false;
	DragSession* session = unknown_cast<DragSession> (&e.session);
	ASSERT (session != nullptr)
	if(!session)
		return false;

	GUI.trackUserInput (e);

	auto checkSourceHandler = [&] ()
	{
		// the source drag handler can claim responsiblity of handling the dragging by setting a SourceResult in the session
		IDragHandler* sourceHandler = session->getSourceDragHandler ();
		if(sourceHandler && !UnknownPtr<ISourceDragBlocker> (session->getDragHandler ()).isValid ())
		{
			switch(e.eventType)
			{
			case DragEvent::kDragEnter:
			case DragEvent::kDragOver:
				{
					if(session->isSourceHandlerActive ())
						sourceHandler->dragOver (e);
					else
					{
						sourceHandler->dragEnter (e);
						session->setSourceHandlerActive (true);
					}

					// check source result from dragEnter / dragOver
					if(session->getSourceResult () != IDragSession::kDropNone)
						return true;
				}
				break;

			case DragEvent::kDragLeave:
				if(sourceHandler)
				{
					sourceHandler->dragLeave (e);
					session->setSourceHandlerActive (false);
				}
				break;

			case DragEvent::kDrop:
				if(sourceHandler && session->getSourceResult () != IDragSession::kDropNone)
				{
					DragSession::DropGuard guard (*session);

					session->setDropped (true);
					sourceHandler->drop (e);
					session->deferDrop (sourceHandler, e, window);
				}
				break;
			}
		}
		return false;
	};

	if(checkSourceHandler ())
	{
		if(dragView)
		{
			// no mouse coords here!
			if(dragView != window)
				dragView->onDragLeave (e);
			dragView = nullptr;
			session->leaveDragHandler (e);
		}
		return true;
	}

	// *** Drag Enter/Over ***
	if(e.eventType == DragEvent::kDragEnter || e.eventType == DragEvent::kDragOver)
	{
		View* deeperDragView = nullptr;
		SharedPtr<IDragHandler> oldDragHandler;

		CCL_PRINTF ("UserInterface::onDragEvent %s\n", e.eventType == DragEvent::kDragEnter ? "kDragEnter" : "kDragOver")
		CCL_PRINTF ("Window: %x\n", window);
		CCL_ADD_INDENT (2)

		if(dragView)
		{
			DragEvent e2 (e);
			dragView->windowToClient (e2.where);

			if(dragView->getWindow () == window &&
			   dragView->isInsideClient (e2.where))
			{
				// still dragging over current view, check if a subView is now interested
				oldDragHandler = session->getDragHandler ();

				DragEvent enterEvent (e2);
				enterEvent.eventType = DragEvent::kDragEnter;
				deeperDragView = dragView->dragEnter (enterEvent);
				ASSERT (deeperDragView != dragView)

				if(!deeperDragView)
				{
					if(oldDragHandler)
					{
						session->triggerAutoScroll ();
						if(oldDragHandler->dragOver (e2))
							return true;
					}
					else if((dragView != window) && dragView->onDragOver (e2))
					{
						session->triggerAutoScroll ();
						return true; // still dragging over current view
					}
				}
			}

			// leave old drag view
			e2.eventType = DragEvent::kDragLeave;
			if(dragView != window)
				dragView->onDragLeave (e2);

			if(deeperDragView)
			{
				dragView = deeperDragView;
				if(oldDragHandler)
					oldDragHandler->dragLeave (e2); // session already has the new handler
			}
			else
			{
				session->leaveDragHandler (e2);
				session->setResult (DragSession::kDropNone);
			}
		}

		if(!deeperDragView)
		{
			// find new drag view
			DragEvent enterEvent (e);
			enterEvent.eventType = DragEvent::kDragEnter;

			AutoScroller* autoScroller = session->getAutoScroller ();
			if(autoScroller && autoScroller->isScrolling ())
			{
				// autoscrolling is just happening, find new drag view only inside the scroll container
				if(View* scrollView = autoScroller->getScrollView ())
				{
					DragEvent e2 (enterEvent);
					scrollView->windowToClient (e2.where);
					dragView = scrollView->dragEnter (e2);
				}
			}
			else
			{
				AutoScroller* autoScroller = session->getAutoScroller ();
				View* scrollView = autoScroller && autoScroller->isScrolling () ? autoScroller->getScrollView () : nullptr;
				if(scrollView)
				{
					// autoscrolling is just happening, find new drag view only inside the scroll container
					DragEvent e2 (enterEvent);
					scrollView->windowToClient (e2.where);
					dragView = scrollView->dragEnter (e2);
				}
				else
				{
					// find new drag view
					dragView = window->dragEnter (enterEvent);

					if(!dragView)
					{
						// try window
						AutoPtr<IDragHandler> dragHandler = window->createDragHandler (enterEvent);
						if(dragHandler && dragHandler->dragEnter (enterEvent))
						{
							enterEvent.session.setDragHandler (dragHandler);
							dragView = window;
						}
					}
				}
			}
		}
		result = dragView != nullptr;

		CCL_PRINTF ("%snew dragView: ", CCL_INDENT) LOG_VIEW (dragView, 0, 0)

		if(result)
		{
			// the new dragView becomes the auto scroll target if it has created a non-null-drag handler
			IDragHandler* dragHandler = session->getDragHandler ();
			if(dragHandler && !dragHandler->isNullHandler ())
				session->setAutoScrollTarget (dragView);
		}

		// (drag handler already got a dragEnter call)
	}
	// *** Drag Leave ***
	else if(e.eventType == DragEvent::kDragLeave)
	{
		if(dragView)
		{
			// no mouse coords here!
			if(dragView != window)
				dragView->onDragLeave (e);
			dragView = nullptr;
			session->leaveDragHandler (e);
		}

		session->onDragFinished (e);
	}
	// *** Drop ***
	else if(e.eventType == DragEvent::kDrop)
	{
		trackUserInput (e);

		if(DragSession* sourceSession = DragSession::getActiveSession ())
			sourceSession->setDropped (true);

		{
			DragSession::DropGuard guard (*session);

			if(dragView)
			{
				session->setDropped (true);

				DragEvent e2 (e);
				dragView->windowToClient (e2.where);
				if(dragView != window)
					result = dragView->onDrop (e2);
			
				window->setFocusView (dragView);
			
				if(IDragHandler* dragHandler = session->getDragHandler ())
				{
					dragHandler->drop (e2);
					session->deferDrop (dragHandler, e2, dragView);
					session->setHandler (nullptr);
				}

				if(session->getTotalResult () != IDragSession::kDropNone)
					result = true;
			}
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::viewDestroyed (View* view)
{
	if(view == mouseView)
		mouseView = nullptr;

	if(view == cursorView)
	{
		setCursor (nullptr); // reset cursor
		cursorView = nullptr;
	}

	if(view == dragView)
		dragView = nullptr;

	if(view == tooltipView)
	{
		hideTooltip (); // hide tooltip
		tooltipView = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::startupPlatform (ModuleRef module)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::shutdownPlatform ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::quitPlatform ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UserInterface::runEventLoop ()
{
	finishStartup ();

	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::runModalLoop (IWindow* window, tbool& loopTerminated)
{
	CCL_NOT_IMPL ("UserInterface::runModalLoop")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::flushUpdates (tbool wait)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserInterface::flushWindowEvents (IWindow* _window)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserInterface::detectKeyPressed (VirtualKey vkey, uchar character) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::getKeyState (KeyState& keys) const
{
	keys.keys = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API UserInterface::getMousePosition (Point& pos) const
{
	pos = lastMousePos;
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserInterface::setMousePosition (const Point& _pos)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::detectDrag (View* view, const Point& where)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserInterface::detectDoubleClick (View* view, const Point& where)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::tryDoubleClick ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API UserInterface::getDoubleClickDelay () const
{
	return .25;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::resetDoubleClick ()
{
	if(doubleClicked == kDoubleClickPending)
		doubleClicked = kDoubleClickTrue;
	else
		doubleClicked = kDoubleClickReset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::updateNativeUserActivity ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::realizeActivityMode (ActivityMode mode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::onNetworkActivity (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserInterface::setActivityMode (ActivityMode mode, ActivityType type, int64 timeout)
{
	if(type == ActivityType::kBasic)
	{
		CCL_PRINTF ("setActivityMode: %d\n", mode)
		if(timeout > 0)
		{
			ASSERT (mode != ActivityMode::kNormal)

			// remember mode before first timeout call (more calls might follow to extend the period)
			if(activityModeResetTime == 0)
				activityModeToReset = basicActivityMode;

			activityModeResetTime = System::GetSystemTicks () + timeout;
			CCL_PRINTF ("schedule reset to mode %d after %d ms\n", activityModeToReset, timeout)
		}
		else
			activityModeResetTime = 0; // setting a mode without timeout makes it permanent

		basicActivityMode = mode;

		if(tempActivityCounter == 0 || basicActivityMode != ActivityMode::kNormal)
			realizeActivityMode (basicActivityMode);

		if(mode == ActivityMode::kNormal)
			activityModeResetTime = 0; // discard a scheduled reset
	}
	else
	{		
		bool isNetworkActivity = (type == ActivityType::kNetwork);
		bool inProgress = (mode != ActivityMode::kNormal);		
		int increment = inProgress ? 1 : -1;
		bool wasActivity = tempActivityCounter > 0;	
		tempActivityCounter = ccl_max (tempActivityCounter + increment, 0);
		bool isActivity = tempActivityCounter > 0;	
		
		if(basicActivityMode == ActivityMode::kNormal && wasActivity != isActivity)
			realizeActivityMode (isActivity ? ActivityMode::kBackground : ActivityMode::kNormal);

		if(isNetworkActivity)
		{
			wasActivity = networkActivityCounter > 0;	
			networkActivityCounter = ccl_max (networkActivityCounter + increment, 0);
			isActivity = networkActivityCounter > 0;	

			if(wasActivity != isActivity)
				onNetworkActivity (isActivity);	
		}		
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserInterface::processMouseMove (bool fromTimer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserInterface::simulateEvent (const GUIEvent& event)
{
	return kResultNotImplemented;
}
