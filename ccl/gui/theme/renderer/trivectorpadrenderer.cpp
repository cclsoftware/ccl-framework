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
// Filename    : ccl/gui/theme/renderer/trivectorpadrenderer.cpp
// Description : Triangular Vector Pad Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/trivectorpadrenderer.h"
#include "ccl/gui/controls/trivectorpad.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"

using namespace CCL;

//************************************************************************************************
// TriVectorPadRenderer
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (TriVectorPad, VisualStyle, "TriVectorPadStyle")
	ADD_VISUALSTYLE_IMAGE  ("peakpadding.left")		///< left padding for triangle peak
	ADD_VISUALSTYLE_IMAGE  ("background")			///< background image
	ADD_VISUALSTYLE_IMAGE  ("triangle")				///< triangle image
	ADD_VISUALSTYLE_IMAGE  ("handle")				///< handle image
	ADD_VISUALSTYLE_COLOR  ("backcolor")			///< backcolor when no image is provided
	ADD_VISUALSTYLE_COLOR  ("triangleColor")		///< color of the triangle when no image is provided 
	ADD_VISUALSTYLE_COLOR  ("handleColor")			///< color for the handle when no image is provided
	ADD_VISUALSTYLE_COLOR  ("snapPointColor")		///< color for the snap point highlights
	ADD_VISUALSTYLE_METRIC ("padding.left")			///< left padding for triangle base
	ADD_VISUALSTYLE_METRIC ("padding.top")			///< top padding triangle peak (or base if upside down) 
	ADD_VISUALSTYLE_METRIC ("padding.right")		///< right padding for triangle base
	ADD_VISUALSTYLE_METRIC ("padding.bottom")		///< bottom padding triangle base (or peak if upside down)
	ADD_VISUALSTYLE_METRIC ("padding")				///< padding fallback
	ADD_VISUALSTYLE_METRIC ("handlesize")			///< size where handle can be clicked (in pixels) when handle is drawn as a circle (no "handle" image)
	ADD_VISUALSTYLE_METRIC ("snapsize")				///< size of the snappoint at corner (or at the edge-center when "invert" option is set) 
	ADD_VISUALSTYLE_METRIC ("handlethickness")		///< width of pen in pixels when handle is drawn as a circle (no "handle" image)
END_VISUALSTYLE_CLASS (TriVectorPad)


//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPadRenderer::TriVectorPadRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  handleSize (6),
  snapPointSize (6),
  handleThickness (2)
{
	background = visualStyle->getImage ("background");
	triangleImage = visualStyle->getImage ("triangle");
	handleImage = visualStyle->getImage ("handle");
	
	backcolor = visualStyle->getColor ("backcolor");
	triangleColor = visualStyle->getColor ("triangleColor", Colors::kGray);
	handleColor = visualStyle->getColor ("handleColor", Colors::kGreen);
	snapPointColor = visualStyle->getColor ("snapPointColor", Colors::kWhite);

	visualStyle->getPadding (padding);
	
	hoverPadding.left = visualStyle->getMetric<Coord> ("hoverpadding.left", padding.left);
	hoverPadding.top = visualStyle->getMetric<Coord> ("hoverpadding.top", padding.top);
	hoverPadding.right = visualStyle->getMetric<Coord> ("hoverpadding.right", padding.right);
	hoverPadding.bottom = visualStyle->getMetric<Coord> ("hoverpadding.bottom", padding.bottom);
	
	handleSize = visualStyle->getMetric<int> ("handlesize", handleSize);
	snapPointSize = visualStyle->getMetric<int> ("snapsize", snapPointSize);
	snapPointImage = visualStyle->getImage ("snapPointImage");
	if(snapPointImage)
	{
		snapPointSize = snapPointImage->getWidth ();
		if(snapPointSize > snapPointImage->getHeight ())
			snapPointSize = snapPointImage->getHeight ();
	}
	
	handleThickness = visualStyle->getMetric ("handlethickness", handleThickness);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	switch(partCode)
	{
	case TriVectorPad::kPartHandle:
		return getHandleRect (view, rect);
	case TriVectorPad::kPartTriangle:
		return getTriangleRect (view, rect);
	case TriVectorPad::kPartHoverTriangle:
		return getHoverTriangleRect (view, rect);
	default:
		return getSnapPointRect (view, partCode, rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadRenderer::getHandleRect (const View* view, Rect& rect) const
{
	if(view->isEnabled () == false)
		return false;
	
	const TriVectorPad* pad = static_cast<const TriVectorPad*> (view);
	
	Point center (pad->getHandlePosition ());
	
	if(handleImage)
	{
		rect.left = center.x - int (handleImage->getWidth () * 0.5f);
		rect.right = rect.left + handleImage->getWidth ();
		rect.top = center.y - int (handleImage->getHeight () * 0.5f);
		rect.bottom = rect.top + handleImage->getHeight ();
	}
	else
	{
		rect.left = center.x + (int)(-handleSize * 0.5f);
		rect.top = center.y + (int)(-handleSize * 0.5f);
		rect.right = center.x + ccl_to_int(handleSize * 0.5f);
		rect.bottom = center.y + ccl_to_int(handleSize * 0.5f);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadRenderer::getTriangleRect (const View* view, Rect& rect) const
{
	Rect triangleRect;
	view->getClientRect (triangleRect);

	triangleRect.left += padding.left;
	triangleRect.top += padding.top;
	triangleRect.right -= padding.right;
	triangleRect.bottom -= padding.bottom;
	rect = triangleRect;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadRenderer::getHoverTriangleRect (const View* view, Rect& rect) const
{
	Rect triangleRect;
	view->getClientRect (triangleRect);
	
	triangleRect.left += hoverPadding.left;
	triangleRect.top += hoverPadding.top;
	triangleRect.right -= hoverPadding.right;
	triangleRect.bottom -= hoverPadding.bottom;
	rect = triangleRect;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadRenderer::getSnapPointRect (const View* view, int partCode, Rect& rect) const
{
	const TriVectorPad* pad = static_cast<const TriVectorPad*> (view);

	Point center (pad->getSnapPoint (partCode));
	
	rect.left = center.x + (int)(-snapPointSize * 0.5f);
	rect.top = center.y + (int)(-snapPointSize * 0.5f);
	rect.right = center.x + ccl_to_int(snapPointSize * 0.5f);
	rect.bottom = center.y + ccl_to_int(snapPointSize * 0.5f);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPadRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	Rect triangleRect;
	getTriangleRect (view, triangleRect);

	GraphicsPort port (view);
	
	Rect dst;
	view->getClientRect (dst);
	
	if(background)
	{
		Rect src (0, 0, background->getWidth (), background->getHeight ());
		port.drawImage (background, src, dst);
	}
	else
	{
		port.fillRect (updateRgn.bounds, SolidBrush (backcolor));
	}

	// draw triangle
	if(triangleImage)
	{
		Rect src (0, 0, triangleImage->getWidth (), triangleImage->getHeight ());
		port.drawImage (triangleImage, src, dst);
	}
	else
	{
		drawTriangleShape (view);
	}
	
	// draw handle
	Rect handleRect;
	if(getHandleRect (view, handleRect))
	{
		if(handleImage)
		{
			StringID frame = view->isMouseDown () ? ThemeNames::kPressed : ThemeNames::kNormal;
			handleImage->setCurrentFrame (handleImage->getFrameIndex (frame));

			Rect src (0, 0, handleImage->getWidth (), handleImage->getHeight ());
			port.drawImage (handleImage, src, handleRect);
		}
		else
			port.drawEllipse (handleRect, Pen (handleColor, handleThickness));
	}
	
	TriVectorPad* pad = static_cast<TriVectorPad*> (view);
	int snapPointCode = pad->getHighlightSnapPointCode ();
	
	if(snapPointCode != -1)
	{
		Rect rect;
		getSnapPointRect (view, snapPointCode, rect);
		
		if(snapPointImage)
		{
			snapPointImage->setCurrentFrame (getFrameForSnapPointCode (snapPointCode));
			Rect src (0, 0, snapPointImage->getWidth (), snapPointImage->getHeight ());
			port.drawImage (snapPointImage, src, rect);
		}
		else
		{
			port.fillEllipse (rect, SolidBrush (snapPointColor));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TriVectorPadRenderer::getFrameForSnapPointCode (int code) const
{
	switch(code)
	{
	case TriVectorPad::kPartSnapPointA: return 0;
	case TriVectorPad::kPartSnapPointB: return 1;
	case TriVectorPad::kPartSnapPointC: return 2;
	case TriVectorPad::kPartSnapPointAB: return 3;
	case TriVectorPad::kPartSnapPointBC: return 4;
	case TriVectorPad::kPartSnapPointCA: return 5;
	default: return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TriVectorPadRenderer::hitTest (View* view, const Point& loc, Point* offset)
{	
	// check part codes: from handle to triangleRect
	Rect rect;
	for(int i = 0; i < TriVectorPad::kNumPartCodes; i++)
		if(getPartRect (view, i, rect))
			if(rect.pointInside (loc))
				return i; 
				
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPadRenderer::drawTriangleShape (View* view)
{
	if(AutoPtr<IGraphicsPath> path = GraphicsFactory::createPath ())
	{
		GraphicsPort port (view);
	
		TriVectorPad* pad = static_cast<TriVectorPad*> (view);
		
		path->startFigure (pad->getTrianglePoint (TriVectorPad::kCornerA));
		path->lineTo (pad->getTrianglePoint (TriVectorPad::kCornerB));
		path->lineTo (pad->getTrianglePoint (TriVectorPad::kCornerC));
		path->closeFigure ();
	
		Rect dst;
		view->getClientRect (dst);
		port.fillRect (dst, SolidBrush (Colors::kTransparentBlack));
		port.fillPath (path, SolidBrush (triangleColor));
	}
}
