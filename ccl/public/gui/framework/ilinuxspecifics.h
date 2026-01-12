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
// Filename    : ccl/public/gui/framework/ilinuxspecifics.h
// Description : Interfaces specific to Linux
//
//************************************************************************************************

#ifndef _ccl_ilinuxspecifics_h
#define _ccl_ilinuxspecifics_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

struct wl_display;
struct wl_surface;
struct wl_proxy;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_popup;
struct xdg_positioner;

namespace CCL {
namespace Linux {

//************************************************************************************************
// WindowContext
/** Wayland window context.
	\ingroup gui_view */
//************************************************************************************************

struct WindowContext
{
	wl_surface* waylandSurface = nullptr;
	xdg_surface* xdgSurface = nullptr;
	xdg_toplevel* topLevelWindow = nullptr;
	xdg_popup* popupWindow = nullptr;
	xdg_positioner* positioner = nullptr;
	Point frameOffset;
};

//************************************************************************************************
// IWaylandChildWindow
/** Interface to query additional information about the parent of a child window in a Wayland session.
	\ingroup gui_view */
//************************************************************************************************

interface IWaylandChildWindow: IUnknown
{
	/**	Get the parent Wayland surface.
		The caller must not change the state of the parent surface. */
	virtual wl_surface* CCL_API getWaylandSurface (wl_display* display) = 0;

	/**	Get the parent XDG surface for creating popup windows.
		If the parent surface is not an xdg_surface, 
		this returns the first xdg_surface that can be found in the surface hierarchy,
		starting the search with the parent surface.
		The caller must not change the state of the parent surface.
		The size and position of the parent surface, relative to the top left corner of 
		the child window surface, is returned in parentSize. */
	virtual xdg_surface* CCL_API getParentSurface (Rect& parentSize, wl_display* display) = 0;

	/**	Get the XDG toplevel surface containing the child window.
		The caller must not change the state of the returned xdg_toplevel. */
	virtual xdg_toplevel* CCL_API getParentToplevel (wl_display* display) = 0;
	
	DECLARE_IID (IWaylandChildWindow)
};

DEFINE_IID (IWaylandChildWindow, 0xc93ab9fd, 0x0b9e, 0x4ca1, 0x8d, 0x31, 0x1b, 0xeb, 0xbc, 0xae, 0xd2, 0x8d)

//************************************************************************************************
// IEventHandler
/** Callback interface for custom events.
	\ingroup gui_view */
//************************************************************************************************

interface IEventHandler: IUnknown
{
	virtual void CCL_API onEvent (int eventFd) = 0;

	DECLARE_IID (IEventHandler)
};

DEFINE_IID (IEventHandler, 0xa1c331c3, 0xac05, 0x44dd, 0x9f, 0x90, 0xb9, 0x13, 0x54, 0x95, 0x6c, 0x11)

//************************************************************************************************
// IEventLoop
/** Interface allowing to register custom event handlers.
	\ingroup gui_view */
//************************************************************************************************

interface IEventLoop: IUnknown
{
	/** Register an event handler. */
	virtual tresult CCL_API addEventHandler (IEventHandler* handler, int eventFd) = 0;

	/** Remove a previously registered event handler. */
	virtual tresult CCL_API removeEventHandler (IEventHandler* handler) = 0;
	
	DECLARE_IID (IEventLoop)
};

DEFINE_IID (IEventLoop, 0xabb38ba6, 0x4672, 0x47ff, 0xbd, 0x1f, 0xca, 0x41, 0x1b, 0x51, 0x3e, 0x66)

//************************************************************************************************
// IWaylandSocket
/** Wayland socket interface.
	\ingroup gui_view */
//************************************************************************************************

interface IWaylandSocket: IUnknown
{
	/** Open a Wayland connection on this socket. */
	virtual wl_display* CCL_API openWaylandConnection () = 0;

	/** Close a previously created connection. */
	virtual tresult CCL_API closeWaylandConnection (wl_display* display) = 0;

	DECLARE_IID (IWaylandSocket)
};

DEFINE_IID (IWaylandSocket, 0xe224d518, 0xfb4d, 0x4a54, 0xac, 0x99, 0xb7, 0x4d, 0xc1, 0xfe, 0xd8, 0x67)

//************************************************************************************************
// IWaylandClient
/** Wayland client interface.

	This interface can be used to connect CCL-based applications or plug-ins to a custom compositor connection.

	By default, CCL connects to the default compositor, which is usually the session compositor.
	When setting a custom \a IWaylandSocket implementation using \a IWaylandClient::setWaylandSocket, CCL disconnects from the previous wl_display
	and connects to the custom implementation using \a IWaylandSocket::openWaylandConnection.
	All existing framework objects remain intact, but internal Wayland objects are replaced with objects from the new connection.

	In addition, \a IWaylandSocket::setEventLoop may be used to integrate CCL-based applications or plug-ins into a custom event loop.
	By default, CCL performs blocking reads. For example, CCL polls the wl_display's file descriptor.
	When setting a custom \a IEventLoop implementation, CCL registers event handlers using \a IEventLoop::addEventHandler instead.
	Like \a IWaylandClient::setWaylandSocket, this method causes CCL to disconnect from the previous wl_display.
	All existing framework objects remain intact, but internal Wayland objects are replaced with objects from a new connection.

	\ingroup gui_view */
//************************************************************************************************

interface IWaylandClient: IUnknown
{
	/** Get the Wayland connection object. */
	virtual wl_display* CCL_API getWaylandDisplay () const = 0;

	/** Use a specific wayland socket. Socket is shared. */
	virtual tresult CCL_API setWaylandSocket (IWaylandSocket* socket) = 0;

	/** Use an external event loop. Event loop is shared. */
	virtual tresult CCL_API setEventLoop (IEventLoop* eventLoop) = 0;
	
	DECLARE_IID (IWaylandClient)
};

DEFINE_IID (IWaylandClient, 0xa7627632, 0xe21b, 0x403a, 0xb5, 0x03, 0x5a, 0x77, 0x63, 0x43, 0x6e, 0x2e)

} // namespace Linux
} // namespace CCL

#endif // _ccl_ilinuxspecifics_h
