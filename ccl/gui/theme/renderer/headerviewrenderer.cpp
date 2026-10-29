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
// Filename    : ccl/gui/theme/renderer/headerviewrenderer.cpp
// Description : HeaderView Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/headerviewrenderer.h"

using namespace CCL;

//************************************************************************************************
// HeaderViewRenderer
/** A headerView draws column headers for each column of a column list.
The "backgound" image is draw in each column rectangle, and the column title is drawn in top. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (HeaderView, VisualStyle, "HeaderViewStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (HeaderView)
	ADD_TYPELIB_MEMBER_METRIC ("spacing") ///< space between two columns
	ADD_TYPELIB_MEMBER_COLOR ("backcolor.spacing") ///< color for the space between two columns
END_TYPELIB_MEMBERS (HeaderView)

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderViewRenderer::HeaderViewRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{
	image = visualStyle->getBackgroundImage ();
	columnSpacing = visualStyle->getMetric ("spacing", 0);
	columnSpacingColor = visualStyle->getColor ("backcolor.spacing", Colors::kBlack);
	columnSizableSpacingColor = visualStyle->getColor ("backcolor.sizable.spacing", columnSpacingColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewRenderer::draw (const UpdateRgn& updateRgn)
{	
	GraphicsPort port (owner);
	HeaderView* headerView = static_cast<HeaderView*> (owner);
	ColumnHeaderList* columnList = headerView->getColumnHeaders ();

	IImage::Selector (image, IImage::kNormal);
	
	Font font (visualStyle->getTextFont ());
	SolidBrush textBrush (visualStyle->getTextBrush ());
	int left = 0;
	if(columnList)
	{
		for(int i = 0; i < columnList->getCount (false); i++)
		{
			ColumnHeader* c = columnList->getColumnAtPosition (i, false);
			if(c->isHidden ())
				continue;
		
			// draw item background
			Rect r (0, 0, c->getWidth () - columnSpacing, owner->getHeight ());
			r.offset (left);

			drawHeader (port, r, *c, textBrush, font);

			r.left = r.right;
			r.right += columnSpacing;
			port.fillRect (r, SolidBrush (c->canResize () ? columnSizableSpacingColor : columnSpacingColor));

			left += c->getWidth ();
		}

		// fill empty space
		if(left < owner->getWidth ()) 
		{
			Rect r;
			r.left = left;
			r.right = owner->getWidth ();
			r.bottom = owner->getHeight ();
			if(image)
			{
				IImage::Selector (image, ThemeNames::kNormal);
				port.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), r); 
			}
			else
				port.fillRect (r, visualStyle->getBackBrush ());
		}
	}
	else
	{
		Rect r;
		owner->getClientRect (r);
		if(image)
		{
			IImage::Selector (image, ThemeNames::kNormal);
			port.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), r); 
		}
		else
			port.drawRect (r, Pen (Colors::kBlack));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewRenderer::drawHeader (GraphicsDevice& port, RectRef r, StringRef label, BrushRef textBrush, FontRef font)
{
	if(image)
	{
		IImage::Selector (image, ThemeNames::kNormal);
		port.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), r); 
	}
	else
	{
		port.fillRect (r, visualStyle->getBackBrush ());
	}
			
	// draw item title
	if(!label.isEmpty ())
	{
		Rect titleRect (r);
		titleRect.left += kTextInset;
		port.drawString (titleRect, label, font, textBrush, Alignment::kLeft|Alignment::kHCenter); 
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewRenderer::drawHeader (IGraphics& graphics, RectRef headerRect, ColumnHeader& c, BrushRef textBrush, FontRef font)
{
	if(image)
	{
		IImage::Selector (image, c.isSorted () ? ThemeNames::kPressed : ThemeNames::kNormal);
		graphics.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), headerRect); 
	}
	else
	{
		graphics.fillRect (headerRect, c.isSorted () ? visualStyle->getForeBrush () : visualStyle->getBackBrush ());
	}
						
	// draw item title
	if(!c.getTitle ().isEmpty ())
	{
		Rect titleRect (headerRect);
		if(c.drawCentered () == false)
			titleRect.left += kTextInset;
		
		Alignment align = (c.drawCentered () ?  Alignment::kHCenter : Alignment::kLeft) | Alignment::kVCenter;
		graphics.drawString (titleRect, c.getTitle (), font, textBrush, align); 
	}

	if(c.isSorted ())
	{
		Rect iconRect (0, 0, 8, c.isSortedUp () ? 5 : 4);
		iconRect.centerH (headerRect);
		iconRect.offset (0, headerRect.top + 1);

		Point p[3];
		if(c.isSortedUp ())
		{
			p[0] = iconRect.getLeftBottom ();
			p[1] = Point (iconRect.getCenter ().x, iconRect.top);
			p[2] = iconRect.getRightBottom ();
		}
		else
		{
			p[0] = iconRect.getLeftTop ();
			p[1] = iconRect.getRightTop ();
			p[2] = Point (iconRect.getCenter ().x, iconRect.bottom);
		}

		graphics.fillTriangle (p, textBrush);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HeaderViewRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderViewRenderer::getPartRect (Rect& rect, int partCode)
{
	return false;
}
