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
// Filename    : ccl/platform/linux/wayland/waylandcompositor.h
// Description : Nested Wayland Compositor
//
//************************************************************************************************

#ifndef _ccl_linux_waylandcompositor_h
#define _ccl_linux_waylandcompositor_h

#include "ccl/base/singleton.h"

#include "ccl/gui/system/systemevent.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/framework/ilinuxspecifics.h"
#include "ccl/public/system/threadsync.h"

#include "ccl/public/base/ccldefpush.h"
#include "wayland-server-delegate/iwaylandclientcontext.h"
#include "ccl/public/base/ccldefpop.h"

struct wl_event_queue;

namespace WaylandServerDelegate {
class WaylandResource; }

namespace CCL {
namespace Linux {

class WaylandServerRunLoop;
class WaylandChildWindow;

//************************************************************************************************
// WaylandClientContext
//************************************************************************************************

class WaylandClientContext: public WaylandServerDelegate::IWaylandClientContext,
							public SystemEventHandler
{
public:
	void initialize ();
	void terminate ();

	// IWaylandClientContext
	bool addListener (WaylandServerDelegate::IContextListener* listener) override;
	bool removeListener (WaylandServerDelegate::IContextListener* listener) override;
	bool getSubSurfaceOffset (int32_t& x, int32_t& y, wl_display* display, wl_surface* parentSurface, wl_surface* childSurface) override;
	wl_compositor* getCompositor () const override;
	wl_subcompositor* getSubCompositor () const override;
	wl_shm* getSharedMemory () const override;
	wl_seat* getSeat () const override;
	xdg_wm_base* getWindowManager () const override;
	zwp_linux_dmabuf_v1* getDmaBuffer () const override;
	uint32_t getSeatCapabilities () const override;
	const char* getSeatName () const override;
	int countOutputs () const override;
	const WaylandServerDelegate::WaylandOutput& getOutput (int index) const override;
	int countDmaBufferModifiers () const override;
	bool getDmaBufferModifier (uint32_t& format, uint32_t& modifierHigh, uint32_t& modifierLow, int index) const override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

private:
	Vector<WaylandServerDelegate::IContextListener*> listeners;

	void signalChange (WaylandServerDelegate::IContextListener::ChangeType changeType);
};

//************************************************************************************************
// WaylandCompositor
//************************************************************************************************

class WaylandCompositor: public Object,
						 public IWaylandSocket,
						 public Singleton<WaylandCompositor>
{
public:
	WaylandCompositor ();
	~WaylandCompositor ();

	wl_proxy* createProxy (wl_display* display, wl_proxy* object, WaylandServerDelegate::WaylandResource* implementation);
	void destroyProxy (wl_proxy* proxy);

	void registerChildWindow (WaylandChildWindow* childWindow);
	void unregisterChildWindow (WaylandChildWindow* childWindow);
	bool getSubSurfaceOffset (Point& offset, wl_display* display, wl_surface* parentSurface, wl_surface* childSurface) const;

	// IWaylandSocket
	wl_display* CCL_API openWaylandConnection () override;
	tresult CCL_API closeWaylandConnection (wl_display* display) override;

	CLASS_INTERFACE (IWaylandSocket, Object)

private:
	Vector<WaylandChildWindow*> childWindows;

	WaylandClientContext clientContext;
	WaylandServerRunLoop* serverRunLoop;
	wl_event_queue* serverEventQueue;
	Threading::CriticalSection serverLock;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_waylandcompositor_h
