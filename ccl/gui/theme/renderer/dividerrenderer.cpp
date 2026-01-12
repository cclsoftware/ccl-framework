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
// Filename    : ccl/gui/theme/renderer/dividerrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/dividerrenderer.h"

#include "ccl/gui/layout/divider.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// DividerRenderer
/** A divider draws a background image, and optionally a handle centered on top. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Divider, VisualStyle, "DividerStyle")
	ADD_VISUALSTYLE_IMAGE  ("hImage")	///< background for a horizontal divider
	ADD_VISUALSTYLE_IMAGE  ("vImage")	///< background for a vertical divider
	ADD_VISUALSTYLE_IMAGE  ("hGrip")	///< handle for a horizontal divider
	ADD_VISUALSTYLE_IMAGE  ("vGrip")	///< handle for a vertical divider
END_VISUALSTYLE_CLASS (Divider)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerRenderer::DividerRenderer (VisualStyle* visualStyle)
:	ThemeRenderer (visualStyle)
{
	hImage = visualStyle->getImage ("hImage");
	vImage = visualStyle->getImage ("vImage");
	hGrip  = visualStyle->getImage ("hGrip");
	vGrip  = visualStyle->getImage ("vGrip");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(view->getStyle ().isTransparent ())
		return;

	GraphicsPort port (view);
	Rect rect;
	view->getClientRect (rect);

	auto selectFrame = [view](IImage* image)
	{
		MutableCString frame;

		if(view->getMouseState () == ThemeElements::kPressed)
			frame = ThemeNames::kPressed;
		else if(view->getMouseState () == ThemeElements::kMouseOver)
			frame = ThemeNames::kMouseOver;
		else
			frame = ThemeNames::kNormal;

		IImage::Selector (image, frame);
	};

	IImage* image = view->getStyle ().isVertical () ? hImage : vImage;
	if(image)
	{
		selectFrame (image);
		Rect src (0, 0, image->getWidth (), image->getHeight ());
		port.drawImage (image, src, rect);
	}
	else
		port.fillRect (rect, SolidBrush (visualStyle->getBackColor ()));

	image = view->getStyle ().isVertical () ? hGrip : vGrip;
	if(image)
	{
		selectFrame (image);
		Rect src (0, 0, image->getWidth (), image->getHeight ());
		Rect dst (0, 0, image->getWidth (), image->getHeight ());		
		dst.center (rect);
		port.drawImage (image, src, dst);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DividerRenderer::needsRedraw (View* view, const Point& sizeDelta)
{
	if(view->getStyle ().isTransparent () || sizeDelta.isNull ())
		return false;

	return (view->getStyle ().isVertical () ? hGrip : vGrip) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DividerRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DividerRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
