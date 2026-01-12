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
// Filename    : ccl/public/gui/framework/controlscalepainter.cpp
// Description : Control Scale Painter
//
//************************************************************************************************

#include "ccl/public/gui/framework/controlscalepainter.h"

#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/ivisualstyle.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ControlScalePainter
//************************************************************************************************

ControlScalePainter::ControlScalePainter (ITickScale* _scale)
: scale (_scale),
  zoomFactor (1),
  tickColor (Colors::kLtGray),
  textColor (Colors::kLtGray),
  hiliteTickColor (Colors::kLtGray),
  hiliteTextColor (Colors::kLtGray),
  textVOffset (0),
  textHOffset (0),
  hiliteExpand (0),
  reducedScaleText (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlScalePainter::updateStyle (const IVisualStyle& style)
{
	font = style.getTextFont ();
	tickColor = style.getColor ("scale.tickcolor", tickColor);
	textColor = style.getColor ("scale.textcolor", textColor);
	hiliteTickVImage = style.getImage ("scale.hilitetick");
	hiliteTickColor = style.getColor ("scale.hilitetick", tickColor);
	hiliteExpand = style.getMetric ("scale.hiliteexpand", hiliteExpand);
	hiliteTextColor = style.getColor ("scale.hilitetext", textColor);
	textVOffset = style.getMetric ("scale.textvoffset", textVOffset);
	textHOffset = style.getMetric ("scale.texthoffset", textHOffset);
	scalePadding.left = style.getMetric ("scalepadding.left", 0);
	scalePadding.top = style.getMetric ("scalepadding.top", 0);
	scalePadding.right = style.getMetric ("scalepadding.right", 0);
	scalePadding.bottom = style.getMetric ("scalepadding.bottom", 0);
	reducedScaleText = style.getMetric ("scale.reducedtext", reducedScaleText);

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlScalePainter::setOpacity (float alpha)
{
	textColor.setAlphaF (alpha);
	tickColor.setAlphaF (alpha);
	hiliteTickColor.setAlphaF (alpha);
	hiliteTextColor.setAlphaF (alpha);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double ControlScalePainter::getZoomedValue (double value) const
{
	if(zoomFactor < 0)
		return 1.0 + value * zoomFactor;
	else
		return value * zoomFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlScalePainter::drawScaleText (IGraphics& graphics, RectRef size, int options)
{
	if(scale == nullptr)
		return;

	bool isVertical = (options & Styles::kVertical) != 0;

	Coord range = isVertical ? size.getHeight () : size.getWidth ();

	int numTicks = 0;
	int weight = 2; // never letter fine grid 
	while(weight < 4) 
	{
		numTicks = scale->getNumTicks (weight);
		if(numTicks < 1)
			return;
		if(range / numTicks > kMinTickDistance)
			break;
		
		weight++;
	}
		
	SolidBrush textBrush (textColor);
	
	Alignment alignment;
	if(isVertical)
	{
		if((options & Styles::kLeft) != 0)
			alignment.setAlignH (Alignment::kLeft);
		else
			alignment.setAlignH (Alignment::kRight);
	}

	Rect contentSize (size);
	contentSize.left += scalePadding.left;
	contentSize.top += scalePadding.top;
	contentSize.right -= scalePadding.right;
	contentSize.bottom -= scalePadding.bottom;
	Rect textRect (contentSize);
	
	Rect lastDrawnRect;

	Coord height = contentSize.getHeight () - 1;
	Coord width = contentSize.getWidth () - 1;
	int fontSize = ccl_to_int (font.getSize ());

	for(int index = numTicks - 1; index >= 0; index--)
	{
		String label;
		double plainValue;
		bool drawable = true;

		if(isReducedScaleText () && (index % 2 == 1))
			continue;

		if(scale->getTick (plainValue, &label, weight, index))
		{
			if(scale->isHiliteTick (weight, index))
				textBrush.setColor (hiliteTextColor);
			else
				textBrush.setColor (textColor);
			
			double value = getZoomedValue (plainValue);	
			if(isVertical)
			{
				textRect.left = contentSize.left;
				textRect.right = contentSize.right;
				textRect.top = ccl_to_int<Coord> ((1.0 - value) * height + contentSize.top - fontSize * 0.5);
				textRect.setHeight (fontSize);
				
				if(textHOffset || textVOffset)
					textRect.offset (textHOffset, textVOffset);
				
				if(textRect.intersect (contentSize) == false)
					drawable = false;
				else if(textRect.top < contentSize.top)
					textRect.offset (0, contentSize.top - textRect.top);
				else if(textRect.bottom > contentSize.bottom)
				{
					if(textVOffset > 0)
						continue;
					textRect.offset (0, contentSize.bottom - textRect.bottom);
				}
			}
			else
			{
				Coord labelWidth = graphics.getStringWidth (label, font);
				textRect.top = contentSize.top;
				textRect.bottom = contentSize.bottom;
				textRect.left = ccl_to_int<Coord> (value * width + contentSize.left - labelWidth * 0.5);
				textRect.setWidth (labelWidth);

				if(textHOffset || textVOffset)
					textRect.offset (textHOffset, textVOffset);

				if(textRect.intersect (contentSize) == false)
					drawable = false;
				else if(textRect.left < contentSize.left)
					textRect.offset (contentSize.left - textRect.left, 0);
				else if(textRect.right > contentSize.right)
					textRect.offset (contentSize.right - textRect.right, 0);				
			}

			if(drawable && lastDrawnRect.isEmpty () == false && lastDrawnRect.intersect (textRect))
				drawable = false;			

			if(drawable)
			{
				graphics.drawString (textRect, label, font, textBrush, alignment);
				lastDrawnRect = textRect;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlScalePainter::drawTicks (IGraphics& graphics, RectRef size, int options, int weight, Pen::Size penSize)
{
	bool isVertical = (options & Styles::kVertical) != 0;
	Coord width = size.getWidth ();
	Coord height = size.getHeight ();
	if(isVertical)
		height -= 1;
	else
		width -= 1;

	Coord zeroX = size.left;	
	Coord zeroY = size.top;
	Coord range = isVertical ? height : width;

	int numTicks = scale->getNumTicks (weight);
	if(numTicks == 0)
		return false;
	if(range / numTicks <= kMinTickDistance)
		return false;

	Pen pen (tickColor, penSize);

	Coord lastTick = -1;
	bool isHiliteTick = false;

	for(int index = numTicks - 1; index >= 0; index--)
	{
		double plainValue;
		String label;
		if(scale->getTick (plainValue, &label, weight, index))
		{
			if(scale->isHiliteTick (weight, index))
			{
				isHiliteTick = true;
				pen.setColor (hiliteTickColor);
			}
			else
			{
				isHiliteTick = false;
				pen.setColor (tickColor);
			}
			
			double value = getZoomedValue (plainValue);

			if(isVertical)
			{
				Coord y = ccl_to_int<Coord> ((1.f - value) * height + zeroY);
				if(y >= size.top && y <= size.bottom && (lastTick < 0 || ccl_abs (y - lastTick) > kMinTickDistance))
				{
					if(isHiliteTick && hiliteTickVImage)
					{
						Coord offset = hiliteTickVImage->getHeight () / 2;
						ScopedVar<int> yOffset (y, y-offset);

						graphics.drawImage (hiliteTickVImage, Point (zeroX - hiliteExpand, y));
					}
					else if(isHiliteTick)
					{
						graphics.drawLine (Point (zeroX - hiliteExpand, y), Point (width + zeroX + hiliteExpand, y), pen);
					}
					else
					{
						graphics.drawLine (Point (zeroX, y), Point (width + zeroX, y), pen);
					}
					lastTick = y;
				}
			}
			else
			{
				Coord x = ccl_to_int<Coord> (value * width + zeroX);
				if(x >= size.left && x <= size.right && (lastTick < 0 || ccl_abs (x - lastTick) > kMinTickDistance))
				{
					graphics.drawLine (Point (x, zeroY), Point (x, height + zeroY), pen);
					lastTick = x;
				}
			}	
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlScalePainter::drawScaleGrid (IGraphics& graphics, RectRef size, int options)
{
	if(scale == nullptr)
		return;

	for(int weight = 1; weight < 4; weight++)
	{
		if(drawTicks (graphics, size, options, weight, 1))
		{
			if(weight == 1)
				drawTicks (graphics, size, options, 2, 2);
			break;
		}
	}
}

//************************************************************************************************
// ControlGridPainter
//************************************************************************************************

ControlGridPainter::ControlGridPainter (RectRef size, ITickScale* _xScale, ITickScale* _yScale)
: size (size),
  xScale (_xScale),
  yScale (_yScale),
  fineColor (Colors::kWhite), 
  gridColor (Colors::kWhite),
  outlineColor (Colors::kWhite),
  textColor (Colors::kWhite),
  labelWidthX (30),
  labelWidthY (30),
  labelSpacingY (3),
  labelSpacingX (3),
  labelMarginY (0),
  labelShiftTolerance (6),
  labelAlignmentY (Alignment::kLeft),
  tickDistance (0),
  yReverse (false)
{}
									
//////////////////////////////////////////////////////////////////////////////////////////////////

ControlGridPainter::~ControlGridPainter ()
{}
									
//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::setStyle (const IVisualStyle& style)
{
	font = style.getTextFont ();
	
	textColor = style.getColor ("textColor", textColor);
	gridColor = style.getColor ("gridColor", textColor);
	outlineColor = style.getColor ("outlineColor", gridColor);
	fineColor = style.getColor ("fineColor", gridColor);

	labelWidthX = style.getMetric ("labelWidthX", labelWidthX);
	labelWidthY = style.getMetric ("labelWidthY", labelWidthY);
	labelSpacingY = style.getMetric ("labelSpacingY", labelSpacingY);
	labelSpacingX = style.getMetric ("labelSpacingX", labelSpacingX);
	labelMarginY = style.getMetric ("labelMarginY", labelMarginY);
	labelShiftTolerance = style.getMetric ("labelShiftTolerance", labelShiftTolerance);
	labelAlignmentY = style.getOptions("labelY", Alignment::kLeft);
	tickDistance = style.getMetric ("tickDistance", -1);
	labelOffsetY = style.getMetric ("labelOffsetY", 2);
	labelPaddingBottomX = style.getMetric ("labelPaddingBottomX", 2);
	labelOffsetX = style.getMetric ("labelOffsetX", 2);
	
	float alpha = style.getMetric ("opacity", 1.f);
	if(alpha != 1.f)
	{
		textColor.setAlphaF (alpha);
		fineColor.setAlphaF (alpha);
		gridColor.setAlphaF (alpha);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::setXScale (ITickScale* scaleParam)
{
	if(xScale != scaleParam)
		xScale = scaleParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::setYScale (ITickScale* scaleParam)
{
	if(yScale != scaleParam)
		yScale = scaleParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::draw (IGraphics& graphics)
{
	drawXScaleGrid (graphics);
	drawYScaleGrid (graphics);
	drawXScaleText (graphics);
	drawYScaleText (graphics);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::drawXScaleGrid (IGraphics& graphics, bool scaleTicksOnly)
{
	if(xScale == nullptr)
		return;

	Coord width = size.getWidth () /*- 1*/;
	Coord height = size.getHeight ();
	Coord zeroX = size.left;	
	Coord zeroY = size.top;

	int weight;
	int numTicks = 0;
	for(weight = 0; weight < 4; weight++)
	{
		numTicks = xScale->getNumTicks (weight);
		if(numTicks < 1)
			return;
		if(width / numTicks > ((tickDistance > 0) ? tickDistance : ((scaleTicksOnly ? labelWidthX : 6))))
			break;
	}

	weight++;

	numTicks = xScale->getNumTicks (weight);

	double value = 0.;
	#if 1	
	//if(!scaleTicksOnly)
	{
		for(int index = 0; index < numTicks; index++)
		{
			if(xScale->getTick (value, nullptr, weight, index))
			{
				Pen pen (fineColor, 1);
				Coord x = ccl_to_int<Coord> (value * width);
				if(x > 0 && x < width)
					graphics.drawLine (Point (x + zeroX, zeroY), Point (x + zeroX, height + zeroY), pen);
			}
		}
	}	
	#endif

	weight--;
	numTicks = xScale->getNumTicks (weight);
	for(int index = 0; index < numTicks; index++)
	{
		if(xScale->getTick (value, nullptr, weight, index))
		{
			Pen pen (gridColor, 1);
			Coord x = ccl_to_int<Coord> (value * width);
			if((x > 0 && x < width) || (scaleTicksOnly && x >= 0 && x <= width))
				graphics.drawLine (Point (x + zeroX, zeroY), Point (x + zeroX, height + zeroY), pen);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::drawXScaleText (IGraphics& graphics, bool scaleTicksOnly)
{
	if(xScale == nullptr)
		return;

	Coord width = size.getWidth ();
	Coord height = size.getHeight ();
	Coord zeroX = size.left;	
	Coord zeroY = size.top;

	int weight = scaleTicksOnly ? 0 : 1;
	int numTicks = 0;
	for(; weight >= 0; weight--)
	{
		numTicks = xScale->getNumTicks (weight);
		if(numTicks < 1)
			return;
		if(width / numTicks > labelWidthX)
			break;
	}

	if(width / numTicks < labelWidthX)
		return;

	int fontSize = ccl_to_int (font.getSize ());
	Rect textRect (zeroX, height - fontSize + zeroY - labelPaddingBottomX, labelWidthX + zeroX, height + zeroY - labelPaddingBottomX);
	SolidBrush textBrush (textColor);

	double value;
	int lastX = 0;

	for(int index = 0; index < numTicks; index++)
	{
		String label;
		if(xScale->getTick (value, &label, weight, index))
		{
			Coord labelWidth = graphics.getStringWidth (label, font);
			textRect.offset ((int)(value * width) + zeroX - textRect.left + labelOffsetX, 0);
			
			if(textRect.left > size.left && textRect.left > lastX + labelSpacingX && textRect.right + labelSpacingX < width - 2)
			{
				graphics.drawString (textRect, label, font, textBrush);
				lastX = textRect.right;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::drawYScaleGrid (IGraphics& graphics, bool scaleTicksOnly)
{	
	if(yScale == nullptr)
		return;

	Coord width = size.getWidth ();
	Coord height = size.getHeight () - 1;
	Coord zeroX = size.left;	
	Coord zeroY = size.top;
	int fontSize = ccl_to_int (font.getSize ());

	int weight;
	int numTicks = 0;
	for(weight = 0; weight < 4; weight++)
	{
		numTicks = yScale->getNumTicks (weight);
		if(numTicks < 1)
			return;
		if(height / numTicks > ((tickDistance > 0) ? tickDistance : (scaleTicksOnly ? 2 * fontSize + 4 : tickDistance)))
			break;
	}

	weight++;
	numTicks = yScale->getNumTicks (weight);

	double value = 0., value2 = 0.;

	//if(!scaleTicksOnly)
	{
		for(int index = 0; index < numTicks; index++)
		{
			yScale->getTick (value, nullptr, weight, index);

			Pen pen (fineColor, 1);
			Coord y = ccl_to_int<Coord> ((1.0 - value) * height);
			Coord nextY = 0;
			if(index < numTicks - 1)
			{
				yScale->getTick (value2, nullptr, weight, index + 1);
				nextY = ccl_to_int ((1.0 - value2) * height) + zeroY;
			}
			if(y > 0 && y < height && y > fontSize + nextY + 3)
			{
				y = yReverse ? (height - (y + zeroY)) : (y + zeroY);
				graphics.drawLine (Point (zeroX, y), Point (width + zeroX, y), pen);
			}
		}
	}

	weight--;
	numTicks = yScale->getNumTicks (weight);
	for(int index = 0; index < numTicks; index++)
	{
		yScale->getTick (value, nullptr, weight, index);

		Pen pen (gridColor, 1);
		Coord y = ccl_to_int<Coord> ((1.f - value) * height);
		Coord nextY = 0;
		if(index < numTicks - 1)
		{
			yScale->getTick (value2, nullptr, weight, index + 1);
			nextY = ccl_to_int<Coord> ((1.f - value2) * height) + zeroY;
		}

		if(((y > 0 && y < height) || (scaleTicksOnly && y >= 0 && y <= height - 1)) && y > fontSize + nextY + 3)
		{
			y = yReverse ? (height - (y + zeroY)) : (y + zeroY);
			graphics.drawLine (Point (zeroX, y), Point (width + zeroX, y), pen);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlGridPainter::drawYScaleText (IGraphics& graphics)
{
	if(yScale == nullptr)
		return;

	// Coord width = size.getWidth ();
	Coord height = size.getHeight () - 1;
	Coord zeroX = size.left;
	
	if(labelAlignmentY == Alignment::kRight)
		zeroX += (size.getWidth () - labelWidthY - 3); // 3 -> a bit more space to the right
	
	if(labelAlignmentY == Alignment::kCenter)
		zeroX += (size.getWidth () - labelWidthY) / 2;
	
	Coord zeroY = size.top;

	int fontSize = ccl_to_int (font.getSize ());

	int weight;
	int numTicks = 0;
	for(weight = 1; weight >= 0; weight--)
	{
		numTicks = yScale->getNumTicks (weight);
		if(numTicks < 1)
			return;
		if(height / numTicks > 2 * fontSize)
			break;
	}
	if(height / numTicks < 2 * fontSize)
		return;

	Rect textRect (zeroX, zeroY, labelWidthY/*!*/ + zeroX, fontSize + zeroY);
	
	zeroY += labelMarginY;
	int lastY = yReverse ? - (labelSpacingY + labelShiftTolerance) : (height + labelSpacingY + textRect.getHeight () + labelShiftTolerance);
	Coord innerHeight = height - (labelMarginY * 2);
	SolidBrush textBrush (textColor);
	double value;
	
	for(int index = 0; index < numTicks; index++)
	{
		String label;
		yScale->getTick (value, &label, weight, index);
		
		Coord y = ccl_to_int<Coord> ((1.f - value) * innerHeight) + zeroY + labelOffsetY;
		
		if(yReverse)
		{
			y = height - y;
			textRect.offset (0, y - textRect.bottom);
		
			if(textRect.top < lastY + labelSpacingY)
				continue;
		}
		else
		{
			textRect.offset (0, y - textRect.top);
			
			if(textRect.bottom > lastY - labelSpacingY - textRect.getHeight ())
				continue;
		}
		
		int missingTopPoints = size.top - textRect.top;
		missingTopPoints = missingTopPoints < 0 ? 0 : missingTopPoints;
		int missingBottomPoints = textRect.bottom - height;
		missingBottomPoints = missingBottomPoints < 0 ? 0 : missingBottomPoints;
		if(missingTopPoints < labelShiftTolerance && missingBottomPoints < labelShiftTolerance)
		{
			textRect.offset (0, missingTopPoints - missingBottomPoints);
			graphics.drawString (textRect, label, font, textBrush, Alignment (Alignment::kRight));
			lastY = textRect.bottom;
		}
	}
}

