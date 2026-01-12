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
// Filename    : ccl/gui/graphics/graphicsdevice.cpp
// Description : Graphics Device
//
//************************************************************************************************

#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/gui/graphics/imaging/bitmap.h"

using namespace CCL;

//************************************************************************************************
// GraphicsDeviceBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (GraphicsDeviceBase, Object)

//************************************************************************************************
// GraphicsDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GraphicsDevice, GraphicsDeviceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice::GraphicsDevice ()
: nativeDevice (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice::~GraphicsDevice ()
{
	if(nativeDevice)
		nativeDevice->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice::GraphicsDevice (const GraphicsDevice&)
: nativeDevice (nullptr)
{
	ASSERT (0) // not allowed!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsDevice::setNativeDevice (NativeGraphicsDevice* device)
{
	take_shared (nativeDevice, device);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* GraphicsDevice::getNativeDevice ()
{
	return nativeDevice;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsDevice::isNullDevice () const
{
	return ccl_cast<NullGraphicsDevice> (nativeDevice) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsDevice::setOrigin (PointRef origin)
{
	if(origin != nativeDevice->getOrigin ())
		nativeDevice->setOrigin (origin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::saveState ()
{
	return nativeDevice->saveState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::restoreState ()
{
	return nativeDevice->restoreState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::addClip (RectRef rect)
{
	return nativeDevice->addClip (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::addClip (RectFRef rect)
{
	return nativeDevice->addClip (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::addClip (IGraphicsPath* path)
{
	GraphicsPath* internalPath = unknown_cast<GraphicsPath> (path);
	ASSERT (internalPath != nullptr)
	if(internalPath)
		return nativeDevice->addClip (&internalPath->getNativePath ());

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::addTransform (TransformRef matrix)
{
	return nativeDevice->addTransform (matrix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::setMode (int mode)
{
	return nativeDevice->setMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API GraphicsDevice::getMode ()
{
	return nativeDevice->getMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API GraphicsDevice::getContentScaleFactor () const
{
	return nativeDevice->getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::clearRect (RectRef rect)
{
	return nativeDevice->clearRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::clearRect (RectFRef rect)
{
	return nativeDevice->clearRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillRect (RectRef rect, BrushRef brush)
{
	return nativeDevice->fillRect (rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillRect (RectFRef rect, BrushRef brush)
{
	return nativeDevice->fillRect (rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawRect (RectRef rect, PenRef pen)
{
	return nativeDevice->drawRect (rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawRect (RectFRef rect, PenRef pen)
{
	return nativeDevice->drawRect (rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	return nativeDevice->drawLine (p1, p2, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	return nativeDevice->drawLine (p1, p2, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawEllipse (RectRef rect, PenRef pen)
{
	return nativeDevice->drawEllipse (rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawEllipse (RectFRef rect, PenRef pen)
{
	return nativeDevice->drawEllipse (rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillEllipse (RectRef rect, BrushRef brush)
{
	return nativeDevice->fillEllipse (rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillEllipse (RectFRef rect, BrushRef brush)
{
	return nativeDevice->fillEllipse (rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawPath (IGraphicsPath* path, PenRef pen)
{
	GraphicsPath* internalPath = unknown_cast<GraphicsPath> (path);
	ASSERT (internalPath != nullptr)
	if(internalPath)
		return internalPath->getNativePath ().draw (*nativeDevice, pen);
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillPath (IGraphicsPath* path, BrushRef brush)
{
	GraphicsPath* internalPath = unknown_cast<GraphicsPath> (path);
	ASSERT (internalPath != nullptr)
	if(internalPath)
		return internalPath->getNativePath ().fill (*nativeDevice, brush);
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen)
{
	ASSERT (rect.isEmpty () == false)
	return nativeDevice->drawRoundRect (rect, rx, ry, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen)
{
	ASSERT (rect.isEmpty () == false)
	return nativeDevice->drawRoundRect (rect, rx, ry, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush)
{
	ASSERT (rect.isEmpty () == false)
	return nativeDevice->fillRoundRect (rect, rx, ry, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush)
{
	ASSERT (rect.isEmpty () == false)
	return nativeDevice->fillRoundRect (rect, rx, ry, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawTriangle (const Point points[3], PenRef pen)
{
	return nativeDevice->drawTriangle (points, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawTriangle (const PointF points[3], PenRef pen)
{
	return nativeDevice->drawTriangle (points, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillTriangle (const Point points[3], BrushRef brush)
{
	return nativeDevice->fillTriangle (points, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::fillTriangle (const PointF points[3], BrushRef brush)
{
	return nativeDevice->fillTriangle (points, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return nativeDevice->drawString (rect, text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return nativeDevice->drawString (rect, text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return nativeDevice->drawString (point, text, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return nativeDevice->drawString (point, text, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API GraphicsDevice::getStringWidth (StringRef text, FontRef font)
{
	return nativeDevice->getStringWidth (text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF CCL_API GraphicsDevice::getStringWidthF (StringRef text, FontRef font)
{
	return nativeDevice->getStringWidthF (text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::measureString (Rect& size, StringRef text, FontRef font)
{
	return nativeDevice->measureString (size, text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::measureString (RectF& size, StringRef text, FontRef font)
{
	return nativeDevice->measureString (size, text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font)
{
	return nativeDevice->measureText (size, lineWidth, text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font)
{
	return nativeDevice->measureText (size, lineWidth, text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	return nativeDevice->drawText (rect, text, font, brush, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	return nativeDevice->drawText (rect, text, font, brush, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	return nativeDevice->drawTextLayout (pos, textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	return nativeDevice->drawTextLayout (pos, textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawImage (IImage* image, PointRef pos, const ImageMode* mode)
{
	if(image == nullptr)
		return kResultInvalidArgument;

	Image* internalImage = unknown_cast<Image> (image);
	ASSERT (internalImage != nullptr)
	if(internalImage)
		return internalImage->draw (*this, pos, mode);
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawImage (IImage* image, PointFRef pos, const ImageMode* mode)
{
	if(image == nullptr)
		return kResultInvalidArgument;

	Image* internalImage = unknown_cast<Image> (image);
	ASSERT (internalImage != nullptr)
	if(internalImage)
		return internalImage->draw (*this, pos, mode);
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode)
{
	Image* internalImage = unknown_cast<Image> (image);
	ASSERT (internalImage != nullptr)
	if(internalImage)
		return internalImage->draw (*this, src, dst, mode);
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsDevice::drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	Image* internalImage = unknown_cast<Image> (image);
	ASSERT (internalImage != nullptr)
	if(internalImage)
		return internalImage->draw (*this, src, dst, mode);
	return kResultInvalidArgument;
}

//************************************************************************************************
// BitmapGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BitmapGraphicsDevice, GraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapGraphicsDevice::BitmapGraphicsDevice (Bitmap* bitmap)
: bitmap (bitmap)
{
	ASSERT (bitmap != nullptr)
	if(!bitmap)
		return;

	bitmap->retain ();
	ASSERT (bitmap->getNativeBitmap () != nullptr)
	NativeGraphicsDevice* device = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createBitmapDevice (bitmap->getNativeBitmap ()));
	setNativeDevice (device);
	device->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapGraphicsDevice::~BitmapGraphicsDevice ()
{
	setNativeDevice (nullptr);

	if(bitmap)
		bitmap->release ();
}
