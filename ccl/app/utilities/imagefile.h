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
// Filename    : ccl/app/utilities/imagefile.h
// Description : Image File
//
//************************************************************************************************

#ifndef _ccl_imagefile_h
#define _ccl_imagefile_h

#include "ccl/public/storage/filetype.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/base/storage/storableobject.h"

namespace CCL {

//************************************************************************************************
// ImageFile
//************************************************************************************************

class ImageFile: public StorableObject
{
public:
	DECLARE_CLASS (ImageFile, StorableObject)

	ImageFile (StringID mimeType = kPNG, IImage* image = nullptr);

	DECLARE_STRINGID_MEMBER (kPNG)
	DECLARE_STRINGID_MEMBER (kJPEG)
	DECLARE_STRINGID_MEMBER (kWebP)
	DECLARE_STRINGID_MEMBER (kIconSet)

	static int getNumImageFormats ();
	static const FileType& getImageFormat (int index);
	static const FileType* getFormatByMimeType (StringID mimeType);	

	static bool canLoadImage (UrlRef path);
	static IImage* loadImage (UrlRef path);
	static IImage* loadImage (IStream& stream, const FileType& format);

	bool setFormat (StringID mimeType);
	void setFormat (const FileType& format);

	PROPERTY_SHARED_AUTO (IAttributeList, encoderOptions, EncoderOptions)
	PROPERTY_SHARED_AUTO (IImage, image, Image)

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

	CLASS_INTERFACES (StorableObject)

protected:
	FileType format;
};

} // namespace CCL

#endif // _ccl_imagefile_h
