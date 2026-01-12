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
// Filename    : ccl/gui/theme/renderer/scrollpickerrenderer.h
// Description : Scroll Picker Renderer
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/theme/renderer/scrollpickerrenderer.h"

#include "ccl/gui/controls/scrollpicker.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/layout/directions.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// ScrollPickerRenderer
//************************************************************************************************

ScrollPickerRenderer::ScrollPickerRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  centerSize (-1),
  centerOffset (-1),
  itemSize (20),
  visibleItemsFlat (-1),
  numberOfValues (0),
  numberOfRenderValues (0),
  multiDigitsMode (false),
  overScrollMargin (0),
  currentScaleFactor (1),
  gradientThickness (10),
  centerCharWidth (0),
  barrelCharWidth (0),
  imageMargin (10),
  wrapAround (false),
  carousel (false),
  vertical (true),
  flatBarrel (false),
  initDone (false)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollPickerRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	Rect centerRect (calcCenterLensViewRect ());
	
	if(centerRect.pointInside (loc))
		return ScrollPicker::kPartCenter;
	
	if((loc.y < centerRect.top) || (loc.x < centerRect.left))
	   return ScrollPicker::kPartUpperLeft;
	else
   		return ScrollPicker::kPartLowerRight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPickerRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	switch(partCode)
	{
	default:
		return false;
	case ScrollPicker::kPartCenter :
		rect = calcCenterLensViewRect ();
		return true;
	case ScrollPicker::kPartUpperLeft :
		{
			if(vertical)
				rect.bottom = getCenterViewOffset () + getCenterViewSize ();
			else
				rect.left = getCenterViewOffset () + getCenterViewSize ();
			return true;
		}
	case ScrollPicker::kPartLowerRight :
		{
			if(vertical)
				rect.top = getCenterViewOffset () - getCenterViewSize ();
			else
				rect.right = getCenterViewOffset () - getCenterViewSize ();
			return true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	ScrollPicker* scrollPicker = static_cast<ScrollPicker*> (view);
	init (scrollPicker);

	GraphicsPort port (scrollPicker);
	port.fillRect (scrollPickerSize, SolidBrush (backcolor));

	if(carousel)
	{
		if(UnknownPtr<IPaletteProvider> provider = scrollPicker->getParameter ())
			if(IPalette* palette = provider->getPalette ())
				drawCarousel<Styles::kHorizontal> (port, scrollPicker->getScrollPosition (), palette);
		
		drawGradients (port);
	}
	else if(multiDigitsMode)
	{
		drawDigitBarrelView (port, scrollPicker);

		if(!scrollPicker->isEditing ())
			drawDigitCenterView (port, scrollPicker);

		drawGradients (port);
	}
	else
	{
		if(vertical)
		{
			if(flatBarrel)
				drawFlatBarrel<Styles::kVertical> (port, scrollPicker->getScrollPosition ());
			else
				drawBarrel<Styles::kVertical> (port, scrollPicker->getScrollPosition ());

			if(!scrollPicker->isEditing ())
				drawCenterView<Styles::kVertical> (port, scrollPicker->getScrollPosition ());
		}
		else
		{
			if(flatBarrel)
				drawFlatBarrel<Styles::kHorizontal> (port, scrollPicker->getScrollPosition ());
			else
				drawBarrel<Styles::kHorizontal> (port, scrollPicker->getScrollPosition ());

			if(!scrollPicker->isEditing ())
				drawCenterView<Styles::kHorizontal> (port, scrollPicker->getScrollPosition ());
		}

		drawGradients (port);
	}

	if(scrollPicker->isEditing ())
		port.fillRect (calcCenterLensViewRect (), SolidBrush (centerBackcolor));
	else if(centerOverlayImage)
		port.drawImage (centerOverlayImage, Rect (0, 0, centerOverlayImage->getWidth (), centerOverlayImage->getHeight ()), calcCenterLensViewRect ());
}

///////////////////////////////////////////////////////////////////////////

template<int direction>
void ScrollPickerRenderer::drawBarrel (GraphicsPort& port, int scrollPosition)
{
	Coord firstSliceBarrelOffset = getBarrelPaddingOffset ();
	Coord barrelScrollPosition = scrollPosition + (itemSize - firstSliceBarrelOffset);
	
	int firstFullyVisibleFrameIndex = barrelScrollPosition / itemSize;
	
	// barrelUnitOffset - "scrollRect" offset to the start position of the first fully visible frame.
	// barrelUnitOffset is in units, not points - drawOffset will be in points - see getBarrelProjectionOffset
	float barrelUnitOffset = ccl_round<2> (getBarrelPlaneOffset (barrelScrollPosition) / (float)itemSize);

	Coord drawOffset = getBarrelProjectionOffset (barrelUnitOffset);
	Coord imageOffset = (firstFullyVisibleFrameIndex * itemSize);
	
	//CCL_PRINTF ("frameIndex: %d, barrelPlaneOffset: %d, barrelUnitOffset: %f, drawOffset: %d, imageOffset: %d barrelScrollPosition: %d, topSliceBarrelMargin: %d \n", firstFullyVisibleFrameIndex, getBarrelPlaneOffset (barrelScrollPosition), barrelUnitOffset, drawOffset, imageOffset, barrelScrollPosition, topSliceBarrelMargin);
	
	Rect imageRect;
	barrelImage->getSize (imageRect);
	
	typedef DirectionTraits<direction> Direction;
	
	Direction::setLength (imageRect, itemSize);
	Direction::offset (imageRect, imageOffset);

	Rect targetRect (scrollPickerSize);

	int fullyVisibleItems = (int)getVisibleBarrelItemsCount ();
	for(int i = 1; i <= fullyVisibleItems; i++)
	{
		Direction::getStartCoord (targetRect) = drawOffset;

		drawOffset = getBarrelProjectionOffset (barrelUnitOffset + i);

		Direction::getEndCoord (targetRect) = drawOffset;

		ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);
		port.drawImage (barrelImage, imageRect, targetRect, &mode);

		Direction::offset (imageRect, itemSize);
	}
}

///////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getBarrelPlaneOffset (int scrollPosition) const
{
	Coord barrelPlaneOffset = (itemSize - ((scrollPosition) % itemSize)) % itemSize;
	if(barrelPlaneOffset == 0)
		barrelPlaneOffset = itemSize;

	return barrelPlaneOffset;
}

///////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getBarrelProjectionOffset (float barrelUnitOffset) const
{
	float barrelTheta = barrelUnitOffset * Math::Constants<float>::kPi / getVisibleBarrelItemsCount ();
	return ccl_to_int((1 - Math::Functions<float>::cos (barrelTheta)) * (visibleItemsFlat / 2.f) * itemSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int direction>
void ScrollPickerRenderer::drawFlatBarrel (GraphicsPort& port, int scrollPosition)
{
	typedef DirectionTraits<direction> Direction;
	
	Rect clientRect (scrollPickerSize);
	Rect imageRect (scrollPickerSize);
	Direction::offset (imageRect, scrollPosition);
	
	port.drawImage (barrelImage, imageRect, clientRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int direction>
void ScrollPickerRenderer::drawCenterView (GraphicsPort& port, int scrollPosition)
{
	typedef DirectionTraits<direction> Direction;
	
	Rect imageRect (scrollPickerSize);
	Direction::setLength (imageRect, getCenterViewSize ());
	Coord centerViewOffset = (itemSize - getCenterViewSize ()) / 2;
	Direction::offset (imageRect, scrollPosition + centerViewOffset);
	
	port.drawImage (centerImage, imageRect, calcCenterLensViewRect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int direction>
void ScrollPickerRenderer::drawCarousel (GraphicsPort& port, int scrollPosition, IPalette* palette)
{
	typedef DirectionTraits<direction> Direction;
	
	ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);

	// getImageIndices
	Coord clientEnd = Direction::getEndCoord (scrollPickerSize);
	int items = (clientEnd - centerSize) / itemSize;
	items += 1; // plus center
	
	bool evenItems = ((items % 2) == 0) ? true : false;
	Coord additionalCenteringOffset = evenItems ? (itemSize * 0.5) : itemSize;
	
	float itemsF = (clientEnd - centerSize) / (float)itemSize;
	float itemsFLeft = (itemsF - 0.999f) / 2;
		
	Coord leftoverPart = ((clientEnd - centerSize) % itemSize) / 2;

	if(!wrapAround)
		scrollPosition -= (3 * itemSize);
	
	int centerItemIndex = scrollPosition / itemSize;
	int firstItemIndex = centerItemIndex - ceil (itemsFLeft);
	int lastItemIndex = firstItemIndex + items + 1;
	
	int secondCenterItemIndex = centerItemIndex;
	int centerOffset = (scrollPosition % itemSize);
	
	if(centerOffset > 0)
		secondCenterItemIndex++;
	else if(centerOffset < 0)
		centerItemIndex--;
	
	float centerOffsetFactor = centerOffset / (float)itemSize;
	leftoverPart = -centerOffset + leftoverPart;
	
	CCL_PRINTF ("items %d %f %d %d first/last %d %d addOffset %d\n", items, itemsFLeft, ((int)ceil (itemsFLeft)), leftoverPart, firstItemIndex, lastItemIndex, additionalCenteringOffset)
	
	Coord otherStart = Direction::OtherDirection::getStartCoord (scrollPickerSize);
	Coord otherEnd = Direction::OtherDirection::getEndCoord (scrollPickerSize);

	auto getImageFrameOfVisiblePosition = [&] (int index, Coord space, Coord offset)
	{
		Coord start = (leftoverPart - itemSize) + (itemSize * index) + offset;
		Coord end = start + space;
		if(Direction::isHorizontal ())
			return Rect (start, otherStart, end, otherEnd);
		else
			return Rect (otherStart, start, otherEnd, end);
	};
	
	auto makeValidIndex = [&] (int& index)
	{
		if(!wrapAround)
			return 0;
		
		if(index < 0)
		{
			index += numberOfValues;
			return numberOfValues;
		}
		else if(index >= numberOfValues)
		{
			index -= numberOfValues;
			return -numberOfValues;
		}
		return 0;
	};


	auto hasValidIndex = [&] (int index)
	{
		return (index >= 0 && index < numberOfValues);
	};

	auto drawItemOverlay = [&] (int index, RectRef rect)
	{
		if(!palette->isEnabled (index))
			port.fillRect (rect, SolidBrush (disabledColor));
	};

	// draw images
	for(int itemIndex = firstItemIndex; itemIndex <= lastItemIndex; itemIndex++)
	{
		if((itemIndex != centerItemIndex) && (itemIndex != secondCenterItemIndex))
		{
			Coord additionalOffset = additionalCenteringOffset;
			if(itemIndex > secondCenterItemIndex)
				additionalOffset += (centerSize - itemSize);

			int validIndex = itemIndex;
			if(hasValidIndex (validIndex))
			{
				int addedIndices = makeValidIndex (validIndex);
				if(AutoPtr<IImage> image = palette->createIcon (validIndex, itemSize, itemSize, VisualStyle::emptyStyle))
				{
					Rect src (0, 0, image->getWidth (), image->getHeight ());
					Rect dst (getImageFrameOfVisiblePosition (validIndex - (firstItemIndex + addedIndices), itemSize, additionalOffset));
					dst.contract (imageMargin);
					Image* drawable = unknown_cast<Image> (image);
					ImageResolutionSelector s (drawable, dst);
					port.drawImage (s.bestImage, s.srcRect, s.dstRect, &mode);
					drawItemOverlay (validIndex, s.dstRect);
				}
			}
		}
	}

	// draw bigger center image
	if(hasValidIndex (centerItemIndex))
	{
		if(AutoPtr<IImage> image = palette->createIcon (centerItemIndex, itemSize, itemSize, VisualStyle::emptyStyle))
		{
			Rect src (0, 0, image->getWidth (), image->getHeight ());
			Coord space = (centerOffsetFactor * itemSize) + (centerSize * (1 - centerOffsetFactor));
			Rect dst (getImageFrameOfVisiblePosition (centerItemIndex - firstItemIndex, space, additionalCenteringOffset));
			dst.contract (imageMargin * centerOffsetFactor);
			Image* drawable = unknown_cast<Image> (image);
			ImageResolutionSelector s (drawable, dst);
			port.drawImage (s.bestImage, s.srcRect, s.dstRect, &mode);
			drawItemOverlay (centerItemIndex, s.dstRect);
		}
	}

	if(centerOffset > 0)
	{
		int validIndex = secondCenterItemIndex;
		if(hasValidIndex (validIndex))
		{
			int addedIndices = makeValidIndex (validIndex);
			if(AutoPtr<IImage> image = palette->createIcon (validIndex, itemSize, itemSize, VisualStyle::emptyStyle))
			{
				Rect src (0, 0, image->getWidth (), image->getHeight ());
				Coord space = ((1 - centerOffsetFactor) * itemSize) + (centerSize * centerOffsetFactor);
				Coord additionalOffset = additionalCenteringOffset;
				additionalOffset += ((1 - centerOffsetFactor) * (centerSize - itemSize));
				Rect dst (getImageFrameOfVisiblePosition (validIndex - (firstItemIndex + addedIndices), space, additionalOffset));
				dst.contract (imageMargin * (1 - centerOffsetFactor));
				Image* drawable = unknown_cast<Image> (image);
				ImageResolutionSelector s (drawable, dst);
				port.drawImage (s.bestImage, s.srcRect, s.dstRect, &mode);
				drawItemOverlay (validIndex, s.dstRect);
			}
		}
	}
	else if(centerOffset < 0) // special case: first item as "second" center image
	{
		if(hasValidIndex (secondCenterItemIndex))
		{
			if(AutoPtr<IImage> image = palette->createIcon (secondCenterItemIndex, itemSize, itemSize, VisualStyle::emptyStyle))
			{
				centerOffsetFactor *= -1;
				Rect src (0, 0, image->getWidth (), image->getHeight ());
				Coord space = (centerOffsetFactor * itemSize) + (centerSize * (1 - centerOffsetFactor));
				Coord additionalOffset = additionalCenteringOffset;
				additionalOffset += ((centerOffsetFactor) * (centerSize - itemSize));
				Rect dst (getImageFrameOfVisiblePosition (secondCenterItemIndex - firstItemIndex, space, additionalOffset));
				dst.contract (imageMargin * centerOffsetFactor);
				Image* drawable = unknown_cast<Image> (image);
				ImageResolutionSelector s (drawable, dst);
				port.drawImage (s.bestImage, s.srcRect, s.dstRect, &mode);
				drawItemOverlay (secondCenterItemIndex, s.dstRect);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawGradients (GraphicsPort& port)
{	
	port.fillRect (firstGradientRect, firstGradient);
	port.fillRect (lastGradientRect, lastGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawDigitBarrelView (GraphicsPort& port, ScrollPicker* picker)
{
	int scrollPosition = picker->getScrollPosition () - overScrollMargin;
	Coord firstSliceBarrelOffset = getBarrelPaddingOffset ();
	Coord barrelScrollPosition = scrollPosition - firstSliceBarrelOffset;
	
	if(barrelScrollPosition < 0)
		barrelScrollPosition -= itemSize;
	
	int frameIndex = (barrelScrollPosition / itemSize) - (int(getVisibleBarrelItemsCount () - 1) / 2);
	// firstFullyVisibleFrameIndex will be frameIndex + 1
	
	// barrelUnitOffset - "scrollRect" offset to the start position of the first fully visible frame.
	// barrelUnitOffset is in units, not points - drawOffset will be in points - see getBarrelProjectionOffset
	float barrelUnitOffset = ccl_round<2> (getBarrelPlaneOffset (barrelScrollPosition) / (float)itemSize);
	
	Coord drawOffset = getBarrelProjectionOffset (barrelUnitOffset);

	Rect targetRect (scrollPickerSize);

	int fullyVisibleItems = (int)getVisibleBarrelItemsCount ();
	
	CCL_PRINTF ("\nframeIndex: %d, barrelUnitOffset: %f, drawOffset: %d\n", frameIndex, barrelUnitOffset, drawOffset);
	
	for(int i = 1; i <= fullyVisibleItems; i++)
	{
		Coord targetStartCoord = drawOffset;
		
		drawOffset = getBarrelProjectionOffset (barrelUnitOffset + i);
		
		Coord targetEndCoord = drawOffset;
		
		int currentValue = getValueFromIndex (picker->getParameter (), frameIndex + i);
		
		CCL_PRINTF (" %d ", currentValue);
		
		if(vertical)
			drawValueBarrelDigitsSliceVertical (port, picker, currentValue, targetStartCoord, targetEndCoord);
		else
			drawValueBarrelDigitsSliceHorizontal (port, picker, currentValue, targetStartCoord, targetEndCoord);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawDigitCenterView (GraphicsPort& port, ScrollPicker* picker)
{
	int scrollPosition = picker->getScrollPosition ();
	int scrollPositionAtCenterStart = scrollPosition - overScrollMargin;
	int leadingIndex = scrollPositionAtCenterStart / itemSize;
	float centerSizeFactor = getCenterViewSize () / (float)itemSize;
	int sourceFrameStartPosition = int((scrollPosition % itemSize) * centerSizeFactor);
	
	if(scrollPositionAtCenterStart < 0)
		leadingIndex -= 1;
	
	CCL_PRINTF ("leading: %d, trailing: %d, startPosition: %d\n", leadingIndex, leadingIndex + 1, sourceFrameStartPosition);
		
	int leadingValue = getValueFromIndex (picker->getParameter (), leadingIndex);
	int trailingValue = getValueFromIndex (picker->getParameter (), leadingIndex + 1);

	if(vertical)
	{
		drawValueDigitsSlice<Styles::kVertical> (port, picker, leadingValue, sourceFrameStartPosition, getCenterViewSize ());
		drawValueDigitsSlice<Styles::kVertical> (port, picker, trailingValue, 0, sourceFrameStartPosition);
	}
	else
	{
		drawValueDigitsSlice<Styles::kHorizontal> (port, picker, leadingValue, sourceFrameStartPosition, getCenterViewSize ());
		drawValueDigitsSlice<Styles::kHorizontal> (port, picker, trailingValue, 0, sourceFrameStartPosition);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawValueBarrelDigitsSliceVertical (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord targetStartCoord, Coord targetEndCoord)
{
	Rect targetRect (scrollPickerSize);
	targetRect.top = targetStartCoord;
	targetRect.bottom = targetEndCoord;

	drawValueDigitsSliceVertical (port, currentValue, barrelImage, 0, itemSize, targetRect, kBarrel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawValueBarrelDigitsSliceHorizontal (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord targetStartCoord, Coord targetEndCoord)
{
	Rect targetRect (scrollPickerSize);
	targetRect.left = targetStartCoord;
	targetRect.right = targetEndCoord;
		
	Rect bitmapRect (0, 0, barrelImage->getWidth (), barrelImage->getHeight ());
	bitmapRect.bottom = targetRect.getHeight ();
	
	port.drawImage (barrelImage, bitmapRect, targetRect);
	
	if(currentValue == NumericLimits::kMaxInt) // invalid value
		return;
	
	
	float frameSquashFactor = targetRect.getWidth () / (float)itemSize;
	
	Rect digitTargetRect (targetRect);
	
	int digitCount = (int)floor (log10 (ccl_abs (currentValue ? currentValue : 1)) + 1);
	int sign = ccl_sign (currentValue);
	
	currentValue = ccl_abs (currentValue);
	
	if(sign < 0)
		digitCount++;
		 
	float leadingBlankDigits = getLeadingBlankDigits (digitCount, kBarrel);
	
	while(digitCount > 0)
	{
		digitCount--;
		
		int digit = currentValue % 10;
		currentValue /= 10;
		
		Coord bitmapSourceOffset;
		if(digitCount == 0 && sign < 0)
			bitmapSourceOffset = itemSize;
		else
			bitmapSourceOffset = (digit + 2) * itemSize;
		
		bitmapRect.top = 0;
		bitmapRect.bottom = itemSize;

		int bitmapRectStart = 0;
		
		digitTargetRect.left = int(int((leadingBlankDigits + digitCount) * barrelCharWidth + padding.left) * frameSquashFactor) + targetRect.left;
		digitTargetRect.right = int(int((leadingBlankDigits + digitCount + 1) * barrelCharWidth + padding.left) * frameSquashFactor) + targetRect.left;
		
		bitmapRect.left = bitmapRectStart;
		bitmapRect.setWidth (barrelCharWidth);
		bitmapRect.offset (0, bitmapSourceOffset);
		
		port.drawImage (barrelImage, bitmapRect, digitTargetRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int direction>
void ScrollPickerRenderer::drawValueDigitsSlice (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition)
{
	typedef DirectionTraits<direction> Direction;
	
	Rect targetRect (calcCenterLensViewRect ());
	Direction::setLength (targetRect, sourceFrameEndPosition - sourceFrameStartPosition);
	Coord targetOffset = (sourceFrameStartPosition == 0) ? (getCenterViewSize () - sourceFrameEndPosition) : 0;
	Direction::offset (targetRect, targetOffset);
	
	if(vertical)
		drawValueDigitsSliceVertical (port, currentValue, centerImage, sourceFrameStartPosition, sourceFrameEndPosition, targetRect);
	else
		drawValueDigitsCenterSliceHorizontal (port, currentValue, centerImage, sourceFrameStartPosition, sourceFrameEndPosition, targetRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawValueDigitsCenterSliceHorizontal (GraphicsPort& port, int currentValue, IImage* sourceBitmap, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition, RectRef targetRect)
{
	bool leadingFrame = (sourceFrameStartPosition == 0) ? false : true;
	Coord frameSize = sourceFrameEndPosition - sourceFrameStartPosition;
		
	Rect bitmapRect (0, 0, sourceBitmap->getWidth (), sourceBitmap->getHeight ());
	bitmapRect.bottom = targetRect.getHeight ();
	
	port.drawImage (sourceBitmap, bitmapRect, targetRect);
	
	if(currentValue == NumericLimits::kMaxInt) // invalid value
		return;
	
	
	Rect digitTargetRect (targetRect);
	
	int digitCount = (int)floor (log10 (ccl_abs (currentValue ? currentValue : 1)) + 1);
	int sign = ccl_sign (currentValue);
	
	currentValue = ccl_abs (currentValue);
	
	if(sign < 0)
		digitCount++;
		 
	float leadingBlankDigits = getLeadingBlankDigits (digitCount);
		
	while(digitCount > 0)
	{
		digitCount--;
		
		int digit = currentValue % 10;
		currentValue /= 10;
		
		Coord bitmapSourceOffset;
		if(digitCount == 0 && sign < 0)
			bitmapSourceOffset = itemSize;
		else
			bitmapSourceOffset = (digit + 2) * itemSize;
		
		bitmapRect.top = 0;
		bitmapRect.bottom = itemSize;

		digitTargetRect.left = targetRect.left;
		digitTargetRect.setWidth (targetRect.getWidth ());
		
		int bitmapRectStart = 0;
		int bitmapLength = centerCharWidth;
		int offsetPosition = ccl_to_int ((leadingBlankDigits + digitCount) * centerCharWidth + padding.left);
		
		if(leadingFrame)
		{
			int hiddenStartTargetPosition = frameSize - calcCenterLensViewRect ().getWidth ();
			offsetPosition += hiddenStartTargetPosition;
			
			if(offsetPosition < 0)
			{
				bitmapRectStart = -offsetPosition;
				offsetPosition = 0;
				bitmapLength = centerCharWidth - bitmapRectStart;
				
				if(bitmapLength < 0)
					continue;
			}
		}
		else
		{
			int endTargetPosition = digitTargetRect.left + offsetPosition + centerCharWidth;
			int hiddenBitmapPortion = endTargetPosition - calcCenterLensViewRect ().right;
			
			if(hiddenBitmapPortion > 0)
			{
				bitmapLength = centerCharWidth - hiddenBitmapPortion;
				
				if(bitmapLength < 0)
					continue;
			}

		}
		
		digitTargetRect.setWidth (bitmapLength);
		digitTargetRect.offset (offsetPosition, 0);
		
		bitmapRect.left = bitmapRectStart;
		bitmapRect.setWidth (bitmapLength);
		bitmapRect.offset (0, bitmapSourceOffset);
		
		port.drawImage (sourceBitmap, bitmapRect, digitTargetRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::drawValueDigitsSliceVertical (GraphicsPort& port, int currentValue, IImage* sourceBitmap, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition, RectRef targetRect, ViewPart viewId)
{
	Coord frameSize = sourceFrameEndPosition - sourceFrameStartPosition;
	
	Rect bitmapRect (0, 0, sourceBitmap->getWidth (), sourceBitmap->getHeight ());
	bitmapRect.bottom = targetRect.getHeight ();
	
	port.drawImage (sourceBitmap, bitmapRect, targetRect);
	
	if(currentValue == NumericLimits::kMaxInt) // invalid value
		return;
	
	Rect digitTargetRect (targetRect);
	Coord verticalBitmapOffset = 0;
	Coord charWidth = barrelCharWidth;
	if(viewId == kCenter)
	{
		verticalBitmapOffset = (itemSize - getCenterViewSize ()) / 2;
		charWidth = centerCharWidth;
	}
		
	int digitCount = (int)floor (log10 (ccl_abs (currentValue ? currentValue : 1)) + 1);
	int sign = ccl_sign (currentValue);
	
	currentValue = ccl_abs (currentValue);
	
	if(sign < 0)
		digitCount++;
		 
	float leadingBlankDigits = getLeadingBlankDigits (digitCount, viewId);
		
	while(digitCount > 0)
	{
		digitCount--;
		
		int digit = currentValue % 10;
		currentValue /= 10;
		
		Coord bitmapSourceOffset;
		if(digitCount == 0 && sign < 0)
			bitmapSourceOffset = itemSize;
		else
			bitmapSourceOffset = (digit + 2) * itemSize;
		
		bitmapRect.top = 0;
		bitmapRect.bottom = frameSize;
		bitmapRect.offset (0, bitmapSourceOffset + sourceFrameStartPosition + verticalBitmapOffset);
		
		digitTargetRect.left = 0;
		digitTargetRect.right = charWidth;
		
		int charPositionOffset = ccl_to_int ((leadingBlankDigits + digitCount) * charWidth + padding.left);
		digitTargetRect.offset (charPositionOffset, 0);

		port.drawImage (sourceBitmap, bitmapRect, digitTargetRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ScrollPickerRenderer::getLeadingBlankDigits (int digitCount, ViewPart viewId) const
{
	float maxDigits = 0;
	
	if(vertical)
	{
		maxDigits = calcCenterLensViewRect ().getWidth () - (padding.left + padding.right);
		if(viewId == kCenter)
			maxDigits /= (float)centerCharWidth;
		else
			maxDigits /= (float)barrelCharWidth;
	}
	else
	{
		if(viewId == kCenter)
			maxDigits = (calcCenterLensViewRect ().getWidth () - (padding.left + padding.right)) / (float)centerCharWidth;
		else
			maxDigits = (itemSize - (padding.left + padding.right)) / (float)barrelCharWidth;
	}
	
	float leadingBlanks = maxDigits - digitCount;
	if(vertical)
	{
		if(textAlignment.getAlignH () == Alignment::kHCenter)
			leadingBlanks /= 2.f;
		else if(textAlignment.getAlignH () == Alignment::kLeft)
			leadingBlanks = 0;
	}
	else
		leadingBlanks /= 2.f;
		
	return leadingBlanks;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::init (ScrollPicker* scrollPicker)
{
	if(initDone)
		return;

	updateViewDependentValues (scrollPicker);

	updateStyle (scrollPicker); // updateStyle () resets calculated centerOffset and centerSize values, when no fixed skin-values are used
	
	prepareGradiants ();
	
	if(carousel == false)
		constructBitmapAssets (scrollPicker->getParameter ());
	
	initDone = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::updateViewDependentValues (ScrollPicker* scrollPicker)
{
	// view options
	StyleRef style = scrollPicker->getStyle ();
	wrapAround = style.isCustomStyle (Styles::kScrollPickerBehaviorWrapAround);
	vertical = style.isVertical ();
	flatBarrel = style.isCustomStyle (Styles::kScrollPickerAppearanceFlatBarrel);
	
	scrollPicker->getClientRect (scrollPickerSize);
	
	itemSize = scrollPicker->getItemSize ();
	visibleItemsFlat = scrollPicker->getVisibleItemCountFlat ();
	overScrollMargin = wrapAround ? 0 : scrollPicker->getOverScrollMargin ();
	currentScaleFactor = scrollPicker->getWindow ()->getContentScaleFactor ();
	numberOfValues = scrollPicker->getValueCount ();
	numberOfRenderValues = numberOfValues;
	
	if(scrollPicker->isCarouselMode ())
	{
		carousel = true;
	}
	else if(isMultiDigitsMode (scrollPicker))
	{
		flatBarrel = false;
		wrapAround = false;
		numberOfRenderValues = 12; // empty, sign and 0 - 9
	}
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::updateStyle (ScrollPicker* scrollPicker)
{
	const IVisualStyle& vs = scrollPicker->getVisualStyle ();
	float zoomFactor = scrollPicker->getZoomFactor ();
	backcolor = vs.getBackColor ();
	centerBackcolor = vs.getColor ("centerBackcolor", backcolor);
	disabledColor = vs.getColor ("disabledColor", Color (0,0,0,128));
	centerSize = vs.getMetric<Coord> ("centerSize", -1) * zoomFactor;
	centerOffset = vs.getMetric<Coord> ("centerOffset", -1);
	gradientThickness = vs.getMetric<Coord> ("gradientThickness", itemSize) * zoomFactor;
	centerTextColor = vs.getColor ("centerTextColor", vs.getTextColor ());
	barrelTextColor = vs.getColor ("barrelTextColor", centerTextColor);
	centerTextFont = vs.getFont ("centerTextFont", vs.getTextFont ());
	barrelTextFont = vs.getFont ("barrelTextFont", centerTextFont);
	centerOverlayImage = vs.getImage ("centerOverlayImage");
	imageMargin = vs.getMetric<Coord> ("imageMargin", imageMargin) * zoomFactor;
	vs.getPadding (padding);
	
	String allDigits ("0-0123456789");

	Rect allDigitsRect;
	Font::measureString (allDigitsRect, allDigits, centerTextFont);
	centerCharWidth = (allDigitsRect.getWidth () / allDigits.length ()) + 1;
	
	Font::measureString (allDigitsRect, allDigits, barrelTextFont);
	barrelCharWidth = (allDigitsRect.getWidth () / allDigits.length ()) + 1;
	
	textAlignment = vs.getTextAlignment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::prepareGradiants ()
{
	Color transparentColor (backcolor);
	transparentColor.setAlphaF (0);
	
	firstGradientRect = scrollPickerSize;
	lastGradientRect = scrollPickerSize;
	
	if(vertical)
	{
		firstGradientRect.bottom = scrollPickerSize.top + gradientThickness;
		lastGradientRect.top = scrollPickerSize.bottom - gradientThickness;

		firstGradient = LinearGradientBrush (pointIntToF (firstGradientRect.getLeftTop ()),
											 pointIntToF (firstGradientRect.getLeftBottom ()),
											 backcolor,
											 transparentColor);

		lastGradient = LinearGradientBrush (pointIntToF (lastGradientRect.getLeftTop ()),
											pointIntToF (lastGradientRect.getLeftBottom ()),
											transparentColor,
											backcolor);
	}
	else
	{
		firstGradientRect.right = scrollPickerSize.left + gradientThickness;
		lastGradientRect.left = scrollPickerSize.right - gradientThickness;
		
		firstGradient = LinearGradientBrush (pointIntToF (firstGradientRect.getLeftTop ()),
											 pointIntToF (firstGradientRect.getRightTop ()),
											 backcolor,
											 transparentColor);

		lastGradient = LinearGradientBrush (pointIntToF (lastGradientRect.getLeftTop ()),
											pointIntToF (lastGradientRect.getRightTop ()),
											transparentColor,
											backcolor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPickerRenderer::isMultiDigitsMode (ScrollPicker* scrollPicker) const
{
	if(multiDigitsMode)
		return true;

	multiDigitsMode = scrollPicker->getStyle ().isCustomStyle (Styles::kScrollPickerAppearanceDigitMode);
		
	Rect barrelRect (calculateBarrelBitmapRect ());
	
	// set explicitly or check for unreasonable bitmap sizes...
	static const int kMaxSize = 2048;
	Rect maxSizeRect (0, 0, kMaxSize, kMaxSize);
	if(maxSizeRect.rectInside (barrelRect) == false)
	{
		// ... set multiDigitsMode, but parameter needs to be an intParam in this case
		bool valid = (scrollPicker->getParameter ()->getType () == IParameter::kInteger);
		ASSERT (valid)
		
		multiDigitsMode = true;
	}
	
	return multiDigitsMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::constructBitmapAssets (IParameter* scrollPickerParam)
{
	Rect barrelRect (calculateBarrelBitmapRect ());

	barrelImage = NEW Bitmap (barrelRect.getWidth (), barrelRect.getHeight (), Bitmap::kRGBAlpha, currentScaleFactor);
	BitmapGraphicsDevice barrelDevice (barrelImage);
	renderScrollPickerBitmap (&barrelDevice, barrelRect, scrollPickerParam, kBarrel);

	Rect centerRect (calculateCenterBitmapRect ());

	centerImage = NEW Bitmap (centerRect.getWidth (), centerRect.getHeight (), IBitmap::kRGBAlpha, currentScaleFactor);
	BitmapGraphicsDevice centerDevice (centerImage);
	renderScrollPickerBitmap (&centerDevice, centerRect, scrollPickerParam, kCenter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getBarrelPaddingOffset () const
{
	// alignment offset to first fully visible barrel item
	int fullyVisibleBarrelRows = int(getVisibleBarrelItemsCount ());
	return ccl_to_int (((getVisibleBarrelItemsCount () - fullyVisibleBarrelRows) * itemSize) / 2.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getBlankBarrelItemsOffset () const
{
	// to have enough space to see the first/last item centered
	if(flatBarrel)
	{
		return ccl_to_int ((visibleItemsFlat - 1) / 2.f * itemSize);
	}
	else
	{
		return int((getVisibleBarrelItemsCount () - 1) / 2) * itemSize;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ScrollPickerRenderer::getVisibleBarrelItemsCount () const
{
	float barrelItems = flatBarrel ? visibleItemsFlat : visibleItemsFlat * Math::Constants<float>::kHalfPi;
	
	if(int(barrelItems) % 2 == 0) // even number of fully visible barrel items at this size
		barrelItems += 1; // barrel will be adjusted - this is totally fine
	
	return barrelItems;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getCenterViewSize () const
{
	return (centerSize == -1) ? itemSize : centerSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPickerRenderer::getCenterViewOffset () const
{
	if(centerOffset == -1)
	{
		Coord halfSize = vertical ? (scrollPickerSize.getHeight () / 2) : (scrollPickerSize.getWidth () / 2);
		return halfSize - (getCenterViewSize () / 2);
	}
	return centerOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ScrollPickerRenderer::calcCenterLensViewRect () const
{
	Rect centerRect (scrollPickerSize);
	
	if(vertical)
	{
		centerRect.setHeight (getCenterViewSize ());
		centerRect.offset (0, getCenterViewOffset ());
	}
	else
	{
		centerRect.setWidth (getCenterViewSize ());
		centerRect.offset (getCenterViewOffset (), 0);
	}

	return centerRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ScrollPickerRenderer::calculateCenterBitmapRect () const
{
	Rect bitmapRect (scrollPickerSize);
	
	Coord size = itemSize * numberOfRenderValues;
	
	if(!multiDigitsMode)
	{
		if(wrapAround)
			size += itemSize;
		else
			size += overScrollMargin * 2;
	}
	
	if(vertical || multiDigitsMode)
	{
		bitmapRect.setHeight (size);
		
		if(multiDigitsMode)
			bitmapRect.setWidth (centerCharWidth);
	}
	else
	{
		bitmapRect.setWidth (size);
	}
	
	return bitmapRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ScrollPickerRenderer::calculateBarrelBitmapRect () const
{
	Rect bitmapRect (calculateCenterBitmapRect ());
	
	if(multiDigitsMode)
	{
		bitmapRect.setWidth (barrelCharWidth);
		return bitmapRect;
	}
	
	Coord size = getBlankBarrelItemsOffset () * 2;
	
	if(vertical)
		bitmapRect.setHeight (bitmapRect.getHeight () + size);
	else
		bitmapRect.setWidth (bitmapRect.getWidth () + size);
	
	return bitmapRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::renderScrollPickerBitmap (GraphicsDevice* device, RectRef bitmapRect, IParameter* scrollPickerParam, ViewPart viewId)
{
	device->fillRect (bitmapRect, SolidBrush ((viewId == kCenter) ? centerBackcolor : backcolor));

	String title;
	Rect r (bitmapRect);
	
	if(multiDigitsMode)
	{
		r.setHeight (itemSize);
	}
	else
	{
		if(vertical)
		{
			r.top = overScrollMargin; // 0 if wrapAround
			r.offset (0, (viewId == kCenter) ? 0 : getBlankBarrelItemsOffset ());
			r.setHeight (itemSize);
		}
		else
		{
			r.left = overScrollMargin; // 0 if wrapAround
			r.offset ((viewId == kCenter) ? 0 : getBlankBarrelItemsOffset (), 0);
			r.setWidth (itemSize);
		}
	}

	FontRef textFont = ((viewId == kCenter) ? centerTextFont : barrelTextFont);
	SolidBrushRef brush ((viewId == kCenter) ? centerTextColor : barrelTextColor);
	
	for(int i = 0; i < numberOfRenderValues; i++)
	{
		if(multiDigitsMode)
			getDigitTitle (title, i);
		else
			scrollPickerParam->getString (title, getValueFromIndex (scrollPickerParam, i));

		device->drawString (r, title, textFont, brush, multiDigitsMode ? Alignment::kCenter : textAlignment);

		if(vertical || multiDigitsMode)
			r.offset (0, itemSize);
		else
			r.offset (itemSize, 0);
	}
	
	if(wrapAround && !multiDigitsMode)
		renderWrapAroundElements (device, bitmapRect, scrollPickerParam, viewId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::renderWrapAroundElements (GraphicsDevice* device, RectRef bitmapRect, IParameter* scrollPickerParam, ViewPart viewId)
{
	String title;
	Rect r (bitmapRect);
	Font& textFont = ((viewId == kCenter) ? centerTextFont : barrelTextFont);
	SolidBrush brush ((viewId == kCenter) ? centerTextColor : barrelTextColor);
	
	if(viewId == kBarrel)
	{
		// add last elements to front
		if(vertical)
		{
			r.top = getBlankBarrelItemsOffset ();
			r.setHeight (itemSize);
			r.offset (0, -itemSize);
		}
		else
		{
			r.left = getBlankBarrelItemsOffset ();
			r.setWidth (itemSize);
			r.offset (-itemSize, 0);
		}
		
		int valueIndex = numberOfRenderValues - 1;
		
		while(r.top >= 0 && r.left >= 0)
		{
			scrollPickerParam->getString (title, getValueFromIndex (scrollPickerParam, valueIndex));
			
			device->drawString (r, title, textFont, brush, textAlignment);
			
			valueIndex--;
			
			if(vertical)
				r.offset (0, -itemSize);
			else
				r.offset (-itemSize, 0);
		}
		
		// add first elements to back
		if(vertical)
		{
			r.top = getBlankBarrelItemsOffset ();
			r.setHeight (itemSize);
			r.offset (0, itemSize * numberOfRenderValues);
		}
		else
		{
			r.left = getBlankBarrelItemsOffset ();
			r.setWidth (itemSize);
			r.offset (itemSize * numberOfRenderValues, 0);
		}
		
		valueIndex = 0;
		
		while((r.top < bitmapRect.bottom) && (r.left < bitmapRect.right))
		{
			scrollPickerParam->getString (title, getValueFromIndex (scrollPickerParam, valueIndex));
			
			device->drawString (r, title, textFont, brush, textAlignment);
			
			valueIndex++;

			if(vertical)
				r.offset (0, itemSize);
			else
				r.offset (itemSize, 0);
		}
	}
	else
	{
		if(vertical)
		{
			r.top = bitmapRect.bottom - itemSize;
			r.setHeight (itemSize);
		}
		else
		{
			r.left = bitmapRect.right - itemSize;
			r.setWidth (itemSize);
		}
			
		scrollPickerParam->getString (title, getValueFromIndex (scrollPickerParam, 0)); // add first element to back
		device->drawString (r, title, textFont, brush, textAlignment);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant ScrollPickerRenderer::getValueFromIndex (IParameter* scrollPickerParam, int valueIndex) const
{
	if(valueIndex < 0)
		return NumericLimits::kMaxInt;
	else if(valueIndex >= numberOfValues)
		return NumericLimits::kMaxInt;
		
	if((numberOfValues - 1) <= 0)
	{
		ASSERT (false)
		return scrollPickerParam->getValuePlain (0);
	}
	
	return scrollPickerParam->getValuePlain (valueIndex / (float)(numberOfValues - 1));
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPickerRenderer::getDigitTitle (String& string, int valueIndex) const
{
	string.empty ();
	
	switch(valueIndex)
	{
		case 0:
			break;	// empty title;
			
		case 1:
			string.append (CCLSTR ("-"));
			break;
			
		default:
			string.appendIntValue (valueIndex - 2);
			break;
	}
}
