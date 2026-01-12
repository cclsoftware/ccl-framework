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
// Filename    : ccl/platform/linux/wayland/surface.h
// Description : Wayland Surface
//
//************************************************************************************************

#ifndef _ccl_linux_surface_h
#define _ccl_linux_surface_h

#include "ccl/platform/linux/wayland/waylandclient.h"
#include "ccl/platform/linux/wayland/inputhandler.h"

namespace CCL {
struct KeyEvent;
struct FocusEvent;
class View;

namespace Linux {

//************************************************************************************************
// Surface
//************************************************************************************************

class Surface: public WaylandObject
{
public:
	Surface ();
	virtual ~Surface ();

	struct SurfaceListener: wl_pointer_listener,
							wl_surface_listener,
							wl_touch_listener
	{
		SurfaceListener (Surface& surface);
		
		Surface& surface;
		
		// wayland surface
		static void onEnter (void* data, wl_surface* surface, wl_output* output);
		static void onLeave (void* data, wl_surface* surface, wl_output* output);
		static void onPreferredBufferScale (void* data, wl_surface* surface, int32_t factor);
		static void onPreferredBufferTransform (void* data, wl_surface* surface, uint32_t transform);
	};
	
	PROPERTY_POINTER (wl_surface, waylandSurface, WaylandSurface)
	
	wl_output* getOutput () const;
	virtual void setOutput (wl_output* output);
	virtual void setScaleFactor (int scaleFactor);
	
	virtual void createSurface ();
	virtual void destroySurface ();
	
	virtual bool suppressInput () { return false; }
	
	virtual void handleKeyboardEvent (const KeyEvent& event) {}
	virtual void handleFocus (const FocusEvent& event) {}
	virtual void handlePointerEvent (const InputHandler::PointerEvent& event) {};
	virtual void handleTouchEvent (const InputHandler::TouchEvent& event) {};
	
	virtual View* getView ()  { return nullptr; }
	
	virtual void enableInput (bool state = true);
	void clearInputRegion ();
	
	void commit ();

	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;
	
protected:
	SurfaceListener surfaceListener;
	bool inputEnabled;
	bool wasInputEnabled;
	
	wl_output* output;
};

} // namespace Linux
} // namespace CCL
	
#endif // _ccl_linux_surface_h
