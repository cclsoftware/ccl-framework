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
// Filename    : ccl/platform/linux/wayland/waylandrendertarget.h
// Description : Wayland Render Target
//
//************************************************************************************************

#ifndef _ccl_waylandrendertarget_h
#define _ccl_waylandrendertarget_h

#include "ccl/platform/linux/wayland/waylandclient.h"

#include "ccl/gui/graphics/mutableregion.h"

namespace CCL {
class SkiaRenderTarget;

//************************************************************************************************
// WaylandRenderTarget
//************************************************************************************************

class WaylandRenderTarget: public Object
{
public:
	WaylandRenderTarget ();
	~WaylandRenderTarget ();
	
	PROPERTY_POINTER (wl_surface, waylandSurface, WaylandSurface)
    
	virtual bool onFrameCallback () { return false; }
	
protected:
	class WaylandMutableRegion: public MutableRegion
	{
	public:
		WaylandMutableRegion (WaylandRenderTarget& target);
		
		void addRect (RectRef rect, bool requestFrame);

		// MutableRegion
		void CCL_API addRect (RectRef rect);
		
	private:
		WaylandRenderTarget& target;
	};
	friend class WaylandMutableRegion;
	
	struct Listener: wl_callback_listener
	{
		Listener (WaylandRenderTarget* target);
		~Listener ();
		
		WaylandRenderTarget* target;
		wl_callback* callback;
		
		void requestFrame ();
		
		// frame callback
		static void onFrameCallback (void* data, wl_callback* callback, uint32_t time);
	};
	Listener* listener;
    
	WaylandMutableRegion updateRegion;
	WaylandMutableRegion invalidateRegion;

	float scaleFactor;
	bool contentScaleChanged;

	void applyContentScaleFactor ();
	void onContentScaleFactorChanged (float factor);
};

} // namespace CCL

#endif // _ccl_waylandrendertarget_h

