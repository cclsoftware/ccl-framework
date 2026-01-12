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
// Filename    : ccl/gui/graphics/imaging/bitmap.cpp
// Description : Bitmap class
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/codecs/pngcodec.h"
#include "ccl/gui/graphics/imaging/codecs/webpcodec.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap File Types
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace FileTypes 
{
	FileType bmp (nullptr, "bmp", "image/bmp");
	FileType png (nullptr, "png", "image/png");
	FileType jpg (nullptr, "jpg", "image/jpeg");
	FileType jpeg (nullptr, "jpeg", "image/jpeg");
	FileType gif (nullptr, "gif", "image/gif");

	static const String kHiResExtension ("@2x");
	static const String kExtraHiResExtension ("@3x");

	static const Vector<const FileType*>& getBitmapTypes ()
	{
		static Vector<const FileType*> bitmapTypes;		
		static bool bitmapTypesInitialized = false;
		if(!bitmapTypesInitialized)
		{
			bitmapTypesInitialized = true;

			// PNG and JPEG should be available on all platforms
			#if CCL_PLATFORM_ANDROID
			CustomBitmapCodecs::instance ().addCodec (NEW PNGBitmapCodec);
			#else
			bitmapTypes.add (&png);
			#endif
			bitmapTypes.add (&jpg);
			bitmapTypes.add (&jpeg);
			
			#if CCL_PLATFORM_WINDOWS
			bitmapTypes.add (&bmp);
			bitmapTypes.add (&gif);
			#endif

			CustomBitmapCodecs::instance ().addCodec (NEW WebPBitmapCodec);
			CustomBitmapCodecs::instance ().collectFileTypes (bitmapTypes);
		}
		return bitmapTypes;
	}
}

//************************************************************************************************
// BitmapHandler
//************************************************************************************************

class BitmapHandler: public ImageHandler
{
public:
	BitmapHandler ()
	: resolutionMode (Bitmap::kStandardResolution)
	{
		// NOTE: Naming mode is initialized per platform, see UserInterface::startupPlatform().
	}

	PROPERTY_VARIABLE (Bitmap::ResolutionNamingMode, resolutionMode, ResolutionNamingMode)
	
	Image* loadHighResolutionImage (UrlRef path, int factor) const
	{		
		String fileName;
		path.getName (fileName, false);
		if(factor == 3)
			fileName << FileTypes::kExtraHiResExtension << CCLSTR (".");
		else
			fileName << FileTypes::kHiResExtension << CCLSTR (".");

		const FileType& fileType = path.getFileType ();
		fileName.append (fileType.getExtension ());
		
		Url path2 (path);
		path2.ascend ();
		path2.descend (fileName);

		Bitmap* bitmap = nullptr;
		if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path2, IStream::kOpenMode))
			if(bitmap = (Bitmap*)loadImage (*stream, fileType))
			{
				// adjust scale factor
				NativeBitmap* nativeBitmap = bitmap->getNativeBitmap ();
				nativeBitmap->setContentScaleFactor ((float)factor);
				bitmap->assign (nativeBitmap);
			}

		return bitmap;
	}

	// ImageHandler
	bool canHandleImage (const FileType& t) const override
	{
		for(auto type : FileTypes::getBitmapTypes ())
			if(t == *type)
				return true;

		return false;
	}

	Image* loadImage (UrlRef path) const override
	{
		if(resolutionMode != Bitmap::kStandardResolution)
		{
			if(resolutionMode == Bitmap::kMultiResolution) // standard and high (1x/2x)
			{
				AutoPtr<Bitmap> bitmap2x = (Bitmap*)loadHighResolutionImage (path, 2);
				AutoPtr<Bitmap> bitmap1x = (Bitmap*)ImageHandler::loadImage (path);
			
				if(bitmap1x && bitmap2x)
					return NEW MultiResolutionBitmap (bitmap1x->getNativeBitmap (), bitmap2x->getNativeBitmap ());
				else
					return bitmap2x ? bitmap2x.detach () : bitmap1x.detach ();
			}
			else // high (2x) or extra high (3x)
			{
				Bitmap* bitmap = nullptr;
				switch(resolutionMode)
				{
				case Bitmap::kExtraHighResolution :
					if(bitmap = (Bitmap*)loadHighResolutionImage (path, 3))
						return bitmap;
					// through
				case Bitmap::kHighResolution :
					if(bitmap = (Bitmap*)loadHighResolutionImage (path, 2))
						return bitmap;
					// through, see below
				}
			}
		}

		// fallback to standard resolution (1x)
		return ImageHandler::loadImage (path);
	}

	Image* loadImage (IStream& stream, const FileType& type) const override
	{
		Bitmap* image = NEW Bitmap (stream, type);
		if(!image->isValid ())
		{
			image->release ();
			return nullptr;
		}
		return image;
	}

	int getNumFileTypes () const override
	{
		return FileTypes::getBitmapTypes ().count ();
	}

	const FileType* getFileType (int index) const override
	{
		return index < FileTypes::getBitmapTypes ().count () ? FileTypes::getBitmapTypes ().at (index) : nullptr;
	}

	bool saveImage (IStream& stream, Image* image, const FileType& type,
					const IAttributeList* encoderOptions = nullptr) const override
	{
		Bitmap* bitmap = ccl_cast<Bitmap> (image);
		return bitmap ? bitmap->saveToStream (stream, type, encoderOptions) : false;
	}
};

static BitmapHandler bitmapHandler;

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (BitmapFile, "Bitmap File")
	XSTRING (PNGFile, "PNG File")
	XSTRING (JPGFile, "JPEG File")
	XSTRING (GIFFile, "GIF File")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (BitmapHandler, kFrameworkLevelFirst)
{
	Image::registerHandler (&bitmapHandler);
	return true;
}

CCL_KERNEL_INIT_LEVEL (BitmapFileTypes, kFrameworkLevelLast)
{
	FileTypes::init (FileTypes::bmp, XSTR (BitmapFile));
	FileTypes::init (FileTypes::png, XSTR (PNGFile));
	FileTypes::init (FileTypes::jpg, XSTR (JPGFile));
	FileTypes::init (FileTypes::jpeg, XSTR (JPGFile));
	FileTypes::init (FileTypes::gif, XSTR (GIFFile));

	for(auto type : FileTypes::getBitmapTypes ())
		System::GetFileTypeRegistry ().registerFileType (*type);
	return true;
}

//************************************************************************************************
// Bitmap
//************************************************************************************************

void Bitmap::setResolutionNamingMode (ResolutionNamingMode mode)
{
	bitmapHandler.setResolutionNamingMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Bitmap::getDefaultContentScaleFactor ()
{
	switch(bitmapHandler.getResolutionNamingMode ())
	{
	case kMultiResolution :
	case kHighResolution : return 2.f;
	case kExtraHighResolution : return 3.f;
	default : // kStandardResolution
		return 1.f; 
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::isHighResolutionFile (UrlRef path)
{
	String fileName;
	path.getName (fileName, false);
	
	return fileName.endsWith (FileTypes::kHiResExtension) ||
		   fileName.endsWith (FileTypes::kExtraHiResExtension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* Bitmap::getOriginalBitmap (Rect& originalRect, Image* image, bool deep)
{
	return image ? ccl_cast<Bitmap> (image->getOriginalImage (originalRect, deep)) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Bitmap, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap ()
: nativeBitmap (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (int width, int height, PixelFormat format, float contentScaleFactor)
: nativeBitmap (nullptr)
{
	assign (NativeGraphicsEngine::instance ().createBitmap (width, height, format, contentScaleFactor));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (NativeBitmap* nativeBitmap)
: nativeBitmap (nullptr)
{
	assign (nativeBitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (IStream& stream, const FileType& format)
: nativeBitmap (nullptr)
{
	if(auto bmp = NativeGraphicsEngine::instance ().loadBitmap (stream, format))
		assign (bmp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (const Bitmap&)
: nativeBitmap (nullptr) 
{
	ASSERT (0)
	// TODO!!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::~Bitmap ()
{
	if(nativeBitmap)
		nativeBitmap->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Bitmap::assign (NativeBitmap* _nativeBitmap)
{
	nativeBitmap = _nativeBitmap;
	ASSERT (nativeBitmap != nullptr)

	if(nativeBitmap)
		size (nativeBitmap->getWidth (), nativeBitmap->getHeight ());
	else
		size (0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* Bitmap::getNativeBitmap ()
{
	return nativeBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::isValid () const
{
	return nativeBitmap != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::saveToStream (IStream& stream, const FileType& format,
						   const IAttributeList* encoderOptions) const
{
	ASSERT (nativeBitmap != nullptr)
	if(!nativeBitmap)
		return false;
	return NativeGraphicsEngine::instance ().saveBitmap (stream, *nativeBitmap, format, encoderOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Bitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	ASSERT (nativeBitmap != nullptr)
	if(!nativeBitmap)
		return kResultUnexpected;
	return nativeBitmap->lockBits (data, format, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Bitmap::unlockBits (BitmapLockData& data)
{
	ASSERT (nativeBitmap != nullptr)
	if(!nativeBitmap)
		return kResultUnexpected;
	return nativeBitmap->unlockBits (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Bitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	ASSERT (nativeBitmap != nullptr)
	if(!nativeBitmap)
		return kResultUnexpected;
	return nativeBitmap->scrollPixelRect (rect, delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point CCL_API Bitmap::getPixelSize () const
{
	ASSERT (nativeBitmap != nullptr)
	return nativeBitmap ? nativeBitmap->getPixelSize () : Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap::PixelFormat CCL_API Bitmap::getPixelFormat () const
{
	ASSERT (nativeBitmap != nullptr)
	return nativeBitmap ? nativeBitmap->getPixelFormat () : kAny;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API Bitmap::getContentScaleFactor () const
{
	ASSERT (nativeBitmap != nullptr)
	return nativeBitmap ? nativeBitmap->getContentScaleFactor () : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API Bitmap::getType () const
{
	return kBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Bitmap::getFrameCount () const
{
	ASSERT (nativeBitmap != nullptr)
	return nativeBitmap ? nativeBitmap->getFrameCount () : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Bitmap::getCurrentFrame () const
{
	ASSERT (nativeBitmap != nullptr)
	return nativeBitmap ? nativeBitmap->getCurrentFrame () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Bitmap::setCurrentFrame (int frameIndex)
{
	ASSERT (nativeBitmap != nullptr)
	if(nativeBitmap)
		nativeBitmap->setCurrentFrame (frameIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Bitmap::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	ASSERT (nativeBitmap != nullptr && graphics.getNativeDevice () != nullptr)
	if(nativeBitmap && graphics.getNativeDevice ())
		return nativeBitmap->draw (*graphics.getNativeDevice (), pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Bitmap::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	ASSERT (nativeBitmap != nullptr && graphics.getNativeDevice () != nullptr)
	if(nativeBitmap && graphics.getNativeDevice ())
		return nativeBitmap->draw (*graphics.getNativeDevice (), pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Bitmap::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	ASSERT (nativeBitmap != nullptr && graphics.getNativeDevice () != nullptr)
	if(nativeBitmap && graphics.getNativeDevice ())
		return nativeBitmap->draw (*graphics.getNativeDevice (), src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Bitmap::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	ASSERT (nativeBitmap != nullptr && graphics.getNativeDevice () != nullptr)
	if(nativeBitmap && graphics.getNativeDevice ())
		return nativeBitmap->draw (*graphics.getNativeDevice (), src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Bitmap::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	ASSERT (nativeBitmap != nullptr && graphics.getNativeDevice () != nullptr)
	if(nativeBitmap && graphics.getNativeDevice ())
		return nativeBitmap->tile (*graphics.getNativeDevice (), method, src, dest, clip, margins);
	return kResultFalse;
}

//************************************************************************************************
// MultiResolutionBitmap
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiResolutionBitmap, Bitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiResolutionBitmap::MultiResolutionBitmap (NativeBitmap* bitmap1x, NativeBitmap* bitmap2x)
: Bitmap (bitmap1x),
  nativeBitmap2 (bitmap2x),
  currentRepresentation (0)
{
	ASSERT (bitmap1x != nullptr && bitmap2x != nullptr)
	bitmap1x->retain ();
	bitmap2x->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiResolutionBitmap::MultiResolutionBitmap (int width, int height, PixelFormat format)
: Bitmap (NativeGraphicsEngine::instance ().createBitmap (width, height, format)),
  nativeBitmap2 (NativeGraphicsEngine::instance ().createBitmap (width, height, format, 2.f)),
  currentRepresentation (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiResolutionBitmap::~MultiResolutionBitmap ()
{
	nativeBitmap2->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* MultiResolutionBitmap::getNativeBitmap2x ()
{
	return nativeBitmap2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiResolutionBitmap::getRepresentationCount () const
{
	return 2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiResolutionBitmap::setCurrentRepresentation (int index)
{
	currentRepresentation = index > 0 ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiResolutionBitmap::getCurrentRepresentation () const
{
	return currentRepresentation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* MultiResolutionBitmap::getCurrentBitmap () const
{
	return currentRepresentation == 1 ? nativeBitmap2 : nativeBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MultiResolutionBitmap::saveToStream (IStream& stream, const FileType& format,
										  const IAttributeList* encoderOptions) const
{
	return NativeGraphicsEngine::instance ().saveBitmap (stream, *getCurrentBitmap (), format, encoderOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MultiResolutionBitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	return getCurrentBitmap ()->lockBits (data, format, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MultiResolutionBitmap::unlockBits (BitmapLockData& data)
{
	return getCurrentBitmap ()->unlockBits (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MultiResolutionBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	getCurrentBitmap ()->scrollPixelRect (rect, delta);
	if(currentRepresentation == 0)
	{
		PixelRect rect2 (rect, 2);
		PixelPoint delta2 (delta, 2);
		nativeBitmap2->scrollPixelRect (rect2, delta2);
	}
	else
	{
		PixelRect rect2 (rect, 0.5);
		PixelPoint delta2 (delta, 0.5);
		nativeBitmap->scrollPixelRect (rect2, delta2);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point CCL_API MultiResolutionBitmap::getPixelSize () const
{
	return getCurrentBitmap ()->getPixelSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap::PixelFormat CCL_API MultiResolutionBitmap::getPixelFormat () const
{
	return getCurrentBitmap ()->getPixelFormat ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API MultiResolutionBitmap::getContentScaleFactor () const
{
	return getCurrentBitmap ()->getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* MultiResolutionBitmap::selectBitmap (NativeGraphicsDevice* graphics)
{
	ASSERT (graphics != nullptr)
	if(graphics == nullptr)
		return nullptr;
	return isHighResolutionScaling (graphics->getContentScaleFactor ()) ? nativeBitmap2 : nativeBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiResolutionBitmap::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	if(NativeBitmap* selectedBitmap = selectBitmap (graphics.getNativeDevice ()))
		return selectedBitmap->draw (*graphics.getNativeDevice (), pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiResolutionBitmap::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	if(NativeBitmap* selectedBitmap = selectBitmap (graphics.getNativeDevice ()))
		return selectedBitmap->draw (*graphics.getNativeDevice (), pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiResolutionBitmap::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	if(NativeBitmap* selectedBitmap = selectBitmap (graphics.getNativeDevice ()))
		return selectedBitmap->draw (*graphics.getNativeDevice (), src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiResolutionBitmap::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	if(NativeBitmap* selectedBitmap = selectBitmap (graphics.getNativeDevice ()))
		return selectedBitmap->draw (*graphics.getNativeDevice (), src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiResolutionBitmap::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	if(NativeBitmap* selectedBitmap = selectBitmap (graphics.getNativeDevice ()))
		return selectedBitmap->tile (*graphics.getNativeDevice (), method, src, dest, clip, margins);
	return kResultFalse;
}
