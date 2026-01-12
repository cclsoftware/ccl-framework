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
// Filename    : ccl/gui/graphics/imaging/imagepart.cpp
// Description : Image Part
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/imagepart.h"

#include "ccl/public/gui/graphics/dpiscale.h"

using namespace CCL;

//************************************************************************************************
// ImagePart
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ImagePart, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImagePart::ImagePart (Image* sourceImage, RectRef _partRect)
: sourceImage (sourceImage),
  partRect (_partRect)
{
	ASSERT (sourceImage != nullptr)
	if(sourceImage)
	{
		ASSERT (sourceImage->getFrameCount () < 2) // frames cannot be handled correctly here!
		sourceImage->retain ();

		// limit part boundaries to source
		Rect limits;
		sourceImage->getSize (limits);
		partRect.bound (limits);
		ASSERT (partRect == _partRect)
	}

	this->size (partRect.getWidth (), partRect.getHeight ());
	this->setIsTemplate (sourceImage->getIsTemplate ());
	this->setIsAdaptive (sourceImage->getIsAdaptive ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImagePart::~ImagePart ()
{
	if(sourceImage)
		sourceImage->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ImagePart::queryInterface (UIDRef iid, void** ptr)
{
	#if 1
	if(iid == ccl_iid<IBitmap> ())
	{
		IBitmap* sourceBitmap = getSourceBitmap ();
		if(sourceBitmap == nullptr)
		{
			*ptr = nullptr;
			return kResultNoInterface;
		}
		QUERY_INTERFACE (IBitmap)
	}
	if(iid == ccl_iid<IMultiResolutionBitmap> ())
	{
		UnknownPtr<IMultiResolutionBitmap> sourceBitmap (getSourceBitmap ());
		if(sourceBitmap == nullptr)
		{
			*ptr = nullptr;
			return kResultNoInterface;
		}
		QUERY_INTERFACE (IMultiResolutionBitmap)
	}
	#endif
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API ImagePart::getType () const
{
	return sourceImage ? sourceImage->getType () : Image::getType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ImagePart::getOriginalImage (Rect& originalRect, bool deep)
{
	originalRect = partRect;
	return sourceImage;
	// NO, don't go deeper here, it would mess up the reported rectangle!
	//return resolveOriginal (sourceImage, originalRect, deep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ImagePart::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	if(!sourceImage)
		return kResultFailed;

	Rect dstRect (0, 0, partRect.getWidth (), partRect.getHeight ());
	dstRect.offset (pos);
	return sourceImage->draw (graphics, partRect, dstRect, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ImagePart::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	if(!sourceImage)
		return kResultFailed;

	RectF partRectF (rectIntToF (partRect));
	RectF dstRect (0, 0, partRectF.getWidth (), partRectF.getHeight ());
	dstRect.offset (pos);
	return sourceImage->draw (graphics, partRectF, dstRect, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ImagePart::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	if(!sourceImage)
		return kResultFailed;

	Rect src2 (src);
	src2.offset (partRect.getLeftTop ());
	return sourceImage->draw (graphics, src2, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ImagePart::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	if(!sourceImage)
		return kResultFailed;

	RectF partRectF (rectIntToF (partRect));
	RectF src2 (src);
	src2.offset (partRectF.getLeftTop ());
	return sourceImage->draw (graphics, src2, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ImagePart::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	if(!sourceImage)
		return kResultFailed;

	Rect src2 (src);
	src2.offset (partRect.getLeftTop ());
	Rect clip2 (clip);
	clip2.offset (partRect.getLeftTop ());
	Rect margins2 (margins);
	margins2.offset (partRect.getLeftTop ());
	return sourceImage->tile (graphics, method, src2, dest, clip2, margins);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap* ImagePart::getSourceBitmap () const
{
	return UnknownPtr<IBitmap> (static_cast<IImage*> (sourceImage));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ImagePart::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	IBitmap* sourceBitmap = getSourceBitmap ();
	ASSERT (sourceBitmap != nullptr)
	if(!sourceBitmap)
		return kResultFailed;

	BitmapLockData srcData;
	tresult tr = sourceBitmap->lockBits (srcData, format, mode);
	if(tr != kResultOk)
		return tr;

	PixelRect pixelRect (partRect, sourceBitmap->getContentScaleFactor ());
	ASSERT (srcData.width >= pixelRect.right)
	ASSERT (srcData.height >= pixelRect.bottom)

	data.width = pixelRect.getWidth ();
	data.height = pixelRect.getHeight ();
	data.format = srcData.format;
	data.scan0 = (char*)srcData.getScanline (pixelRect.top) + (pixelRect.left * srcData.getBytesPerPixel ());
	data.rowBytes = srcData.rowBytes;
	data.bitsPerPixel = srcData.bitsPerPixel;
	data.mode = mode;
	data.nativeData = NEW BitmapLockData (srcData); // need to keep original for unlock!
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ImagePart::unlockBits (BitmapLockData& data)
{
	IBitmap* sourceBitmap = getSourceBitmap ();
	ASSERT (sourceBitmap != nullptr)
	if(!sourceBitmap)
		return kResultFailed;

	BitmapLockData* srcData = reinterpret_cast<BitmapLockData*> (data.nativeData);
	ASSERT (srcData != nullptr)
	if(srcData == nullptr)
		return kResultInvalidArgument;

	tresult tr = sourceBitmap->unlockBits (*srcData);
	delete srcData;
	data.nativeData = nullptr;
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ImagePart::scrollPixelRect (const Rect& rect, const Point& delta)
{
	IBitmap* sourceBitmap = getSourceBitmap ();
	ASSERT (sourceBitmap != nullptr)
	if(!sourceBitmap)
		return kResultFailed;

	Rect rect2 (rect);
	rect2.offset (partRect.getLeftTop ());

	Point delta2 (delta);
	delta2.offset (partRect.getLeftTop ());

	return sourceBitmap->scrollPixelRect (rect2, delta2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point CCL_API ImagePart::getPixelSize () const
{
	PixelPoint sizeInPixel (size, getContentScaleFactor ());
	return sizeInPixel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap::PixelFormat CCL_API ImagePart::getPixelFormat () const
{
	IBitmap* sourceBitmap = getSourceBitmap ();
	ASSERT (sourceBitmap != nullptr)
	return sourceBitmap ? sourceBitmap->getPixelFormat () : kAny;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API ImagePart::getContentScaleFactor () const
{
	IBitmap* sourceBitmap = getSourceBitmap ();
	ASSERT (sourceBitmap != nullptr)
	return sourceBitmap ? sourceBitmap->getContentScaleFactor () : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ImagePart::getRepresentationCount () const
{
	UnknownPtr<IMultiResolutionBitmap> sourceBitmap (getSourceBitmap ());
	ASSERT (sourceBitmap != nullptr)
	return sourceBitmap ? sourceBitmap->getRepresentationCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImagePart::setCurrentRepresentation (int index)
{
	UnknownPtr<IMultiResolutionBitmap> sourceBitmap (getSourceBitmap ());
	ASSERT (sourceBitmap != nullptr)
	if(sourceBitmap)
		sourceBitmap->setCurrentRepresentation (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ImagePart::getCurrentRepresentation () const
{
	UnknownPtr<IMultiResolutionBitmap> sourceBitmap (getSourceBitmap ());
	ASSERT (sourceBitmap != nullptr)
	return sourceBitmap ? sourceBitmap->getCurrentRepresentation () : 0;
}
