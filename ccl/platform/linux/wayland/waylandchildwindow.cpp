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
// Filename    : ccl/platform/linux/wayland/waylandchildwindow.cpp
// Description : Wayland-specific Child Window implementation
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/waylandchildwindow.h"
#include "ccl/platform/linux/wayland/cclwaylandserver.h"
#include "ccl/platform/linux/wayland/waylandcompositor.h"

#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/base/message.h"

#include "wayland-server-delegate/waylandresource.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// ChildWindowWaylandSurfaceDelegate
//************************************************************************************************

class ChildWindowWaylandSurfaceDelegate: public WaylandServerDelegate::WaylandResource,
										 public wl_surface_interface
{
public:
	ChildWindowWaylandSurfaceDelegate ();

	// interface
	static void onDestroy (wl_client* client, wl_resource* resource);
	static void onAttach (wl_client* client, wl_resource* resource, wl_resource* buffer, int32_t x, int32_t y);
	static void onDamage (wl_client* client, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
	static void requestFrame (wl_client* client, wl_resource* resource, uint32_t callback);
	static void setOpaqueRegion (wl_client* client, wl_resource* resource, wl_resource* region);
	static void setInputRegion (wl_client* client, wl_resource* resource, wl_resource* region);
	static void onCommit (wl_client* client, wl_resource* resource);
	static void setBufferTransform (wl_client* client, wl_resource* resource, int32_t transform);
	static void setBufferScale (wl_client* client, wl_resource* resource, int32_t scale);
	static void onDamageBuffer (wl_client* client, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
	static void setOffset (wl_client* client, wl_resource* resource, int32_t x, int32_t y);
};

//************************************************************************************************
// ChildWindowSurfaceDelegate
//************************************************************************************************

class ChildWindowSurfaceDelegate: public WaylandServerDelegate::WaylandResource,
								  public xdg_surface_interface
{
public:
	ChildWindowSurfaceDelegate ();

	// interface
	static void onDestroy (wl_client* client, wl_resource* resource);
	static void getToplevel (wl_client* client, wl_resource* resource, uint32_t id);
	static void getPopup (wl_client* client, wl_resource* resource, uint32_t id, wl_resource* parent, wl_resource* positioner);
	static void setWindowGeometry (wl_client *client, wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
	static void ackConfigure (wl_client* client, wl_resource* resource, uint32_t serial);
};

//************************************************************************************************
// ChildWindowToplevelDelegate
//************************************************************************************************

class ChildWindowToplevelDelegate: public WaylandServerDelegate::WaylandResource,
								   public xdg_toplevel_interface
{
public:
	ChildWindowToplevelDelegate ();

	// interface
	static void onDestroy (wl_client* client, wl_resource* resource);
	static void setTitle (wl_client* client, wl_resource* resource, const char* title);
	static void setApplicationID (wl_client* client, wl_resource* resource, const char* applicationId);
	static void showWindowMenu (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial, int32_t x, int32_t y);
	static void onMove (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial);
	static void onResize (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial, uint32_t edges);
	static void setMaxSize (wl_client* client, wl_resource* resource, int32_t width, int32_t height);
	static void setMinSize (wl_client* client, wl_resource *resource, int32_t width, int32_t height);
	static void setMaximized (wl_client* client, wl_resource *resource);
	static void unsetMaximized (wl_client* client, wl_resource *resource);
	static void setFullscreen (wl_client* client, wl_resource *resource, wl_resource* output);
	static void unsetFullscreen (wl_client* client, wl_resource *resource);
	static void setMinimized (wl_client* client, wl_resource *resource);
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;
using namespace WaylandServerDelegate;

//************************************************************************************************
// WaylandChildWindow
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (WaylandChildWindow, kSurfaceCreated, "surfaceCreated")

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandChildWindow::WaylandChildWindow (LinuxWindow& window)
: SubSurface (window),
  window (window),
  display (nullptr),
  waylandSurface (nullptr),
  parentSurface (nullptr),
  parentToplevel (nullptr),
  size ({0, 0, 1, 1})
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandChildWindow::~WaylandChildWindow ()
{
	cancelSignals ();
	show (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WaylandChildWindow::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kSurfaceCreated)
		commit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandChildWindow::show (bool state)
{
	WaylandCompositor& compositor = WaylandCompositor::instance ();
	if(state == true)
	{
		commit ();
		compositor.registerChildWindow (this);
	}
	else
	{
		compositor.unregisterChildWindow (this);
		if(waylandSurface)
			compositor.destroyProxy (waylandSurface);
		waylandSurface = nullptr;
		if(parentSurface)
			compositor.destroyProxy (parentSurface);
		parentSurface = nullptr;
		if(parentToplevel)
			compositor.destroyProxy (parentToplevel);
		parentToplevel = nullptr;

		if(Surface::getWaylandSurface ())
			destroySurface ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandChildWindow::getParentOffset (Point& offset, wl_surface* parentSurface) const
{
	offset = getPosition ();

	IWindow* parent = &window;
	while(parent != nullptr)
	{
		LinuxWindow* window = LinuxWindow::cast (unknown_cast<Window> (parent));
		const WindowContext* parentContext = window->getNativeContext ();
		if(window && parentContext)
		{
			if(parentContext->waylandSurface == parentSurface)
				return true;
			offset += window->getPosition ();
		}
		parent = window ? LinuxWindow::cast (window)->getParentLinuxWindow () : nullptr;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandChildWindow::setUserSize (RectRef _size)
{
	size = _size;

	wl_surface* surface = Surface::getWaylandSurface ();
	if(surface == nullptr)
		return;

	wl_surface* parentSurface = window.getWindowContext ().waylandSurface;
	if(parentSurface == nullptr)
		return;

	buffer.resize ({size.getWidth (), size.getHeight ()}, size.getWidth () * 4);
	::memset (buffer.getData (), 0, buffer.getByteSize ());
	buffer.attach (Surface::getWaylandSurface (), 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_surface* CCL_API WaylandChildWindow::getWaylandSurface (wl_display* _display)
{
	ASSERT (display == nullptr || display == _display)
	if(display == nullptr)
		display = _display;
	if(display != _display)
		return nullptr;

	if(waylandSurface == nullptr)
	{
		if(Surface::getWaylandSurface () == nullptr)
		{
			// update window context with the current state of the parent window
			const WindowContext* parentContext = window.getNativeContext ();
			if(parentContext)
				window.setWindowContext (*parentContext);

			ASSERT (window.getWindowContext ().waylandSurface != nullptr)
			if(wl_surface* parentSurface = window.getWindowContext ().waylandSurface)
			{
				// create an asynchronous subsurface
				window.setWaylandSurface (parentSurface);
				createSurface ();
				setSynchronous (false);
				window.setWaylandSurface (nullptr);

				// We need to attach a buffer. Children of surfaces which have no buffer attached are always invisible.
				setUserSize (size);

				// committing the parent surface applies the initial subsurface position
				wl_surface_commit (parentSurface);
			}
		}

		WaylandCompositor& compositor = WaylandCompositor::instance ();
		surfaceDelegate = NEW ChildWindowWaylandSurfaceDelegate;
		waylandSurface = compositor.createProxy (display, reinterpret_cast<wl_proxy*> (Surface::getWaylandSurface ()), surfaceDelegate);

		(NEW Message (kSurfaceCreated))->post (this);
	}
	return reinterpret_cast<wl_surface*> (waylandSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

xdg_surface* CCL_API WaylandChildWindow::getParentSurface (Rect& parentSize, wl_display* _display)
{
	ASSERT (display == nullptr || display == _display)
	if(display != _display)
		return nullptr;

	if(parentSurface == nullptr)
	{
		// update window context with the current state of the parent window
		const WindowContext* parentContext = window.getNativeContext ();
		if(parentContext)
			window.setWindowContext (*parentContext);

		WindowContext recursiveContext;
		window.getParentContextRecursive (recursiveContext, true);

		Point parentOffset = window.getPositioningOffset ();
		parentSize.moveTo (parentOffset);
		parentSize.offset (position);
		parentSize.setSize (window.getSize ().getSize ());

		WaylandCompositor& compositor = WaylandCompositor::instance ();
		WaylandResource* implementation = NEW ChildWindowSurfaceDelegate;
		parentSurface = compositor.createProxy (display, reinterpret_cast<wl_proxy*> (recursiveContext.xdgSurface), implementation);
	}
	return reinterpret_cast<xdg_surface*> (parentSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

xdg_toplevel* CCL_API WaylandChildWindow::getParentToplevel (wl_display* _display)
{
	ASSERT (display == nullptr || display == _display)
	if(display != _display)
		return nullptr;

	if(parentToplevel == nullptr)
	{
		// update window context with the current state of the parent window
		const WindowContext* parentContext = window.getNativeContext ();
		if(parentContext)
			window.setWindowContext (*parentContext);

		WindowContext recursiveContext;
		window.getParentContextRecursive (recursiveContext, true);

		WaylandCompositor& compositor = WaylandCompositor::instance ();
		WaylandResource* implementation = NEW ChildWindowToplevelDelegate;
		parentToplevel = compositor.createProxy (display, reinterpret_cast<wl_proxy*> (recursiveContext.topLevelWindow), implementation);
	}
	return reinterpret_cast<xdg_toplevel*> (parentToplevel);
}

//************************************************************************************************
// ChildWindowWaylandSurfaceDelegate
//************************************************************************************************

ChildWindowWaylandSurfaceDelegate::ChildWindowWaylandSurfaceDelegate ()
: WaylandResource (&::wl_surface_interface, static_cast<wl_surface_interface*> (this))
{
	destroy = onDestroy;
	attach = onAttach;
	damage = onDamage;
	frame = requestFrame;
	set_opaque_region = setOpaqueRegion;
	set_input_region = setInputRegion;
	commit = onCommit;
	set_buffer_transform = setBufferTransform;
	set_buffer_scale = setBufferScale;
	damage_buffer = onDamageBuffer;
	offset = setOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::onDestroy (wl_client* client, wl_resource* resource)
{
	WaylandResource::onDestroy (resource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::onAttach (wl_client* client, wl_resource* resource, wl_resource* buffer, int32_t x, int32_t y)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::onDamage (wl_client* client, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::requestFrame (wl_client* client, wl_resource* resource, uint32_t callback)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::setOpaqueRegion (wl_client* client, wl_resource* resource, wl_resource* region)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::setInputRegion (wl_client* client, wl_resource* resource, wl_resource* region)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::onCommit (wl_client* client, wl_resource* resource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::setBufferTransform (wl_client* client, wl_resource* resource, int32_t transform)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::setBufferScale (wl_client* client, wl_resource* resource, int32_t scale)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::onDamageBuffer (wl_client* client, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowWaylandSurfaceDelegate::setOffset (wl_client* client, wl_resource* resource, int32_t x, int32_t y)
{}

//************************************************************************************************
// ChildWindowSurfaceDelegate
//************************************************************************************************

ChildWindowSurfaceDelegate::ChildWindowSurfaceDelegate ()
: WaylandResource (&::xdg_surface_interface, static_cast<xdg_surface_interface*> (this))
{
	destroy = onDestroy;
	get_toplevel = getToplevel;
	get_popup = getPopup;
	set_window_geometry = setWindowGeometry;
	ack_configure = ackConfigure;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowSurfaceDelegate::onDestroy (wl_client* client, wl_resource* resource)
{
	WaylandResource::onDestroy (resource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowSurfaceDelegate::getToplevel (wl_client* client, wl_resource* resource, uint32_t id)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowSurfaceDelegate::getPopup (wl_client* client, wl_resource* resource, uint32_t id, wl_resource* parent, wl_resource* positioner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowSurfaceDelegate::setWindowGeometry (wl_client *client, wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowSurfaceDelegate::ackConfigure (wl_client* client, wl_resource* resource, uint32_t serial)
{}

//************************************************************************************************
// ChildWindowToplevelDelegate
//************************************************************************************************

ChildWindowToplevelDelegate::ChildWindowToplevelDelegate ()
: WaylandResource (&::xdg_toplevel_interface, static_cast<xdg_toplevel_interface*> (this))
{
	destroy = onDestroy;
	set_title = setTitle;
	set_app_id = setApplicationID;
	show_window_menu = showWindowMenu;
	move = onMove;
	resize = onResize;
	set_max_size = setMaxSize;
	set_min_size = setMinSize;
	set_maximized = setMaximized;
	unset_maximized = unsetMaximized;
	set_fullscreen = setFullscreen;
	unset_fullscreen = unsetFullscreen;
	set_minimized = setMinimized;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::onDestroy (wl_client* client, wl_resource* resource)
{
	WaylandResource::onDestroy (resource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setTitle (wl_client* client, wl_resource* resource, const char* title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setApplicationID (wl_client* client, wl_resource* resource, const char* applicationId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::showWindowMenu (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::onMove (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::onResize (wl_client* client, wl_resource* resource, wl_resource* seat, uint32_t serial, uint32_t edges)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setMaxSize (wl_client* client, wl_resource* resource, int32_t width, int32_t height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setMinSize (wl_client* client, wl_resource *resource, int32_t width, int32_t height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setMaximized (wl_client* client, wl_resource *resource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::unsetMaximized (wl_client* client, wl_resource *resource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setFullscreen (wl_client* client, wl_resource *resource, wl_resource* output)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::unsetFullscreen (wl_client* client, wl_resource *resource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowToplevelDelegate::setMinimized (wl_client* client, wl_resource *resource)
{}
