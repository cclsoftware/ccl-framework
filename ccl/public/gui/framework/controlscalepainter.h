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
// Filename    : ccl/public/gui/framework/controlscalepainter.h
// Description : Control Scale Painter
//
//************************************************************************************************

#ifndef _ccl_controlscalepainter_h
#define _ccl_controlscalepainter_h

#include "ccl/public/gui/graphics/types.h"

#include "ccl/public/gui/iparameter.h"

namespace CCL {

interface IVisualStyle;

//************************************************************************************************
// ControlScalePainter
/** 
\ingroup gui */
//************************************************************************************************

class ControlScalePainter
{
public:
	ControlScalePainter (ITickScale* curve = nullptr);
	
	ITickScale* getScale () const { return scale; }
	void setScale (ITickScale* s) { scale = s;}

	PROPERTY_VARIABLE (double, zoomFactor, ZoomFactor)
	PROPERTY_VARIABLE (Color, tickColor, TickColor)
	PROPERTY_VARIABLE (Color, textColor, TextColor)
	PROPERTY_VARIABLE (Color, hiliteTickColor, HiliteTickColor)
	PROPERTY_VARIABLE (Color, hiliteTextColor, HiliteTextColor)
	PROPERTY_VARIABLE (int, textVOffset, TextVOffset)
	PROPERTY_VARIABLE (int, textHOffset, TextHOffset)
	PROPERTY_VARIABLE (int, hiliteExpand, HiliteExpand)
	PROPERTY_VARIABLE (Rect, scalePadding, ScalePadding)
	PROPERTY_BOOL (reducedScaleText, ReducedScaleText)
	PROPERTY_OBJECT (Font, font, Font)

	void updateStyle (const IVisualStyle& style);
	void setOpacity (float alpha);

	void drawScaleText (IGraphics& graphics, RectRef size, int options); //text
	void drawScaleGrid (IGraphics& graphics, RectRef size, int options); //grid

private:
	double getZoomedValue (double value) const;
	bool drawTicks (IGraphics& graphics, RectRef size, int options, int weight, Pen::Size penSize);

	SharedPtr<ITickScale> scale;
	SharedPtr<IImage> hiliteTickVImage;
	
	static const Coord kMinTickDistance = 5;
};

//************************************************************************************************
// ControlGridPainter
/** 
\ingroup gui */
//************************************************************************************************

class ControlGridPainter
{
public:
	ControlGridPainter (RectRef size, ITickScale* xScale = nullptr, ITickScale* yScale = nullptr);
	~ControlGridPainter ();

	void setStyle (const IVisualStyle& style);

	void setXScale (ITickScale* scaleParam);
	void setYScale (ITickScale* scaleParam);
	ITickScale* getXScale () { return xScale; }
	ITickScale* getYScale () { return yScale; }

	void draw (IGraphics& graphics);

	void drawXScaleGrid (IGraphics& graphics, bool scaleTicksOnly = false);
	void drawXScaleText (IGraphics& graphics, bool scaleTicksOnly = false);
	void drawYScaleGrid (IGraphics& graphics, bool scaleTicksOnly = false);
	void drawYScaleText (IGraphics& graphics);

	virtual void setSize (RectRef newSize) { size = newSize; }
	RectRef getSize () const { return size; }

	PROPERTY_VARIABLE (Color, fineColor, FineColor)
	PROPERTY_VARIABLE (Color, gridColor, GridColor)
	PROPERTY_VARIABLE (Color, outlineColor, OutLineColor)
	PROPERTY_VARIABLE (Color, textColor, TextColor)
	PROPERTY_OBJECT (Font, font, Font)
	//TODO: margin
	
	PROPERTY_VARIABLE (int, labelWidthX, LabelWidthX)
	PROPERTY_VARIABLE (int, labelWidthY, LabelWidthY)
	PROPERTY_VARIABLE (int, labelSpacingX, LabelSpacingX)
	PROPERTY_VARIABLE (int, labelSpacingY, LabelSpacingY)
	PROPERTY_VARIABLE (int, labelMarginY, LabelMarginY)
	PROPERTY_VARIABLE (int, labelShiftTolerance, LabelShiftTolerance)
	PROPERTY_VARIABLE (int, labelAlignmentY, LabelAlignmentY)
	PROPERTY_VARIABLE (int, tickDistance, TickDistance)
	PROPERTY_VARIABLE (int, labelOffsetY, LabelOffsetY)
	PROPERTY_VARIABLE (int, labelOffsetX, LabelOffsetX)
	PROPERTY_VARIABLE (int, labelPaddingBottomX, LabelPaddingBottomX)
	

	PROPERTY_BOOL (yReverse, YReverse)
	
protected:
	Rect size;
	
	SharedPtr<ITickScale> xScale;
	SharedPtr<ITickScale> yScale;
};

} // namespace CCL

#endif // _ccl_controlclasses_h
