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
// Filename    : ccl/app/utilities/imagefile.cpp
// Description : Image File
//
//************************************************************************************************

#include "ccl/app/utilities/imagefile.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"

using namespace CCL;

//************************************************************************************************
// ImageFile
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ImageFile, kPNG, "image/png")
DEFINE_STRINGID_MEMBER_ (ImageFile, kJPEG, "image/jpeg")
DEFINE_STRINGID_MEMBER_ (ImageFile, kWebP, "image/webp")
DEFINE_STRINGID_MEMBER_ (ImageFile, kIconSet, CCL_MIME_TYPE "-iconset")

//////////////////////////////////////////////////////////////////////////////////////////////////

int ImageFile::getNumImageFormats ()
{
	return GraphicsFactory::getNumImageFormats ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& ImageFile::getImageFormat (int index)
{
	const FileType* format = GraphicsFactory::getImageFormat (index);
	ASSERT (format != nullptr)
	static FileType emptyFormat;
	return format ? *format : emptyFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* ImageFile::getFormatByMimeType (StringID _mimeType)
{
#if 1 // use registry to include hidden image formats
	return System::GetFileTypeRegistry ().getFileTypeByMimeType (String (_mimeType));
#else
	String mimeType (_mimeType);
	int count = getNumImageFormats ();
	for(int i = 0; i < count; i++)
	{
		const FileType& format = getImageFormat (i);
		if(format.getMimeType () == mimeType)
			return &format;
	}
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageFile::canLoadImage (UrlRef path)
{
	int count = getNumImageFormats ();
	for(int i = 0; i < count; i++)
	{
		const FileType& format = getImageFormat (i);
		if(path.getFileType () == format)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageFile::loadImage (UrlRef path)
{
	return GraphicsFactory::loadImageFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageFile::loadImage (IStream& stream, const FileType& format)
{
	return GraphicsFactory::loadImageStream (stream, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ImageFile, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageFile::ImageFile (StringID mimeType, IImage* image)
{
	setFormat (mimeType);
	setImage (image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ImageFile::queryInterface (UIDRef iid, void** ptr)
{
	// delegate IImage interface
	if(iid == ccl_iid<IImage> () && image)
		return image->queryInterface (iid, ptr);
	
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageFile::setFormat (StringID mimeType)
{
	const FileType* format = getFormatByMimeType (mimeType);
	ASSERT (format != nullptr)
	if(format)
		setFormat (*format);
	else
		setFormat (FileType ());
	return format != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageFile::setFormat (const FileType& _format)
{
	format = _format;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageFile::getFormat (FileType& format) const
{
	format = this->format;
	return image != nullptr; // if image is null, save() should not be called!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageFile::save (IStream& stream) const
{
	ASSERT (image != nullptr)
	if(image == nullptr)
		return false;
	return GraphicsFactory::saveImageStream (stream, image, format, encoderOptions) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageFile::load (IStream& stream)
{
	AutoPtr<IImage> image = GraphicsFactory::loadImageStream (stream, format);
	ASSERT (image != nullptr)
	setImage (image);
	return image != nullptr;
}
