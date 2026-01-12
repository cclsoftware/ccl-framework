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
// Filename    : ccl/platform/linux/wayland/waylandcompositor.cpp
// Description : Nested Wayland Compositor
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/waylandcompositor.h"
#include "ccl/platform/linux/wayland/waylandclient.h"
#include "ccl/platform/linux/wayland/dmabufferhelper.h"
#include "ccl/platform/linux/wayland/monitorhelper.h"
#include "ccl/platform/linux/wayland/waylandchildwindow.h"

#include "ccl/public/system/userthread.h"

#include "wayland-server-delegate/iwaylandserver.h"

#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

namespace CCL {
namespace Linux {

//************************************************************************************************
// WaylandServerRunLoop
//************************************************************************************************

class WaylandServerRunLoop: public Unknown,
					 		public Threading::UserThread
{
public:
	WaylandServerRunLoop (int serverFd, int displayFd, wl_event_queue* queue, Threading::CriticalSection& lock);
	~WaylandServerRunLoop ();

	void cancel ();

private:
	Threading::CriticalSection& lock;
	int cancelFd[2];
	int serverFd;
	int displayFd;
	wl_event_queue* queue;

	// UserThread
	int threadEntry ();
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// WaylandCompositor
//************************************************************************************************

DEFINE_SINGLETON (WaylandCompositor)

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandCompositor::WaylandCompositor ()
: serverRunLoop (nullptr),
  serverEventQueue (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandCompositor::~WaylandCompositor ()
{
	ASSERT (serverRunLoop == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_display* CCL_API WaylandCompositor::openWaylandConnection ()
{
	WaylandServerDelegate::IWaylandServer& server = WaylandServerDelegate::IWaylandServer::instance ();
	if(!server.isStarted ())
	{
		clientContext.initialize ();

		wl_display* display = WaylandClient::instance ().getDisplay ();
		int displayFd = wl_display_get_fd (display);

		ASSERT (serverEventQueue == nullptr)
		serverEventQueue = wl_display_create_queue (display);
		int serverFd = server.startup (&clientContext, serverEventQueue);
		if(serverFd != -1 && displayFd != -1)
		{
			serverRunLoop = NEW WaylandServerRunLoop (serverFd, displayFd, serverEventQueue, serverLock);
			serverRunLoop->startThread (Threading::kPriorityNormal);
		}
	}
	Threading::ScopedLock guard (serverLock);
	return server.openClientConnection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WaylandCompositor::closeWaylandConnection (wl_display* display)
{
	if(display == nullptr)
		return kResultInvalidArgument;

	WaylandServerDelegate::IWaylandServer& server = WaylandServerDelegate::IWaylandServer::instance ();
	tresult result = kResultFailed;
	{
		Threading::ScopedLock guard (serverLock);
		if(server.closeClientConnection (display))
			result = kResultOk;
	}
	if(server.countActiveClients () == 0)
	{
		serverRunLoop->cancel ();
		serverRunLoop->stopThread (500);
		safe_release (serverRunLoop);
		server.shutdown ();
		if(serverEventQueue)
			wl_event_queue_destroy (serverEventQueue);
		serverEventQueue = nullptr;
		clientContext.terminate ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_proxy* CCL_API WaylandCompositor::createProxy (wl_display* display, wl_proxy* object, WaylandServerDelegate::WaylandResource* implementation)
{
	WaylandServerDelegate::IWaylandServer& server = WaylandServerDelegate::IWaylandServer::instance ();
	Threading::ScopedLock guard (serverLock);
	return server.createProxy (display, object, implementation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WaylandCompositor::destroyProxy (wl_proxy* proxy)
{
	WaylandServerDelegate::IWaylandServer& server = WaylandServerDelegate::IWaylandServer::instance ();
	Threading::ScopedLock guard (serverLock);
	return server.destroyProxy (proxy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandCompositor::registerChildWindow (WaylandChildWindow* childWindow)
{
	childWindows.addOnce (childWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandCompositor::unregisterChildWindow (WaylandChildWindow* childWindow)
{
	childWindows.remove (childWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandCompositor::getSubSurfaceOffset (Point& offset, wl_display* display, wl_surface* parentSurface, wl_surface* childSurface) const
{
	for(WaylandChildWindow* childWindow : childWindows)
	{
		if(childWindow->getDisplay () != display)
			continue;
		if(childWindow->getParentOffset (offset, parentSurface))
			return true;
	}
	return false;
}

//************************************************************************************************
// WaylandServerRunLoop
//************************************************************************************************

WaylandServerRunLoop::WaylandServerRunLoop (int serverFd, int displayFd, wl_event_queue* queue, Threading::CriticalSection& lock)
: UserThread ("ServerRunLoop"),
  cancelFd {-1, -1},
  serverFd (serverFd),
  displayFd (displayFd),
  queue (queue),
  lock (lock)
{
	::pipe2 (cancelFd, O_NONBLOCK);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandServerRunLoop::~WaylandServerRunLoop ()
{
	if(cancelFd[0] >= 0)
		::close (cancelFd[0]);
	if(cancelFd[1] >= 0)
		::close (cancelFd[1]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandServerRunLoop::cancel ()
{
	requestTerminate ();

	char buffer = 1;
	::write (cancelFd[1], &buffer, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WaylandServerRunLoop::threadEntry ()
{
	wl_display* display = WaylandClient::instance ().getDisplay ();
	bool receivedClientEvents = false;

	while(true)
	{
		// flush server events (server -> clients)
		{
			Threading::ScopedLock guard (lock);
			CCL_PRINTLN ("WaylandServerRunLoop: Flushing server events (server -> clients)")
			WaylandServerDelegate::IWaylandServer::instance ().flush ();
		}

		// flush display (server -> session compositor)
		if(receivedClientEvents)
		{
			CCL_PRINTLN ("WaylandServerRunLoop: Flushing display (server -> session compositor, roundtrip)")
			wl_display_roundtrip_queue (display, queue);
		}
		else
		{
			CCL_PRINTLN ("WaylandServerRunLoop: Flushing display (server -> session compositor)")
			wl_display_flush (display);
		}
		
		receivedClientEvents = false;
		if(wl_display_prepare_read_queue (display, queue) == 0)
		{
			pollfd fds[] =
			{
				{ serverFd, POLLIN, 0 },
				{ displayFd, POLLIN, 0 },
				{ cancelFd[0], POLLIN, 0 }
			};

			CCL_PRINTLN ("WaylandServerRunLoop: Polling fds")
			::poll (fds, ARRAY_COUNT (fds), -1);

			// dispatch incoming server events (clients -> server)
			if((fds[0].revents & POLLIN) > 0)
			{
				CCL_PRINTLN ("WaylandServerRunLoop: Dispatching incoming server events (clients -> server)")
				Threading::ScopedLock guard (lock);
				WaylandServerDelegate::IWaylandServer::instance ().dispatch ();
		
				receivedClientEvents = true;
			}

			// dispatch server-side Wayland objects (session compositor -> server)
			if((fds[1].revents & POLLIN) > 0)
			{
				CCL_PRINTLN ("WaylandServerRunLoop: Dispatching server-side Wayland objects (session compositor -> server)")
				if(wl_display_read_events (display) < 0)
				{
					CCL_WARN ("%s: %s\n", "Failed to read server Wayland events", ::strerror (errno))
					break;
				}
			}
			else
			{
				wl_display_cancel_read (display);
			}
		}
		else
		{
			CCL_PRINTLN ("WaylandServerRunLoop: Dispatching pending server-side Wayland objects (session compositor -> server)")
			if(wl_display_dispatch_queue_pending (display, queue) < 0)
			{
				CCL_WARN ("%s: %s\n", "Failed to dispatch pending Wayland server events to display", ::strerror (errno))
				break;
			}
		}

		if(shouldTerminate ())
			break;
	}

	return 0;
}

//************************************************************************************************
// WaylandClientContext
//************************************************************************************************

void WaylandClientContext::initialize ()
{
	WaylandClient::instance ().registerEventHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClientContext::terminate ()
{
	WaylandClient::instance ().unregisterEventHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClientContext::addListener (WaylandServerDelegate::IContextListener* listener)
{
	return listeners.add (listener);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClientContext::removeListener (WaylandServerDelegate::IContextListener* listener)
{
	return listeners.remove (listener);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClientContext::getSubSurfaceOffset (int32_t& x, int32_t& y, wl_display* display, wl_surface* parentSurface, wl_surface* childSurface)
{
	Point offset;
	if(WaylandCompositor::instance ().getSubSurfaceOffset (offset, display, parentSurface, childSurface))
	{
		x = wl_fixed_from_int (offset.x);
		y = wl_fixed_from_int (offset.y);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_compositor* WaylandClientContext::getCompositor () const
{
	return WaylandClient::instance ().getCompositor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_subcompositor* WaylandClientContext::getSubCompositor () const
{
	return WaylandClient::instance ().getSubCompositor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_shm* WaylandClientContext::getSharedMemory () const
{
	return WaylandClient::instance ().getSharedMemory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_seat* WaylandClientContext::getSeat () const
{
	return WaylandClient::instance ().getSeat ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

xdg_wm_base* WaylandClientContext::getWindowManager () const
{
	return WaylandClient::instance ().getWindowManager ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

zwp_linux_dmabuf_v1* WaylandClientContext::getDmaBuffer () const
{
	return WaylandClient::instance ().getDmaBuffer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t WaylandClientContext::getSeatCapabilities () const
{
	return WaylandClient::instance ().getSeatCapabilities ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* WaylandClientContext::getSeatName () const
{
	return WaylandClient::instance ().getSeatName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WaylandClientContext::countOutputs () const
{
	return MonitorHelper::instance ().countOutputs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WaylandServerDelegate::WaylandOutput& WaylandClientContext::getOutput (int index) const
{
	return MonitorHelper::instance ().getOutput (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WaylandClientContext::countDmaBufferModifiers () const
{
	return DmaBufferHelper::instance ().countModifiers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClientContext::getDmaBufferModifier (uint32_t& format, uint32_t& modifierHigh, uint32_t& modifierLow, int index) const
{
	return DmaBufferHelper::instance ().getModifier (format, modifierHigh, modifierLow, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WaylandClientContext::handleEvent (SystemEvent& event)
{
	switch(event.eventType)
	{
	case SystemEvent::kSeatCapabilitiesChanged :
		signalChange (WaylandServerDelegate::IContextListener::kSeatCapabilitiesChanged);
		break;
	case SystemEvent::kOutputsChanged :
		signalChange (WaylandServerDelegate::IContextListener::kOutputsChanged);
		break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClientContext::signalChange (WaylandServerDelegate::IContextListener::ChangeType changeType)
{
	for(WaylandServerDelegate::IContextListener* listener : listeners)
		listener->contextChanged (changeType);
}
