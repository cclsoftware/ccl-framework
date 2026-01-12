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
// Filename    : ccl/gui/theme/renderer/updownboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/updownboxrenderer.h"
#include "ccl/gui/theme/renderer/scrollbarrenderer.h"

#include "ccl/gui/controls/updownbox.h"

using namespace CCL;

//************************************************************************************************
// UpDownButtonRenderer
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (UpDownButton, VisualStyle, "UpDownButtonStyle")
END_VISUALSTYLE_CLASS (UpDownButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

UpDownButtonRenderer::UpDownButtonRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UpDownButtonRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	Rect rect;
	view->getClientRect (rect);
	int mouseState = view->getMouseState ();

	GraphicsPort port (view);

	// background
	view->getTheme ().getPainter ().drawElement (port, rect, ThemeElements::kPushButton, mouseState);

	// triangle
	bool horizontal = view->getStyle ().isHorizontal ();
	int orientation = view->getStyle ().isCustomStyle (Styles::kUpDownButtonBehaviorIncrement)
		? (horizontal ? Alignment::kRight : Alignment::kTop)
		: (horizontal ? Alignment::kLeft : Alignment::kBottom);

	ScrollButtonRenderer::drawTriangleIcon (port, rect, orientation);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

int UpDownButtonRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpDownButtonRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
