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
// Filename    : ccl/gui/graphics/imaging/image.h
// Description : Image class
//
//************************************************************************************************

#ifndef _ccl_image_h
#define _ccl_image_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class FileType;
interface IStream;
interface IAttributeList;

class Container;
class ImageHandler;
class GraphicsDevice;

//************************************************************************************************
// Image
//************************************************************************************************

class Image: public Object,
			 public IImage
{
public:
	DECLARE_CLASS_ABSTRACT (Image, Object)

	Image ();

	// IImage
	ImageType CCL_API getType () const override;
	int CCL_API getWidth () const override;
	int CCL_API getHeight () const override;
	int CCL_API getFrameCount () const override;
	int CCL_API getCurrentFrame () const override;
	void CCL_API setCurrentFrame (int frameIndex) override;
	int CCL_API getFrameIndex (StringID name) const override;
	IImage* CCL_API getOriginal () override;

	// Internal methods
	Rect& getSize (Rect&) const;
	const Point& getSize () const;
	PROPERTY_VARIABLE (bool, isTemplate, IsTemplate)
	PROPERTY_VARIABLE (bool, isAdaptive, IsAdaptive)

	virtual Image* getOriginalImage (Rect& originalRect, bool deep = false); ///< resolve original image (could be this)

	// Drawing
	virtual tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) = 0;
	virtual tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) = 0;
	virtual tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) = 0;

	CLASS_INTERFACE (IImage, Object)

	// Image Format Handling
	static Image* loadImage (UrlRef path);
	static Image* loadImage (IStream& stream, const FileType& format);
	static bool saveImage (IStream& stream, Image* image, const FileType& format, const IAttributeList* encoderOptions = nullptr);
	static void registerHandler (ImageHandler* handler);
	static const ImageHandler* findHandler (const FileType& format);
	static Container& getHandlerList ();

protected:
	Point size;

	static Image* resolveOriginal (Image* image, Rect& originalRect, bool deep);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ImageHandler
//************************************************************************************************

class ImageHandler: public Object
{
public:
	virtual bool canHandleImage (const FileType& type) const = 0;
	virtual Image* loadImage (IStream& stream, const FileType& type) const = 0;
	virtual Image* loadImage (UrlRef path) const;
	virtual int getNumFileTypes () const = 0;
	virtual const FileType* getFileType (int index) const = 0;
	virtual bool saveImage (IStream& stream, Image* image, const FileType& type, 
							const IAttributeList* encoderOptions = nullptr) const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Image inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Rect& Image::getSize (Rect& r) const { r (0, 0, size.x, size.y); return r; }
inline const Point& Image::getSize () const { return size; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_image_h
