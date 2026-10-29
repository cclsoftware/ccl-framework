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
// Description : UpDownButton Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/updownboxrenderer.h"
#include "ccl/gui/theme/renderer/scrollbarrenderer.h"

#include "ccl/gui/controls/updownbox.h"

using namespace CCL;

//************************************************************************************************
// UpDownButtonRenderer
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (UpDownButton, VisualStyle, "UpDownButtonStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

UpDownButtonRenderer::UpDownButtonRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UpDownButtonRenderer::draw (const UpdateRgn& updateRgn)
{
	Rect rect;
	owner->getClientRect (rect);
	int mouseState = owner->getMouseState ();

	GraphicsPort port (owner);

	// background
	owner->getTheme ().getPainter ().drawElement (port, rect, ThemeElements::kPushButton, mouseState);

	// triangle
	bool horizontal = owner->getStyle ().isHorizontal ();
	int orientation = owner->getStyle ().isCustomStyle (Styles::kUpDownButtonBehaviorIncrement)
		? (horizontal ? Alignment::kRight : Alignment::kTop)
		: (horizontal ? Alignment::kLeft : Alignment::kBottom);

	ScrollButtonRenderer::drawTriangleIcon (port, rect, orientation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UpDownButtonRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpDownButtonRenderer::getPartRect (Rect& rect, int partCode)
{
	return false;
}
