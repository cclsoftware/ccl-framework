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
// Filename    : ccl/gui/graphics/imaging/filmstrip.cpp
// Description : Filmstrip
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/imagepart.h"

using namespace CCL;

//************************************************************************************************
// Filmstrip
//************************************************************************************************

DEFINE_CLASS (Filmstrip, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

Filmstrip::Filmstrip (Image* sourceImage, int frames, FrameMode mode)
: sourceImage (sourceImage),
  frameMode (mode),
  frameCount (1),
  currentFrame (0),
  duration (0.),
  tableRowCount (-1),
  tableColumnCount (-1)
{
	if(sourceImage)
	{
		sourceImage->retain ();
		setFrameCount (frames);
	}

	subImages.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Filmstrip::~Filmstrip ()
{
	if(sourceImage)
		sourceImage->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Filmstrip::setFrameCount (int frames)
{
	frameCount = frames > 0 ? frames : 1;
	currentFrame = ccl_min (frameCount - 1, currentFrame);

	if(sourceImage)
	{
		if(frameMode == kTableMode)
		{
			ASSERT (tableColumnCount > 0 && tableRowCount > 0)
			size (sourceImage->getWidth () / ccl_max (tableColumnCount, 1), sourceImage->getHeight () / ccl_max (tableRowCount, 1));
		}
		else if(frameMode == kHorizontalMode)
		{
			size (sourceImage->getWidth () / frameCount, sourceImage->getHeight ());
		}
		else
		{
			ASSERT (frameMode == kVerticalMode)
			size (sourceImage->getWidth (), sourceImage->getHeight () / frameCount);
		}
	}
	else
		size (0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Filmstrip::getFrameRect (Rect& frameRect, int frameIndex) const
{
	if(frameMode == kTableMode)
	{
		ASSERT (tableColumnCount >= 1)
		int columns = ccl_max (tableColumnCount, 1);
		int row = frameIndex / columns;
		int col = frameIndex % columns;

		Point offset (col * size.x, row * size.y);
		frameRect (offset.x, offset.y, offset.x + size.x, offset.y + size.y);
	}
	else if(frameMode == kHorizontalMode)
	{
		Coord frameOffset = size.x * frameIndex;
		frameRect (frameOffset, 0, frameOffset + size.x , size.y);
	}
	else
	{
		ASSERT (frameMode == kVerticalMode)
		Coord frameOffset = size.y * frameIndex;
		frameRect (0, frameOffset, size.x, frameOffset + size.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Filmstrip::getFrameRect (RectF& frameRect, int frameIndex) const
{
	Rect r;
	getFrameRect (r, frameIndex);
	frameRect = rectIntToF (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Filmstrip::parseFrameNames (StringRef _string)
{
	String string (_string);
	string.trimWhitespace ();

	static String horizontalToken ("h:");
	static String verticalToken ("v:");
	static String tableToken ("t:");

	if(string.startsWith (horizontalToken))
	{
		frameMode = kHorizontalMode;
		string.remove (0, horizontalToken.length ());
	}
	else if(string.startsWith (verticalToken))
	{
		frameMode = kVerticalMode;
		string.remove (0, verticalToken.length ());
	}
	else if(string.startsWith (tableToken))
	{
		frameMode = kTableMode;
		string.remove (0, tableToken.length ());

		// e.g. "t: 4x4 13"
		int frames = 0;
		int result = ::sscanf (MutableCString (string), "%dx%d %d", &tableColumnCount, &tableRowCount, &frames);
		ASSERT (result == 3)

		ccl_lower_limit (tableColumnCount, 1);
		ccl_lower_limit (tableRowCount, 1);

		setFrameCount (frames);
		return result == 3;
	}

	int64 value = 0;
	if(string.getIntValue (value) && value > 0)
	{
		setFrameCount ((int)value);
	}
	else
	{
		ForEachStringToken (string, " ", frameName)
			frameNames.add (MutableCString (frameName));
		EndFor
		setFrameCount (frameNames.count ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID Filmstrip::getFrameName (int index) const
{
	return index < frameNames.count () ? frameNames.at (index) : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Filmstrip::setFrameName (int index, StringID name)
{
	ASSERT (index >= 0 && index < frameCount)

	int newCount = index + 1;
	if(frameNames.count () < newCount)
	{
		if(frameNames.getCapacity () < newCount)
			frameNames.resize (newCount);

		frameNames.setCount (newCount);
	}

	frameNames[index] = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Filmstrip::getSubFrame (StringID name)
{
	ArrayForEach (subImages, ImagePart, subImage)
		if(subImage->getPartName () == name)
			return subImage;
	EndFor

	int frameIndex = getFrameIndex (name);
	if(frameIndex == -1)
		return nullptr;

	Rect frameRect;
	getFrameRect (frameRect, frameIndex);

	ImagePart* subImage = NEW ImagePart (sourceImage, frameRect);
	subImage->setPartName (name);

	// overwrite with filmstrip attributes instead of source image
	subImage->setIsTemplate (getIsTemplate ());
	subImage->setIsAdaptive (getIsAdaptive ());

	subImages.add (subImage);
	return subImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API Filmstrip::getType () const
{
	return kMultiple;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Filmstrip::getFrameCount () const
{
	return frameCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Filmstrip::getCurrentFrame () const
{
	return currentFrame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Filmstrip::setCurrentFrame (int frameIndex)
{
	currentFrame = ccl_bound<int> (frameIndex, 0, frameCount - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Filmstrip::getFrameIndex (StringID name) const
{
	for (int i = 0; i < frameNames.count (); i++)
		if(name == frameNames[i])
			return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Filmstrip::getOriginalImage (Rect& originalRect, bool deep)
{
	getFrameRect (originalRect, currentFrame);

	return resolveOriginal (sourceImage, originalRect, deep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Filmstrip::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	if(sourceImage)
	{
		Rect frameRect;
		getFrameRect (frameRect, currentFrame);

		Rect dst (pos.x, pos.y, size);
		return sourceImage->draw (graphics, frameRect, dst, mode);
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Filmstrip::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	if(sourceImage)
	{
		RectF frameRect;
		getFrameRect (frameRect, currentFrame);

		RectF dst (pos.x, pos.y, pointIntToF (size));
		return sourceImage->draw (graphics, frameRect, dst, mode);
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Filmstrip::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	if(sourceImage)
	{
		Rect frameRect;
		getFrameRect (frameRect, currentFrame);

		Rect frameSrc (src);
		frameSrc.offset (frameRect.getLeftTop ());

		return sourceImage->draw (graphics, frameSrc, dst, mode);
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Filmstrip::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	if(sourceImage)
	{
		RectF frameRect;
		getFrameRect (frameRect, currentFrame);

		RectF frameSrc (src);
		frameSrc.offset (frameRect.getLeftTop ());

		return sourceImage->draw (graphics, frameSrc, dst, mode);
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Filmstrip::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	if(sourceImage)
	{
		Rect frameRect;
		getFrameRect (frameRect, currentFrame);

		// adjust frameSrc according to margins
		Rect frameSrc (src);
		if(src.top > margins.top)
			frameSrc.top = margins.top;
		if(src.left > margins.left)
			frameSrc.left = margins.left;
		if(src.right < (frameRect.getWidth () - margins.right))
			frameSrc.right = frameRect.getWidth () - margins.right;
		if(src.bottom < (frameRect.getHeight () - margins.bottom))
			frameSrc.bottom = frameRect.getHeight () - margins.bottom;

		// adjust margins2 according to frameSrc
		Rect margins2 (margins);
		margins2.left = ccl_bound (ccl_max (0, (margins.left - frameSrc.left)), 0, margins.left);
		margins2.top = ccl_bound (ccl_max (0, (margins.top - frameSrc.top)), 0, margins.top);
		margins2.right = ccl_bound (ccl_max (0, (margins.right - (frameRect.getWidth () - frameSrc.left) + frameSrc.getWidth ())), 0, margins.right);
		margins2.bottom = ccl_bound (ccl_max (0, (margins.bottom - (frameRect.getHeight () - frameSrc.top) + frameSrc.getHeight ())), 0, margins.bottom);

		// adjust frameSrc accroding to frameRect
		frameSrc.offset (frameRect.getLeftTop ());

		return sourceImage->tile (graphics, method, frameSrc, dest, clip, margins2);
	}
	return kResultFalse;
}
