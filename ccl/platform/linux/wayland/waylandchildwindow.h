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
// Filename    : ccl/platform/linux/wayland/waylandchildwindow.h
// Description : Wayland-specific Child Window implementation
//
//************************************************************************************************

#ifndef _ccl_waylandchildwindow_h
#define _ccl_waylandchildwindow_h

#include "ccl/platform/linux/wayland/subsurface.h"
#include "ccl/platform/linux/wayland/waylandbuffer.h"

#include "ccl/public/gui/framework/ilinuxspecifics.h"

namespace WaylandServerDelegate {
class WaylandResource; }

namespace CCL {
class LinuxWindow;

namespace Linux {

//************************************************************************************************
// WaylandChildWindow
//************************************************************************************************

class WaylandChildWindow: public Object,
						  public SubSurface<>,
						  public IWaylandChildWindow
{
public:
	WaylandChildWindow (LinuxWindow& window);
	~WaylandChildWindow ();

	PROPERTY_POINTER (wl_display, display, Display)

	void show (bool state);
	void setUserSize (RectRef size);
	bool getParentOffset (Point& offset, wl_surface* parentSurface) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IWaylandChildWindow
	wl_surface* CCL_API getWaylandSurface (wl_display* display) override;
	xdg_surface* CCL_API getParentSurface (Rect& parentSize, wl_display* display) override;
	xdg_toplevel* CCL_API getParentToplevel (wl_display* display) override;

	CLASS_INTERFACE (IWaylandChildWindow, Object)

private:
	DECLARE_STRINGID_MEMBER (kSurfaceCreated)

	WaylandServerDelegate::WaylandResource* surfaceDelegate;

	Rect size;
	LinuxWindow& window;
	WaylandBuffer buffer;
	wl_proxy* waylandSurface;
	wl_proxy* parentSurface;
	wl_proxy* parentToplevel;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_waylandchildwindow_h
