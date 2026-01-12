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
// Filename    : ccl/platform/linux/wayland/imagesurface.cpp
// Description : Wayland Image Surface
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/imagesurface.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicsdevice.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// ImageSurface
//************************************************************************************************

ImageSurface::ImageSurface ()
: scaleFactor (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSurface::setImage (Image* _image)
{
	image = _image;
	render ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSurface::createSurface ()
{
	Surface::createSurface ();
	clearInputRegion ();
	render ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSurface::setScaleFactor (int factor)
{
	scaleFactor = factor;
	render ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSurface::render ()
{
	if(image == nullptr)
		return;

	if(wl_surface* surface = getWaylandSurface ())
	{
		wl_surface_set_buffer_scale (surface, scaleFactor);

		PointRef size (image->getSize ());
		AutoPtr<Bitmap> bitmap = NEW Bitmap (size.x, size.y, Bitmap::kRGBAlpha, scaleFactor);
		BitmapGraphicsDevice device (bitmap);
		if(!device.isNullDevice ())
			device.drawImage (image, Point (0, 0));

		buffer.fromBitmap (*bitmap);
		buffer.attach (surface, -size.x / 2, -size.y);
	}
}
