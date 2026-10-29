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
// Description : Divider Renderer
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

DEFINE_TYPELIB_ENTRY_WITH_PARENT (Divider, VisualStyle, "DividerStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (Divider)
	ADD_TYPELIB_MEMBER_IMAGE ("hImage") ///< background for a horizontal divider
	ADD_TYPELIB_MEMBER_IMAGE ("vImage") ///< background for a vertical divider
	ADD_TYPELIB_MEMBER_IMAGE ("hGrip") ///< handle for a horizontal divider
	ADD_TYPELIB_MEMBER_IMAGE ("vGrip") ///< handle for a vertical divider
END_TYPELIB_MEMBERS (Divider)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerRenderer::DividerRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{
	hImage = visualStyle->getImage ("hImage");
	vImage = visualStyle->getImage ("vImage");
	hGrip  = visualStyle->getImage ("hGrip");
	vGrip  = visualStyle->getImage ("vGrip");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerRenderer::draw (const UpdateRgn& updateRgn)
{
	if(owner->getStyle ().isTransparent ())
		return;

	GraphicsPort port (owner);
	Rect rect;
	owner->getClientRect (rect);

	auto selectFrame = [this](IImage* image)
	{
		MutableCString frame;

		if(owner->getMouseState () == ThemeElements::kPressed)
			frame = ThemeNames::kPressed;
		else if(owner->getMouseState () == ThemeElements::kMouseOver)
			frame = ThemeNames::kMouseOver;
		else
			frame = ThemeNames::kNormal;

		IImage::Selector (image, frame);
	};

	IImage* image = owner->getStyle ().isVertical () ? hImage : vImage;
	if(image)
	{
		selectFrame (image);
		Rect src (0, 0, image->getWidth (), image->getHeight ());
		port.drawImage (image, src, rect);
	}
	else
		port.fillRect (rect, SolidBrush (visualStyle->getBackColor ()));

	image = owner->getStyle ().isVertical () ? hGrip : vGrip;
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

bool DividerRenderer::needsRedraw (const Point& sizeDelta)
{
	if(owner->getStyle ().isTransparent () || sizeDelta.isNull ())
		return false;

	return (owner->getStyle ().isVertical () ? hGrip : vGrip) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DividerRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DividerRenderer::getPartRect (Rect& rect, int partCode)
{
	return false;
}
