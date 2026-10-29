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
// Filename    : ccl/gui/theme/renderer/dialoggrouprenderer.cpp
// Description : DialogGroup Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/dialoggrouprenderer.h"

#include "ccl/gui/views/dialoggroup.h"

using namespace CCL;

//************************************************************************************************
// DialogGroupRenderer
/** A dialog group draws a background image and optional title.
The style contains 2 images "Image" and "SecondaryImage", that can be selected via the option "primary" or "secondary".
If the DialogGroup has a title, it is drawn at the top edge. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (DialogGroup, VisualStyle, "DialogGroupStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (DialogGroup)
	ADD_TYPELIB_MEMBER_IMAGE ("Image") ///< used when options "primary" is set (or as default)
	ADD_TYPELIB_MEMBER_IMAGE ("SecondaryImage") ///< used when options "secondary" is set
	ADD_TYPELIB_MEMBER_METRIC ("headerheight") ///< height of text rectangle when title is drawn
	ADD_TYPELIB_MEMBER_METRIC ("headergap") ///< a gap between the header and body is left of "headergap" pixels
	ADD_TYPELIB_MEMBER_COLOR ("headerlinecolor") ///< color for simple divider line between header and body when there is no headergap
END_TYPELIB_MEMBERS (DialogGroup)

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogGroupRenderer::DialogGroupRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{
	image = visualStyle->getImage ("Image");
	secondaryImage = visualStyle->getImage ("SecondaryImage");
	headerGap = visualStyle->getMetric ("headergap", 0);
	headerlinecolor = visualStyle->getColor ("headerlinecolor", Color(0,0,0,0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogGroupRenderer::draw (const UpdateRgn& updateRgn)
{
	StyleRef style = owner->getStyle ();
	if(style.isOpaque ())
	{
		GraphicsPort port (owner);

		Rect rect;
		owner->getClientRect (rect);

		IImage* frameImage = nullptr;
		if(style.isCustomStyle (Styles::kDialogGroupAppearanceSecondary))
			frameImage = secondaryImage;
		else
			frameImage = image;

		Coord themeHeaderHeight = owner->getTheme ().getThemeMetric (ThemeElements::kHeaderHeight);
		Coord headerheight = visualStyle->getMetric ("headerheight", themeHeaderHeight);

		if(frameImage)
		{
			if(style.isCustomStyle (Styles::kDialogGroupAppearanceSecondary) == false)
			{
				if(getHeaderGap () > 0)
				{
					Rect headerRect (rect);
					headerRect.bottom = headerheight;
					port.drawImage (frameImage, Rect (0, 0, frameImage->getWidth (), frameImage->getHeight ()), headerRect);
					Rect bodyRect (rect);
					bodyRect.top = headerheight + getHeaderGap ();
					port.drawImage (frameImage, Rect (0, 0, frameImage->getWidth (), frameImage->getHeight ()), bodyRect);
				}
				else
				{
					port.drawImage (frameImage, Rect (0, 0, frameImage->getWidth (), frameImage->getHeight ()), rect);
					if(headerlinecolor.isVisible ())
					{
						Rect bodyRect (rect);
						bodyRect.top = headerheight;
						port.drawLine (bodyRect.getLeftTop (), bodyRect.getRightTop (), Pen (headerlinecolor));
					}
				}
			}
			else
			{
				port.drawImage (frameImage, Rect (0, 0, frameImage->getWidth (), frameImage->getHeight ()), rect);
			}
		}
		else
			port.drawRect (rect, Pen (Colors::kBlack));

		if(owner->getTitle().isEmpty () == false)
		{
			rect.bottom = headerheight;
			port.drawString (rect, owner->getTitle (), visualStyle->getTextFont (), visualStyle->getTextBrush (), visualStyle->getTextAlignment ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DialogGroupRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DialogGroupRenderer::getPartRect (Rect& rect, int partCode)
{
	return false;
}
