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
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/headerviewrenderer.h"

using namespace CCL;

//************************************************************************************************
// HeaderViewRenderer
/** A headerView draws column headers for each column of a column list.
The "backgound" image is draw in each column rectangle, and the column title is drawn in top. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (HeaderView, VisualStyle, "HeaderViewStyle")
	ADD_VISUALSTYLE_METRIC ("spacing")			///< space between two columns
	ADD_VISUALSTYLE_COLOR ("backcolor.spacing")	///< color for the space between two columns
END_VISUALSTYLE_CLASS (HeaderView)

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderViewRenderer::HeaderViewRenderer (VisualStyle* visualStyle)
:	ThemeRenderer (visualStyle)
{
	image = visualStyle->getBackgroundImage ();
	columnSpacing = visualStyle->getMetric ("spacing", 0);
	columnSpacingColor = visualStyle->getColor ("backcolor.spacing", Colors::kBlack);
	columnSizableSpacingColor = visualStyle->getColor ("backcolor.sizable.spacing", columnSpacingColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	GraphicsPort port (view);
	// Theme& theme = view->getTheme ();
	HeaderView* headerView = (HeaderView*)view;
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
			Rect r (0, 0, c->getWidth () - columnSpacing, view->getHeight ());
			r.offset (left);

			drawHeader (port, r, *c, textBrush, font);

			r.left = r.right;
			r.right += columnSpacing;
			port.fillRect (r, SolidBrush (c->canResize () ? columnSizableSpacingColor : columnSpacingColor));

			left += c->getWidth ();
		}

		// fill empty space
		if(left < view->getWidth ()) 
		{
			Rect r;
			r.left = left;
			r.right = view->getWidth ();
			r.bottom = view->getHeight ();
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
		view->getClientRect (r);
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

void HeaderViewRenderer::drawHeader (View* view, GraphicsDevice& port, RectRef r, StringRef label, BrushRef textBrush, FontRef font)
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

		//AntiAliasSetter smoother (graphics);
		graphics.fillTriangle (p, textBrush);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HeaderViewRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderViewRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	return false;
}
