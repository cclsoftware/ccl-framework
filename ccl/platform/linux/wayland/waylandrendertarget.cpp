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
// Filename    : ccl/platform/linux/wayland/waylandrendertarget.cpp
// Description : Wayland Render Target
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/waylandrendertarget.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// WaylandRenderTarget
//************************************************************************************************

WaylandRenderTarget::WaylandRenderTarget ()
: waylandSurface (nullptr),
  listener (nullptr),
  updateRegion (*this),
  invalidateRegion (*this),
  scaleFactor (0.f),
  contentScaleChanged (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandRenderTarget::~WaylandRenderTarget ()
{
	if(listener)
		delete listener;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::applyContentScaleFactor ()
{
	if(getWaylandSurface () != nullptr)
		wl_surface_set_buffer_scale (getWaylandSurface (), scaleFactor);
	contentScaleChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::onContentScaleFactorChanged (float factor)
{
	if(scaleFactor != factor)
	{
		// We need to make sure that the surface dimensions are a multiple of the scaling factor.
		// If the new content scale factor is smaller, we set the new factor directly and render with the smaller (potentially uneven) content size.
		// If the new content scale is larger, we render with the new content size (now a multiple of the scaling factor) and set the factor afterwards.
		bool applyImmediately = scaleFactor > factor;
		scaleFactor = factor;
		contentScaleChanged = true;
		if(applyImmediately)
			applyContentScaleFactor ();
	}
}

//************************************************************************************************
// WaylandMutableRegion
//************************************************************************************************

WaylandRenderTarget::WaylandMutableRegion::WaylandMutableRegion (WaylandRenderTarget& target)
: target (target)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::WaylandMutableRegion::addRect (RectRef rect, bool requestFrame)
{
	MutableRegion::addRect (rect);
	if(target.listener && requestFrame)
		target.listener->requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::WaylandMutableRegion::addRect (RectRef rect)
{
	addRect (rect, true);
}

//************************************************************************************************
// WaylandRenderTarget::Listener
//************************************************************************************************

WaylandRenderTarget::Listener::Listener (WaylandRenderTarget* target)
: target (target),
  callback (nullptr)
{
	done = onFrameCallback;
	requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandRenderTarget::Listener::~Listener ()
{
	if(callback)
	{
		wl_callback_set_user_data (callback, nullptr);
		if(WaylandClient::instance ().isInitialized ())
			wl_callback_destroy (callback);
	}
	target = nullptr;
	callback = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::Listener::requestFrame ()
{
	if(wl_surface* surface = target->getWaylandSurface ())
	{
		if(callback == nullptr && WaylandClient::instance ().isInitialized ())
		{
			callback = wl_surface_frame (target->getWaylandSurface ());
			wl_callback_set_user_data (callback, this);
			wl_callback_add_listener (callback, this, this);
			wl_surface_commit (surface);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandRenderTarget::Listener::onFrameCallback (void* data, wl_callback* callback, uint32_t time)
{
	Listener* This = static_cast<Listener*> (wl_callback_get_user_data (callback));
	if(This == nullptr)
		return;
	
	if(This->callback == callback)
	{
		if(WaylandClient::instance ().isInitialized ())
			wl_callback_destroy (callback);
		This->callback = nullptr;
	}
	
	if(This->target == nullptr)
	{
		delete This;
		return;
	}
	
	// draw a frame
	This->target->onFrameCallback ();
}
