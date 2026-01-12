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
// Filename    : ccl/gui/theme/renderer/compositedrenderer.cpp
// Description : Composited Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/compositedrenderer.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/textlayoutbuilder.h"

#include "ccl/public/gui/framework/iusercontrol.h" // IBackgroundView
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// CompositedRenderer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompositedRenderer, ThemeRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompositedRenderer::CompositedRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  backgroundView (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompositedRenderer::update (View* view, const UpdateInfo& info)
{
	if(view->getStyle ().isDirectUpdate ())
	{
		GraphicsPort port (view);
		RectRef updateRect = port.getVisibleRect ();
		if(updateRect.isEmpty ())
			return;

		if(view->getStyle ().isComposited ())
			drawCompositedBackground (port, view, updateRect);

		draw (view, UpdateRgn (updateRect));

		info.windowInfo->addDirtyRect (updateRect);
	}
	else
		ThemeRenderer::update (view, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompositedRenderer::drawCompositedBackground (IGraphics& graphics, View* view, RectRef rect)
{
	ASSERT (view->getStyle ().isComposited ())

	if(backgroundView == nullptr)
	{
		position = Point (view->getSize ().getLeftTop ());
		for(View* v = view->getParent (); v != nullptr; v = v->getParent ())
		{
			UnknownPtr<IBackgroundView> bgView (v->asUnknown ());
			if(bgView && bgView->canDrawControlBackground ())
			{
				backgroundView = bgView;
				break;
			}
			position.offset (v->getSize ().getLeftTop ());	
		}
	}

	ASSERT (backgroundView != nullptr)
	if(backgroundView)
	{
		Rect r (rect);
		r.offset (position);
		backgroundView->drawControlBackground (graphics, r, Point (-position.x, -position.y));
	}

	#if (0 && DEBUG)
	graphics.fillRect (rect, SolidBrush (Colors::kGreen));
	#endif
}

//************************************************************************************************
// TextScaler
//************************************************************************************************

TextScaler::TextScaler ()
: cachedFontSize (0),
  explicitMaximalFontSize (100),
  explicitMinimalFontSize (6)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextScaler::scaleTextFont (Font& font, RectRef r, StringRef text, int options)
{
	if(text == cachedText && r == cachedRect)
	{
		font.setSize (cachedFontSize);
		return;
	}

	float upperBound = ccl_min (explicitMaximalFontSize, (r.getHeight () * 0.75f));
	float lowerBound = explicitMinimalFontSize;
	ASSERT (upperBound >= lowerBound)
	if(upperBound < lowerBound)
		ccl_swap (upperBound, lowerBound);

	float fontSize = ccl_bound (font.getSize (), lowerBound, upperBound);
	font.setSize (fontSize);

	while(true)
	{
		Rect size;
		if(options & kMarkupText)
			MarkupPainter ().measureMarkupString (size, text, font, ITextLayout::kNoMargin);
		else
			Font::measureString (size, text, font);

		if(r.getWidth () == size.getWidth ())
			break;

		float newFontSize = fontSize;
		if(r.getWidth () > size.getWidth ())
		{
			lowerBound = fontSize;
			newFontSize = ccl_round<2> ((fontSize + upperBound) / 2.f);
		}
		else
		{
			upperBound = fontSize;
			newFontSize = ccl_round<2> ((fontSize + lowerBound) / 2.f);
		}

		if(ccl_equals (fontSize, newFontSize, .1f))
			break;

		fontSize = ccl_round<2> (newFontSize);
		font.setSize (fontSize);
	}

	cachedRect = r;
	cachedText = text;
	cachedFontSize = font.getSize ();
}
