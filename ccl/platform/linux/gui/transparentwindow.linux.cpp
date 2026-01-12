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
// Filename    : ccl/platform/linux/gui/transparentwindow.linux.cpp
// Description : Transparent Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/gui/transparentwindow.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/offscreen.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW LinuxTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// LinuxTransparentWindow
//************************************************************************************************

LinuxTransparentWindow::LinuxTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  SubSurface (*LinuxWindow::cast (parentWindow)),
  initialized (false),
  visible (false),
  suspended (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxTransparentWindow::~LinuxTransparentWindow ()
{
	enableInput (false);
	destroySurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTransparentWindow::show ()
{
	if(suspended)
		return;
    
	if(visible)
		return;

	createSurface ();
	enableInput ();
	setSynchronous (false);
	visible = true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTransparentWindow::hide ()
{
	if(suspended)
		return;
    
	if(!visible)
		return;
	
	enableInput (false);
	destroySurface ();
	visible = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxTransparentWindow::isVisible () const
{
	return visible;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTransparentWindow::update (RectRef _size, Bitmap& bitmap, PointRef offset, float opacity)
{
	size = _size;
	setPosition (size.getLeftTop ());
	
	wl_surface* surface = getWaylandSurface ();
	if(surface == nullptr)
		return;
	
	for(WaylandBuffer& buffer : buffers)
	{
		if(buffer.ready ())
		{
			AutoPtr<Offscreen> offscreen = (getContentScaleFactor () == 1.f && offset.isNull () && opacity == 1.f && bitmap.getPixelFormat () == IBitmap::kRGBAlpha) ? nullptr : NEW Offscreen (size.getWidth (), size.getHeight (), IBitmap::kRGBAlpha, false, parentWindow);
			if(offscreen)
			{
				Rect windowSize (size.getSize ());
				Rect src (windowSize);
				src.offset (offset);
				BitmapGraphicsDevice device (offscreen);
				ImageMode mode (opacity);
				device.drawImage (&bitmap, src, windowSize, &mode);
			}
			wl_surface_set_buffer_scale (surface, getContentScaleFactor ());
			
			buffer.fromBitmap (offscreen ? *offscreen : bitmap);
			buffer.attach (surface);
			break;
		}
	}
	
	// request a new frame for the parent surface
	if(parentWindow)
		parentWindow->invalidate (Rect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTransparentWindow::move (PointRef _position)
{
	setPosition (_position);
	size.moveTo (_position);

	commit ();

	// request a new frame for the parent surface
	if(parentWindow)
		parentWindow->invalidate (Rect ());
}
