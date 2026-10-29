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

#include "ccl/public/gui/framework/iusercontrol.h" // IBackgroundView

using namespace CCL;

//************************************************************************************************
// CompositedRenderer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompositedRenderer, ThemeRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompositedRenderer::CompositedRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner),
  backgroundView (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompositedRenderer::update (const UpdateInfo& info)
{
	if(owner->getStyle ().isDirectUpdate ())
	{
		GraphicsPort port (owner);
		RectRef updateRect = port.getVisibleRect ();
		if(updateRect.isEmpty ())
			return;

		if(owner->getStyle ().isComposited ())
			drawCompositedBackground (port, updateRect);

		draw (UpdateRgn (updateRect));

		info.windowInfo->addDirtyRect (updateRect);
	}
	else
		ThemeRenderer::update (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompositedRenderer::drawCompositedBackground (IGraphics& graphics, RectRef rect)
{
	ASSERT (owner->getStyle ().isComposited ())

	if(backgroundView == nullptr)
	{
		position = Point (owner->getSize ().getLeftTop ());
		for(View* v = owner->getParent (); v != nullptr; v = v->getParent ())
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
