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
// Filename    : ccl/platform/linux/wayland/surface.cpp
// Description : Wayland Surface
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/surface.h"
#include "ccl/platform/linux/wayland/datadevicehelper.h"
#include "ccl/platform/linux/wayland/monitorhelper.h"

#include <linux/input-event-codes.h>

#include <unistd.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// Surface
//************************************************************************************************

Surface::Surface ()
: surfaceListener (*this),
  waylandSurface (nullptr),
  inputEnabled (false),
  wasInputEnabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface::~Surface ()
{
	ASSERT (waylandSurface == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_output* Surface::getOutput () const
{
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::setOutput (wl_output* newOutput)
{
	if(newOutput)
	{
		if(waylandSurface)
		{
			#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
			if(wl_surface_get_version (waylandSurface) < WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION)
			#endif
			{
				int scaleFactor = MonitorHelper::instance ().getScaleFactor (newOutput);
				setScaleFactor (scaleFactor);
			}
		}
		output = newOutput;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::setScaleFactor (int scaleFactor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::enableInput (bool state)
{
	// drag&drop
	if(state)
		DataDeviceHelper::instance ().registerSurface (*this);
	else
		DataDeviceHelper::instance ().unregisterSurface (*this);
	
	// keyboard / mouse / touch
	if(state)
		InputHandler::instance ().registerSurface (*this);
	else
		InputHandler::instance ().unregisterSurface (*this);
	
	inputEnabled = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::clearInputRegion ()
{
	if(!WaylandClient::instance ().isInitialized ())
		return;
	wl_region* region = wl_compositor_create_region (WaylandClient::instance ().getCompositor ());
	if(region)
	{
		wl_surface_set_input_region (getWaylandSurface (), region);
		wl_region_destroy (region);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::createSurface ()
{
	wl_compositor* compositor = WaylandClient::instance ().getCompositor ();
	if(compositor == nullptr)
		return;
	
	ASSERT (getWaylandSurface () == nullptr)
	setWaylandSurface (wl_compositor_create_surface (compositor));
	if(getWaylandSurface () == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to create a Wayland surface.")
		return;
	}
	wl_surface_add_listener (getWaylandSurface (), &surfaceListener, &surfaceListener);	
	
	WaylandClient::instance ().registerObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::destroySurface ()
{
	WaylandClient::instance ().unregisterObject (*this);
	
	if(getWaylandSurface () != nullptr && WaylandClient::instance ().isInitialized ())
		wl_surface_destroy (getWaylandSurface ());
	setWaylandSurface (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::onCompositorDisconnected ()
{
	wasInputEnabled = inputEnabled;
	enableInput (false);
	destroySurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::onCompositorConnected ()
{
	createSurface ();
	enableInput (wasInputEnabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::commit ()
{
	if(wl_surface* surface = getWaylandSurface ())
		wl_surface_commit (surface);
}

//************************************************************************************************
// Surface::SurfaceListener
//************************************************************************************************

Surface::SurfaceListener::SurfaceListener (Surface& surface)
: surface (surface)
{	
	wl_surface_listener::enter = onEnter;
	wl_surface_listener::leave = onLeave;

	#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
	wl_surface_listener::preferred_buffer_scale = onPreferredBufferScale;
	wl_surface_listener::preferred_buffer_transform = onPreferredBufferTransform;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::SurfaceListener::onEnter (void *data, wl_surface* surface, wl_output* output)
{
	SurfaceListener* This = static_cast<SurfaceListener*> (data);
	if(This->surface.getWaylandSurface () == surface)
	{
		This->surface.setOutput (output);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::SurfaceListener::onLeave (void *data, wl_surface* surface, wl_output* output)
{
	SurfaceListener* This = static_cast<SurfaceListener*> (data);
	if(This->surface.getWaylandSurface () == surface)
	{
		This->surface.setOutput (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::SurfaceListener::onPreferredBufferScale (void* data, wl_surface* surface, int32_t factor)
{
	SurfaceListener* This = static_cast<SurfaceListener*> (data);
	if(This->surface.getWaylandSurface () == surface)
	{
		This->surface.setScaleFactor (factor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Surface::SurfaceListener::onPreferredBufferTransform (void* data, wl_surface* surface, uint32_t transform)
{}
