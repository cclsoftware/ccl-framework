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
// Filename    : ccl/gui/graphics/imaging/multiimage.cpp
// Description : MultiImage class
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/graphics/igraphics.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace FileTypes 
{
	FileType iconset (nullptr, "iconset", CCL_MIME_TYPE "-iconset");
}

//************************************************************************************************
// IconSetFormat2
//************************************************************************************************

const CStringPtr IconSetFormat2::fileNamePattern = "icon_%dx%d.png";
const CStringPtr IconSetFormat2::fileNamePatternHiRes = "icon_%dx%d@2x.png";

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IconSetFormat2::isValidIconSize (int size)
{
	for(int i = 0; i < kIconSizesAll; i++)
		if(size == getIconSizeAt (i).size)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IconSetFormat2::makeIconName (String& fileName, const IconSize& iconSize)
{
	fileName = String (MutableCString ().appendFormat (fileNamePattern, iconSize.size, iconSize.size));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IconSetFormat2::makeIconName (String& fileName, const Image* image, float scaleFactor)
{
	ASSERT (image->getWidth () == image->getHeight () && isValidIconSize (image->getWidth ()))
	ASSERT (scaleFactor == 1.f || scaleFactor == 2.f)
	CStringPtr pattern = scaleFactor == 2.f ? fileNamePatternHiRes : fileNamePattern;
	fileName = String (MutableCString ().appendFormat (pattern, image->getWidth (), image->getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IconSetFormat2::isValidIconName (StringRef fileName)
{
	int w, h;
	return ::sscanf (MutableCString (fileName), fileNamePattern, &w, &h) == 2 && w == h;
}

//************************************************************************************************
// ImageWriter (writes image to package file)
//************************************************************************************************

class ImageWriter: public Object, 
				   public IPackageItemWriter
{
public:
	ImageWriter (Image* image, int representation)
	: image (image),
	  representation (representation)
	{}

	static void addToPackage (IPackageFile* pf, StringRef fileName, Image* image, int representation = -1)
	{
		Url url;
		url.setPath (fileName);
		pf->createItem (url, NEW ImageWriter (image, representation));
	}

	// IPackageItemWriter
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress = nullptr) override
	{
		bool result = false;
		if(MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (image))
		{
			IMultiResolutionBitmap::RepSelector selector (multiBitmap, representation);
			result = Image::saveImage (dstStream, multiBitmap, FileTypes::png);
		}
		else
			result = Image::saveImage (dstStream, image, FileTypes::png);
		return result ? kResultOk : kResultFailed;
	}

	CLASS_INTERFACE (IPackageItemWriter, Object)

protected:
	Image* image;
	int representation;
};

//************************************************************************************************
// IconSetHandler
//************************************************************************************************

class IconSetHandler: public ImageHandler
{
public:
	bool canHandleImage (const FileType& type) const override
	{
		return type == FileTypes::iconset;
	}
	
	Image* loadImage (IStream& stream, const FileType& type) const override
	{
		AutoPtr<IStream> seekableStream = System::GetFileUtilities ().createSeekableStream (stream);
		AutoPtr<IPackageFile> pf = System::GetPackageHandler ().openPackageWithStream (*seekableStream);
		if(!pf)
			return nullptr;
		
		const String kTempID (String () << "~iconset" << System::GetThreadSelfID ());
		tresult tr = System::GetPackageHandler ().mountPackageVolume (pf, kTempID, IPackageVolume::kHidden);
		ASSERT (tr == kResultOk)
		if(tr != kResultOk)
			return nullptr;

		MultiImage* image = NEW MultiImage;
		for(int i = 0; i < IconSetFormat::kIconSizesAll; i++)
		{
			const IconSetFormat::IconSize& iconSize = IconSetFormat::getIconSizeAt (i);

			String fileName;
			IconSetFormat2::makeIconName (fileName, iconSize);

			PackageUrl path (kTempID, fileName);
			AutoPtr<Image> frame = Image::loadImage (path);
			if(frame)
				image->addFrame (frame, iconSize.name);
		}

		System::GetPackageHandler ().unmountPackageVolume (pf);
		return image;
	}

	int getNumFileTypes () const override
	{
		return 0; // not a public file type!
	}
	
	const FileType* getFileType (int index) const override
	{
		return nullptr;
	}

	bool saveImage (IStream& stream, Image* image, const FileType& type,
					const IAttributeList* encoderOptions = nullptr) const override
	{
		AutoPtr<IPackageFile> pf = System::GetPackageHandler ().createPackageWithStream (stream, ClassID::ZipFile);
		if(!pf)
			return false;

		pf->setOption (PackageOption::kCompressed, true);

		auto getScaleFactor = [] (Image* image)
		{
			Bitmap* bitmap = ccl_cast<Bitmap> (image);
			return bitmap ? bitmap->getContentScaleFactor () : 1.f;
		};

		// collect all frames (and representations) to be saved
		if(MultiImage* multiImage = ccl_cast<MultiImage> (image))
		{
			for(int frameIndex = 0; frameIndex < multiImage->getFrameCount (); frameIndex++)
			{
				Image* frameImage = multiImage->getFrame (frameIndex);
				if(MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (frameImage))
				{
					for(int i = 0; i < multiBitmap->getRepresentationCount (); i++)
					{
						IMultiResolutionBitmap::RepSelector selector (multiBitmap, i);
						
						String fileName;
						IconSetFormat2::makeIconName (fileName, frameImage, getScaleFactor (frameImage));
						ImageWriter::addToPackage (pf, fileName, frameImage, i);
					}
				}
				else
				{
					String fileName;
					IconSetFormat2::makeIconName (fileName, frameImage, getScaleFactor (frameImage));
					ImageWriter::addToPackage (pf, fileName, frameImage);
				}
			}
		}
		else if(image)
		{
			String fileName;
			IconSetFormat2::makeIconName (fileName, image, getScaleFactor (image));
			ImageWriter::addToPackage (pf, fileName, image);
		}

		return pf->flush () != 0;
	}
};

//************************************************************************************************
// MultiImage::FrameEntry
//************************************************************************************************

class MultiImage::FrameEntry: public Object
{
public:
	DECLARE_CLASS (FrameEntry, Object)

	FrameEntry (Image* image = nullptr, StringID name = nullptr)
	: name (name)
	{ setImage (image); }

	Image* getImage () const		{ return image; }
	StringID getName () const		{ return name; }
	void setImage (Image* _image)	{ image.share (_image); }

protected:
	AutoPtr<Image> image;
	MutableCString name;
};

DEFINE_CLASS_HIDDEN (MultiImage::FrameEntry, Object)

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (IconFile, "Icon File")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (IconSetHandler, kFrameworkLevelFirst)
{
	static IconSetHandler theHandler;
	Image::registerHandler (&theHandler);
	return true;
}

CCL_KERNEL_INIT_LEVEL (IconSetFileType, kFrameworkLevelLast)
{
	FileTypes::init (FileTypes::iconset, XSTR (IconFile));
	System::GetFileTypeRegistry ().registerFileType (FileTypes::iconset);
	return true;
}

//************************************************************************************************
// MultiImage
//************************************************************************************************

DEFINE_CLASS (MultiImage, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiImage::MultiImage ()
: currentFrame (-1)
{
	frames.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiImage::addFrame (Image* image, StringID name)
{
	frames.add (NEW FrameEntry (image, name));
	if(currentFrame == -1)
		setCurrentFrame (frames.count () - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Image* MultiImage::getFrame (int frameIndex)
{
	FrameEntry* entry = (FrameEntry*)frames.at (frameIndex);
	return entry ? entry->getImage () : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API MultiImage::getType () const
{ 
	return kMultiple;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiImage::getFrameCount () const
{
	return frames.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiImage::getCurrentFrame () const
{
	return currentFrame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiImage::setCurrentFrame (int frameIndex)
{
	frameIndex = ccl_bound<int> (frameIndex, 0, getFrameCount () - 1);
	if(frameIndex != currentFrame)
	{
		currentFrame = frameIndex;
		Image* frameImage = getFrame (currentFrame);
		if(frameImage)
			size = frameImage->getSize ();
		else
			size (0, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiImage::getFrameIndex (StringID name) const
{
	int i = 0;
	ForEach (frames, FrameEntry, entry)
		if(name == entry->getName ())
			return i;
		i++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* MultiImage::getOriginalImage (Rect& originalRect, bool deep)
{
	getSize (originalRect);
	return resolveOriginal (getFrame (currentFrame), originalRect, deep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiImage::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	Image* image = getFrame (currentFrame);
	if(image)
		return image->draw (graphics, pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiImage::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	Image* image = getFrame (currentFrame);
	if(image)
		return image->draw (graphics, pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiImage::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	Image* image = getFrame (currentFrame);
	if(image)
		return image->draw (graphics, src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiImage::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	Image* image = getFrame (currentFrame);
	if(image)
		return image->draw (graphics, src, dst, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MultiImage::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins)
{
	Image* image = getFrame (currentFrame);
	if(image)
		return image->tile (graphics, method, src, dst, clip, margins);
	return kResultFalse;
}

//************************************************************************************************
// ImageResolutionSelector
//************************************************************************************************

inline int compareSize      (PointRef size1, PointRef size2) {	return size1.y - size2.y; }
inline int compareOtherSize (PointRef size1, PointRef size2) {	return size1.x - size2.x; }

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageResolutionSelector::selectImage (IImage* image, PointRef destSize, int flags)
{
	return selectImage (unknown_cast<Image> (image), destSize, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ImageResolutionSelector::selectImage (Image* image, PointRef destSize, int flags)
{
	MultiImage* multiImage = ccl_cast<MultiImage> (image);
	if(multiImage)
	{
		Image* bestSmaller = nullptr;
		Image* bestLarger  = nullptr;
		Coord smallerDiff = kMaxCoord;
		Coord largerDiff  = kMaxCoord;

		int numFrames = multiImage->getFrameCount ();
		for(int i = 0; i < numFrames; i++)
		{
			Image* frame = multiImage->getFrame (i);
			if(frame)
			{
				Point frameSize (frame->getSize ());
				int cmp = compareSize (frameSize, destSize);
				if(cmp == 0)
				{
					if(compareOtherSize (frameSize, destSize) == 0)
					{
						return frame; // found frame with the exact size
					}
					else // only one direction matches, continue
					{
						bestSmaller = bestLarger = frame;
						smallerDiff = largerDiff = 0;
					}
				}
				else if(cmp < 0)
				{
					if(!bestSmaller || -cmp < smallerDiff )
					{
						bestSmaller = frame;
						smallerDiff = -cmp;
					}
				}
				else
				{
					if(!bestLarger || cmp < largerDiff)
					{
						bestLarger = frame;
						largerDiff = cmp;
					}
				}
			}
		}

		if(bestLarger && (!bestSmaller || (largerDiff < smallerDiff) || (flags & kAllowZoom)))
			return bestLarger;
		else
			return bestSmaller;
	}
	else
		return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageResolutionSelector::draw (IGraphics& port, IImage* image, RectRef rect, int flags, int frame, const ImageMode* mode)
{
	draw (port, unknown_cast<Image> (image), rect, flags, frame, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageResolutionSelector::draw (IGraphics& port, Image* image, RectRef rect, int flags, int frameToDraw, const ImageMode* mode)
{
	ImageResolutionSelector s (image, rect, flags, frameToDraw);
	if(s.bestImage)
		port.drawImage (s.bestImage, s.srcRect, s.dstRect, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageResolutionSelector::ImageResolutionSelector (Image* image, RectRef rect, int flags, int frameToDraw)
: bestImage (nullptr)
{
	Point destSize (rect.getWidth (), rect.getHeight ());

	if(Image* best = selectImage (image, destSize, flags))
	{
		best->setCurrentFrame (frameToDraw);

		Rect destRect (rect);
		Rect src;
		best->getSize (src);
		Rect dst (src);

		if(flags & kAllowZoom)
		{
			float imageRatio = src.getWidth () / (float)src.getHeight ();
			float rectRatio = rect.getWidth () / (float)rect.getHeight ();
			
			if(rectRatio > imageRatio)
			{
				int heightDelta = int((rect.getWidth () / imageRatio - rect.getHeight ()) / 2.f);
				destRect.top -= heightDelta;
				destRect.bottom += heightDelta;
			}
			else if(rectRatio < imageRatio)
			{
				int widthDelta = int((rect.getHeight () * imageRatio - rect.getWidth ()) / 2.f);
				destRect.left -= widthDelta;
				destRect.right += widthDelta;
			}
			dst = destRect;
		}
		else if(src.right > destSize.x || src.bottom > destSize.y || (flags & kAllowStretch))
		{
			dst.fitProportionally (destRect);
			dst.center (destRect);
		}
		else
		{
			dst.center (destRect);
		}

		this->bestImage = best;
		this->srcRect = src;
		this->dstRect = dst;
	}
}
