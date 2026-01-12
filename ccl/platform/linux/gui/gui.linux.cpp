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
// Filename    : ccl/platform/linux/gui/gui.linux.cpp
// Description : Platform-specific GUI implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/inputhandler.h"
#include "ccl/platform/linux/wayland/datadevicehelper.h"
#include "ccl/platform/linux/wayland/waylandcompositor.h"
#include "ccl/platform/linux/gui/exceptionhandler.h"
#include "ccl/platform/linux/gui/dbusapplication.h"
#include "ccl/platform/linux/gui/window.linux.h"
#include "ccl/platform/linux/interfaces/ilinuxsystem.h"

#include "ccl/platform/shared/host/platformidletask.h"
#include "ccl/platform/shared/host/platformthemepainter.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/system/mousecursor.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <poll.h>

namespace CCL {

//************************************************************************************************
// LinuxUserInterface
//************************************************************************************************

class LinuxUserInterface: public UserInterface,
						  public Linux::IWaylandClient,
						  public Linux::IEventLoop,
						  public Linux::IEventHandler
{
public:
	LinuxUserInterface ();
	~LinuxUserInterface ();

	// UserInterface
	bool startupPlatform (ModuleRef module) override;
	void shutdownPlatform () override;
	void quitPlatform () override;
	tbool CCL_API activateApplication (tbool startupMode, ArgsRef args) override;

	int CCL_API runEventLoop () override;
	tbool CCL_API flushUpdates (tbool wait = true) override;
	tbool CCL_API flushWindowEvents (IWindow* window) override;

	void CCL_API getKeyState (KeyState& keys) const override;
	tresult CCL_API detectKeyPressed (VirtualKey vkey, uchar character) const override;

	bool detectDrag (View* view, const Point& where) override;
	bool detectDoubleClick (View* view, const Point& where) override;
	void tryDoubleClick () override;
	void resetCursor () override;

	void updateNativeUserActivity () override;
	tresult CCL_API setActivityMode (ActivityMode mode, ActivityType type = ActivityType::kBasic, int64 timeout = 0) override;

	void CCL_API setMousePosition (const Point& pos) override;
	void processMouseMove (bool fromTimer) override;
	tresult CCL_API simulateEvent (const GUIEvent& event) override;
	ITimer* CCL_API createTimer (unsigned int period) const override;
	
	void runModalLoop (IWindow* window, tbool& loopTerminated) override;

	// IWaylandClient
	wl_display* CCL_API getWaylandDisplay () const override;
	tresult CCL_API setWaylandSocket (Linux::IWaylandSocket* socket) override;
	tresult CCL_API setEventLoop (Linux::IEventLoop* eventLoop) override;

	// IEventLoop
	tresult CCL_API addEventHandler (Linux::IEventHandler* handler, int eventFd) override;
	tresult CCL_API removeEventHandler (Linux::IEventHandler* handler) override;

	// IEventHandler
	void CCL_API onEvent (int eventFd) override;

	CLASS_INTERFACES (UserInterface)

protected:
	struct CustomEventHandler
	{
		SharedPtr<Linux::IEventHandler> handler;
		int fd;

		CustomEventHandler (Linux::IEventHandler* handler = nullptr, int fd = -1)
		: handler (handler),
		  fd (fd)
		{}

		bool operator == (const CustomEventHandler& other) const
		{
			return handler == other.handler && fd == other.fd;
		}
	};
	Vector<CustomEventHandler> customEventHandlers;

	SharedPtr<Linux::IEventLoop> externalEventLoop;

	PlatformIntegration::PlatformIdleTask platformIdleTask;

	bool inDoubleClickDetection;
	int syntheticMouseUpTime;
	
	wl_display* display;
	int displayFd;
	int timerFd;
	bool inDispatch;
	bool shouldQuit;

	AutoPtr<MouseCursor> defaultCursor;
	
	bool nextEvent ();
	tresult dispatchEvents (bool wait);
	void onTimerEvent ();
	bool reconnectCompositor ();
};

//************************************************************************************************
// LinuxTimer
//************************************************************************************************

class LinuxTimer: public SystemTimer
{
public:
	LinuxTimer (unsigned int period);
	~LinuxTimer ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxUserInterface linuxGUI;
UserInterface& GUI = linuxGUI;

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxUserInterface
//************************************************************************************************

LinuxUserInterface::LinuxUserInterface ()
: inDoubleClickDetection (false),
  syntheticMouseUpTime (0),
  display (nullptr),
  displayFd (-1),
  timerFd (-1),
  inDispatch (false),
  shouldQuit (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxUserInterface::~LinuxUserInterface ()
{
	#if EXCEPTION_HANDLER_ENABLED
	LinuxExceptionHandler::instance ().cleanupInstance ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::queryInterface (UIDRef iid, void** ptr)
{
	#if EXCEPTION_HANDLER_ENABLED
	if(iid == ccl_iid<IDiagnosticDataProvider> ())
		return LinuxExceptionHandler::instance ().queryInterface (iid, ptr);
	#endif
	if(iid == ccl_iid<IWaylandSocket> ())
		return WaylandCompositor::instance ().queryInterface (iid, ptr);
	QUERY_INTERFACE (IWaylandClient)
	QUERY_INTERFACE (IEventLoop)
	QUERY_INTERFACE (IEventHandler)
	return UserInterface::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxUserInterface::startupPlatform (ModuleRef moduleRef)
{
	UnknownPtr<ILinuxSystem> linuxSystem (&System::GetSystem ());
	if(linuxSystem.isValid ())
		linuxSystem->setDBusSupport (&DBusSupport::instance ());

	#if EXCEPTION_HANDLER_ENABLED
	// install exception handler for main application
	if(moduleRef)
		LinuxExceptionHandler::instance ().install ();
	#endif

	bool succeeded = WaylandClient::instance ().startup ();
	InputHandler::instance ().initialize ();
	DataDeviceHelper::instance ().initialize ();
	
	display = WaylandClient::instance ().getDisplay ();
	if(display)
		displayFd = wl_display_get_fd (display);
	
	ASSERT (display != nullptr && displayFd != -1)
	
	timerFd = ::timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
	itimerspec timerSpec {};
	timerSpec.it_interval.tv_nsec = 10 * 1000 * 1000;
	timerSpec.it_value.tv_nsec = timerSpec.it_interval.tv_nsec;
	::timerfd_settime (timerFd, 0, &timerSpec, nullptr);
	
	if(externalEventLoop.isValid ())
	{
		externalEventLoop->addEventHandler (this, displayFd);
		externalEventLoop->addEventHandler (this, timerFd);
	}

	Bitmap::setResolutionNamingMode (Bitmap::kMultiResolution);
	
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::shutdownPlatform ()
{
	defaultCursor.release ();
	
	if(externalEventLoop.isValid ())
	{
		externalEventLoop->removeEventHandler (this);
		externalEventLoop->removeEventHandler (this);
	}
	::close (timerFd);

	DataDeviceHelper::instance ().terminate ();
	InputHandler::instance ().terminate ();
	WaylandClient::instance ().shutdown ();	

	#if EXCEPTION_HANDLER_ENABLED
	LinuxExceptionHandler::instance ().uninstall ();
	#endif

	UnknownPtr<ILinuxSystem> linuxSystem (&System::GetSystem ());
	if(linuxSystem.isValid ())
		linuxSystem->setDBusSupport (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxUserInterface::runEventLoop ()
{
	platformIdleTask.initialize (WaylandClient::instance ().getApplicationId ());
	static_cast<PlatformIntegration::PlatformThemePainter&> (NativeThemePainter::instance ()).initialize ();
	
	if(!finishStartup ())
		return kExitError;

	{
		DBusApplication dbusApplication (DBusSupport::instance (), WaylandClient::instance ().getApplicationId ().str ());

		if(appProvider)
			if(!appProvider->onInit ())
				return exitCode;

		onAppStateChanged (IApplication::kUIInitialized);

		if(quitDone) // quit has been requested during startup
		{
			// see also: UserInterface::quit()
			Desktop.closeAll ();
			quitPlatform ();
		}

		ScopedVar<bool> scope (eventLoopRunning, true);
		while(nextEvent ());
	}

	platformIdleTask.terminate ();
	
	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUserInterface::flushUpdates (tbool wait)
{
	InputHandler::instance ().flushEvents ();
	if(!inDispatch)
	{
		tresult result = dispatchEvents (wait);
		if(result == kResultFailed)
			return reconnectCompositor ();
		return result == kResultOk;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUserInterface::flushWindowEvents (IWindow* _window)
{
	if(!inDispatch)
	{
		InputHandler::instance ().flushEvents ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxUserInterface::dispatchEvents (bool wait)
{
	if(display == nullptr)
		return kResultInvalidArgument;

	SOFT_ASSERT (inDispatch == false, "LinuxUserInterface::dispatchEvents called while still dispatching Wayland events.")
			
	if(display == nullptr)
		return kResultInvalidArgument;

	if(wl_display_flush (display) < 0 && errno != EAGAIN)
	{
		CCL_WARN ("%s: %s\n", "Failed to flush Wayland display", ::strerror (errno))
	}
	
	if(wl_display_prepare_read (display) == 0)
	{
		Vector<pollfd> fds (2 + customEventHandlers.count ());
		fds.add ({ displayFd, POLLIN, 0 });
		fds.add ({ timerFd, POLLIN, 0 });

		for(const CustomEventHandler& item : customEventHandlers)
			fds.add ({ item.fd, POLLIN, 0 });
		
		int result = ::poll (fds, fds.count (), wait ? -1 : 0);
		
		if((fds[0].revents & POLLIN) > 0 && display != nullptr)
		{
			ScopedVar<bool> scope (inDispatch, true);
			if(wl_display_read_events (display) < 0)
			{
				CCL_WARN ("%s: %s\n", "Failed to read Wayland events", ::strerror (errno))
				return kResultFailed;
			}
		}
		else if((fds[0].revents & POLLHUP) || (fds[0].revents & POLLERR))
		{
			CCL_WARN ("%s: %s %s\n", (fds[0].revents & POLLHUP) ? "POLLHUP" : "POLLERR", "Lost connection to Wayland compositor!", ::strerror (errno))
			return kResultFailed;
		}
		else 
		{
			wl_display_cancel_read (display);
		}
		
		if((fds[1].revents & POLLIN) > 0)
		{
			onTimerEvent ();
		}
		
		for(const pollfd& pollFd : fds)
		{
			if((pollFd.revents & POLLIN) <= 0)
				continue;
			
			for(const CustomEventHandler& item : customEventHandlers)
			{
				if(item.fd == pollFd.fd && item.handler != nullptr)
				{
					item.handler->onEvent (item.fd);
					break;
				}
			}
		}

		if(shouldQuit)
			return kResultAborted;
		
		if(result == -1)
			return kResultFailed;

		return kResultOk;
	}
	else
	{
		ScopedVar<bool> scope (inDispatch, true);
		if(wl_display_dispatch_pending (display) < 0)
		{
			CCL_WARN ("%s: %s\n", "Failed to dispatch pending Wayland events to display", ::strerror (errno))
			return kResultFailed;
		}
		
		return kResultOk;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxUserInterface::nextEvent ()
{
	InputHandler::instance ().flushEvents ();
	tresult result = dispatchEvents (true);
	if(result == kResultFailed)
		return reconnectCompositor ();
	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::onTimerEvent ()
{
	if(!isTimerBlocked ())
		SystemTimer::serviceTimers ();
	
	int64 numExpirations = 0;
	while(::read (timerFd, &numExpirations, 8) > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxUserInterface::onEvent (int eventFd)
{
	InputHandler::instance ().flushEvents ();
	if(eventFd == displayFd)
	{
		{
			ScopedVar<bool> scope (inDispatch, true);
			if(wl_display_prepare_read (display) == 0)
			{
				if(wl_display_read_events (display) < 0)
				{
					CCL_WARN ("%s: %s\n", "Client failed to read Wayland events", ::strerror (errno))
					return;
				}
			}
			if(wl_display_dispatch_pending (display) < 0)
			{
				CCL_WARN ("%s: %s\n", "Client failed to dispatch pending Wayland events to display", ::strerror (errno))
				return;
			}
		}

		if(wl_display_flush (display) < 0 && errno != EAGAIN)
		{
			CCL_WARN ("%s: %s\n", "Client failed to flush Wayland display", ::strerror (errno))
		}
	}
	else if(eventFd == timerFd)
	{
		onTimerEvent ();
		if(wl_display_flush (display) < 0 && errno != EAGAIN)
		{
			CCL_WARN ("%s: %s\n", "Client failed to flush Wayland display", ::strerror (errno))
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxUserInterface::reconnectCompositor ()
{
	CCL_WARN ("%s\n", "Lost connection to compositor. Trying to reconnect.")
	shutdownPlatform ();
	return startupPlatform (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::setWaylandSocket (IWaylandSocket* socket)
{
	ASSERT (!eventLoopRunning)
	if(eventLoopRunning)
		return kResultFailed;

	if(WaylandClient::instance ().getSocket () == socket)
		return kResultOk;

	bool wasInitialized = display != nullptr;
	if(wasInitialized)
		shutdownPlatform ();
	WaylandClient::instance ().setSocket (socket);
	if(wasInitialized)
		startupPlatform (nullptr);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::setEventLoop (Linux::IEventLoop* eventLoop)
{
	ASSERT (!eventLoopRunning)
	if(eventLoopRunning)
		return kResultFailed;

	if(externalEventLoop == eventLoop)
		return kResultOk;

	ASSERT (customEventHandlers.isEmpty ())

	bool wasInitialized = display != nullptr;
	if(wasInitialized)
		shutdownPlatform ();
	externalEventLoop = eventLoop;
	if(wasInitialized)
		startupPlatform (nullptr);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_display* CCL_API LinuxUserInterface::getWaylandDisplay () const
{
	return display;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::quitPlatform ()
{
	ASSERT (System::IsInMainThread ())
	shouldQuit = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool LinuxUserInterface::activateApplication (tbool startupMode, ArgsRef args)
{	
	if(startupMode)
	{
		WaylandClient& waylandClient = WaylandClient::instance ();
		DBusApplicationClient client (DBusSupport::instance (), waylandClient.getApplicationId ().str ());
		if(!client.sendCommandLine (args))
			return false;
	
		CStringPtr activationToken = ::getenv ("XDG_ACTIVATION_TOKEN");
		CCL_PRINTF ("activationToken from environment variable: %s\n", activationToken ? activationToken : "<none>")
		if(activationToken)
			client.activate (activationToken);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::detectKeyPressed (VirtualKey vkey, uchar character) const
{
	if(vkey != VKey::kUnknown)
	{
		return InputHandler::instance ().isKeyPressed (vkey) ? kResultTrue : kResultFalse;
	}
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxUserInterface::getKeyState (KeyState& keys) const
{
	InputHandler::instance ().getActiveModifierKeys (keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxUserInterface::detectDrag (View* view, const Point& where)
{
	Window* window = view ? view->getWindow () : nullptr;
	if(window == nullptr)
		return false;
	
	if(Desktop.findWindowUnderCursor () != window)
		return false;
	
	LinuxWindow* linuxWindow = LinuxWindow::cast (window);
	linuxWindow->inputEventsSuspended (true);
	
	Point where2 (where);
	view->clientToScreen (where2);
	
	static int kDragRange = 3;
	Rect dragRect (where2.x - kDragRange, where2.y - kDragRange, where2.x + kDragRange, where2.y + kDragRange);
	
	bool detected = false;
	while(nextEvent ())
	{
		if(InputHandler::instance ().isMouseButtonDown () == false)
			break;
		
		if(Desktop.findWindowUnderCursor () != window)
		{
			detected = true;
			break;
		}
		
		Point mousePosition;
		getMousePosition (mousePosition);
		if(!dragRect.pointInside (mousePosition))
		{
			detected = true;
			break;
		}
	}
	linuxWindow->inputEventsSuspended (false);
	
	return detected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxUserInterface::detectDoubleClick (View* view, const Point& where)
{
	if(doubleClicked > kDoubleClickReset)
		return doubleClicked == kDoubleClickTrue;
	
	doubleClicked = kDoubleClickFalse;
	
	Window* window = view ? view->getWindow () : nullptr;
	if(window == nullptr)
		return false;
	
	ObservedPtr<LinuxWindow> linuxWindow = LinuxWindow::cast (window);
	linuxWindow->inputEventsSuspended (true);
	
	Point where2 (where);
	view->clientToScreen (where2);
	
	Rect clickRect (where2.x - 2, where2.y - 2, where2.x + 2, where2.y + 2);
	double now = System::GetProfileTime ();
	bool wasButtonDown = InputHandler::instance ().isMouseButtonDown ();
	while(nextEvent ())
	{
		if(view->getWindow () == nullptr)
			break;
		
		if(Desktop.findWindowUnderCursor () != window)
			break;
		
		Point mousePosition;
		getMousePosition (mousePosition);
		if(!clickRect.pointInside (mousePosition))
			break;
		
		if(System::GetProfileTime () - now > 0.25)
			break;
		
		if(!wasButtonDown && InputHandler::instance ().isMouseButtonDown ())
		{
			doubleClicked = kDoubleClickTrue;
			break;
		}
		wasButtonDown = InputHandler::instance ().isMouseButtonDown ();
	}
	
	if(linuxWindow.isValid ())
	{
		if(doubleClicked == kDoubleClickTrue)
			linuxWindow->discardSuspendedEvents ();
		linuxWindow->inputEventsSuspended (false);
	}
	
	return doubleClicked == kDoubleClickTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::tryDoubleClick ()
{
	LinuxWindow* window = LinuxWindow::cast (unknown_cast<Window> (Desktop.findWindowUnderCursor ()));
	if(window == nullptr)
		return;
	
	bool wasButtonDown = InputHandler::instance ().isMouseButtonDown ();
	double now = System::GetProfileTime ();
	while(nextEvent ())
	{
		if(Desktop.findWindowUnderCursor () != static_cast<IWindow*> (window))
			break;
		
		if(System::GetProfileTime () - now > 0.25)
			break;
		
		if(!wasButtonDown && InputHandler::instance ().isMouseButtonDown ())
		{
			doubleClicked = kDoubleClickPending;
			break;
		}
		wasButtonDown = InputHandler::instance ().isMouseButtonDown ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::resetCursor ()
{
	if(!defaultCursor.isValid ())
		defaultCursor = MouseCursor::createCursor (ThemeElements::kArrowCursor);
	safe_release (cursor);
	if(defaultCursor)
		defaultCursor->makeCurrent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::updateNativeUserActivity ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::setActivityMode (ActivityMode mode, ActivityType type, int64 timeout)
{
	// nothing here for Linux applications
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxUserInterface::setMousePosition (const Point& pos)
{
	lastMousePos = pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::processMouseMove (bool fromTimer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::simulateEvent (const GUIEvent& event)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API LinuxUserInterface::createTimer (unsigned int period) const
{
	return NEW LinuxTimer (period);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxUserInterface::runModalLoop (IWindow* window, tbool& loopTerminated)
{
	while(!loopTerminated && nextEvent ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::addEventHandler (IEventHandler* handler, int eventFd)
{
	ASSERT (!externalEventLoop.isValid ())
	customEventHandlers.add (CustomEventHandler (handler, eventFd));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUserInterface::removeEventHandler (IEventHandler* handler)
{
	for(const CustomEventHandler& item : customEventHandlers)
	{
		if(item.handler == handler)
		{
			customEventHandlers.remove (item);
			return kResultOk;
		}
	}
	return kResultFailed;
}

//************************************************************************************************
// LinuxTimer
//************************************************************************************************

LinuxTimer::LinuxTimer (unsigned int period)
: SystemTimer (period)
{
	systemTimer = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxTimer::~LinuxTimer ()
{}
