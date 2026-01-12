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
// Filename    : ccl/gui/graphics/imaging/image.cpp
// Description : Image class
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ImageHandler
//************************************************************************************************

Image* ImageHandler::loadImage (UrlRef path) const
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	return stream ? loadImage (*stream, path.getFileType ()) : nullptr;
}

//************************************************************************************************
// Image
//************************************************************************************************

Container& Image::getHandlerList ()
{
	static ObjectList imageHandlers;
	return imageHandlers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Image::registerHandler (ImageHandler* handler)
{
	getHandlerList ().addOnce (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ImageHandler* Image::findHandler (const FileType& format)
{
	ForEach (getHandlerList (), ImageHandler, handler)
		if(handler->canHandleImage (format))
			return handler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Image::loadImage (UrlRef url)
{
	if(const ImageHandler* handler = findHandler (url.getFileType ()))
		return handler->loadImage (url);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Image::loadImage (IStream& stream, const FileType& format)
{
	if(const ImageHandler* handler = findHandler (format))
		return handler->loadImage (stream, format);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Image::saveImage (IStream& stream, Image* image, const FileType& format,
					  const IAttributeList* encoderOptions)
{
	if(const ImageHandler* handler = findHandler (format))
		return handler->saveImage (stream, image, format, encoderOptions);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (Image, Object)
DEFINE_CLASS_NAMESPACE (Image, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::Image ()
: isTemplate (false),
  isAdaptive (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API Image::getType () const
{
	return kScalable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Image::getWidth () const
{
	return size.x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Image::getHeight () const
{
	return size.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Image::getFrameCount () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Image::getCurrentFrame () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Image::setCurrentFrame (int frameIndex)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Image::getFrameIndex (StringID name) const
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API Image::getOriginal ()
{
	Rect unused;
	return getOriginalImage (unused);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Image::getOriginalImage (Rect& originalRect, bool deep)
{
	getSize (originalRect);
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/Image* Image::resolveOriginal (Image* image, Rect& originalRect, bool deep)
{
	if(deep && image)
	{
		// LATER TODO: size is not bound when going deeper!
		return image->getOriginalImage (originalRect, true);
	}
	else
		return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Image::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "width")			{ var = getWidth (); return true; }
	if(propertyId == "height")			{ var = getHeight (); return true; }
	if(propertyId == "frameCount")		{ var = getFrameCount (); return true; }
	if(propertyId == "currentFrame")	{ var = getCurrentFrame (); return true; }
	if(propertyId == kIsTemplate)		{ var = getIsTemplate (); return true; }
	if(propertyId == kIsAdaptive)		{ var = getIsAdaptive (); return true; }

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Image::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "currentFrame")	{ setCurrentFrame (var); return true; }
	if(propertyId == kIsTemplate)		{ setIsTemplate (var); return true; }
	if(propertyId == kIsAdaptive)		{ setIsAdaptive (var); return true; }

	return SuperClass::setProperty (propertyId, var);
}
