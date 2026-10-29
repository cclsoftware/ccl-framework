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

DEFINE_TYPELIB_ENTRY_WITH_PARENT (Label, VisualStyle, "LabelStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

LabelRenderer::LabelRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{
	if(visualStyle)
	{
		if(visualStyle->getTextAlignment ().getAlignH () == Alignment::kRight)
			offset.x = -visualStyle->getMetric<Coord> (StyleID::kPaddingRight, 0);
		else if(visualStyle->getTextAlignment ().getAlignH () == Alignment::kLeft)
			offset.x = visualStyle->getMetric<Coord> (StyleID::kPaddingLeft, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LabelRenderer::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort port (owner);
	StyleRef style = owner->getStyle ();
	
	if(style.isOpaque ())
	{
		if(style.isCustomStyle (Styles::kLabelColorize))
			port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());
	}

	if(ITextLayout* textLayout = (static_cast<Label*> (owner))->getTextLayout ())
	{
		Rect rect;
		owner->getClientRect (rect);

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

int LabelRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LabelRenderer::getPartRect (Rect& rect, int partCode)
{
	return false;
}
