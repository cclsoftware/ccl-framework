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
// Filename    : ccl/gui/theme/renderer/labelrenderer.cpp
// Description : Label Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/labelrenderer.h"

#include "ccl/gui/controls/label.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// LabelRenderer
/** A label draws it's title using the style's "textfont", "textcolor", and "textalignment" (for single line text) or "textoptions" (for multiline text). */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Label, VisualStyle, "LabelStyle")
END_VISUALSTYLE_CLASS (Label)

//////////////////////////////////////////////////////////////////////////////////////////////////

LabelRenderer::LabelRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle)
{
	if(visualStyle)
	{
		if(visualStyle->getTextAlignment ().getAlignH () == Alignment::kRight)
			offset.x = -visualStyle->getMetric<Coord> (StyleID::kPaddingRight, 0);
		else
			offset.x = visualStyle->getMetric<Coord> (StyleID::kPaddingLeft, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LabelRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	GraphicsPort port (view);
	StyleRef style = view->getStyle ();
	
	if(style.isOpaque ())
		if(style.isCustomStyle (Styles::kLabelColorize))
			port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());
	
	if(ITextLayout* textLayout = ((Label*)view)->getTextLayout ())
	{
		Rect rect;
		view->getClientRect (rect);
		
		if(style.isVertical ())
		{
			port.saveState ();

			Transform t;
			t.translate ((float)rect.left, (float)rect.bottom);
			t.rotate (Math::degreesToRad (-90.f));

			port.addTransform (t);
		}

		port.drawTextLayout (offset, textLayout, visualStyle->getTextBrush ());

		if(style.isVertical ())
			port.restoreState ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LabelRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LabelRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
