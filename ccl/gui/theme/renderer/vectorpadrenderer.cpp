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
// Filename    : ccl/gui/theme/renderer/vectorpadrenderer.cpp
// Description : VectorPad Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/vectorpadrenderer.h"
#include "ccl/public/gui/iparameter.h"

#include "ccl/gui/controls/vectorpad.h"

using namespace CCL;

//************************************************************************************************
// VectorPadRenderer
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (VectorPad, VisualStyle, "VectorPadStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (VectorPad)
	ADD_TYPELIB_MEMBER_IMAGE ("background")			///< background image
	ADD_TYPELIB_MEMBER_IMAGE ("handle")				///< handle
	ADD_TYPELIB_MEMBER_METRIC ("handlesize")		///< size where handle can be clicked (in pixels) when handle is drawn as a circle (no "handle" image)
	ADD_TYPELIB_MEMBER_METRIC ("handlethickness")	///< width of line in pixels) when handle is drawn as a circle (no "handle" image)
	ADD_TYPELIB_MEMBER_METRIC ("drawlabels")		///< draws labels with min / max values for both parameters
	ADD_TYPELIB_MEMBER_COLOR ("backcolor")			///< used when "back" image is not available
	ADD_TYPELIB_MEMBER_COLOR ("handlecolor")		///< used when "handle" image is not available
	ADD_TYPELIB_MEMBER_COLOR ("textcolor")			///< used for "drawlabels"
	ADD_TYPELIB_MEMBER_COLOR ("crosshair")			///< draw crosshair
	ADD_TYPELIB_MEMBER_COLOR ("crosshair.color")	///< color for crosshair
	ADD_TYPELIB_MEMBER_COLOR ("reference.color")	///< used to draw the second frame of the "background" image, when the luminacnce of this "reference.color" is > 0.5f
END_TYPELIB_MEMBERS (VectorPad)

//////////////////////////////////////////////////////////////////////////////////////////////////

VectorPadRenderer::VectorPadRenderer (VisualStyle* visualStyle, View* owner)
: ThemeRenderer (visualStyle, owner)
{
	back = visualStyle->getImage ("background");
	if(!back)
		back = visualStyle->getImage ("back");	// look for legacy backgrounds
	
	handle = visualStyle->getImage ("handle");

	backColor = visualStyle->getColor ("backcolor", Colors::kBlack);
	handleColor = visualStyle->getColor ("handlecolor", Colors::kWhite);
	
	textColor = visualStyle->getColor ("textcolor", Colors::kWhite);
	font = visualStyle->getTextFont ();
	
	drawCrosshair = visualStyle->getMetric ("crosshair", false);
	crosshairColor = visualStyle->getColor ("crosshair.color", Colors::kBlack);
	referenceBackcolor = visualStyle->getColor ("reference.color", Color (0,0,0,0));
	
	handleSize = (int)visualStyle->getMetric ("handlesize");
	if(handleSize == 0)
		handleSize = 6;
	handleThickness = visualStyle->getMetric ("handlethickness");
	if(handleThickness == 0)
		handleThickness = 2;

	drawLabels = (visualStyle->getMetric ("drawlabels") != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPadRenderer::selectBackgroundFrame ()
{
	if(back->getFrameCount () > 1)
	{
		if(referenceBackcolor.isVisible () && referenceBackcolor.getLuminance () > 0.5f)
			back->setCurrentFrame (1);
		else
			back->setCurrentFrame (0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPadRenderer::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort port (owner);
	
	//background

	Rect dst;
	owner->getClientRect (dst);
	Rect crosshairBounds (dst);

	if(back)
	{
		selectBackgroundFrame ();
		
		Rect src (0, 0, back->getWidth (), back->getHeight ());
		port.drawImage (back, src, dst);
	}
	else
	{
		port.fillRect (updateRgn.bounds, SolidBrush (backColor));
	}

	// labels 
	if(drawLabels)
	{
		SolidBrush textBrush (textColor);
		int fontSize = (int)(font.getSize () + 0.5f);
		String label;

		IParameter* param = ((VectorPad*)owner)->getParameter ();
		param->getString (label, param->getMin ());
		int textWidth = port.getStringWidth (label, font);
		Rect textRect (0, 0, textWidth + 1, fontSize);
		textRect.offset (1, int (0.5f * (owner->getHeight () - fontSize)));
		port.drawText (textRect, label, font, textBrush);
	
		crosshairBounds.left = textRect.right + 2;

		param->getString (label, param->getMax ());
		textWidth = port.getStringWidth (label, font);
		textRect.setWidth (textWidth + 1);
		int xOffset = - textRect.left + owner->getWidth () - textWidth - 1;
		textRect.offset (xOffset, 0);
		port.drawText (textRect, label, font, textBrush);

		crosshairBounds.right = textRect.left - 2;
		
		param = (static_cast<VectorPad*> (owner))->getYParameter ();
		param->getString (label, param->getMax ());
		textWidth = port.getStringWidth (label, font);
		textRect.setWidth (textWidth + 1);
		xOffset = - textRect.left + int (0.5f * (owner->getWidth () - textWidth));
		int yOffset = - (textRect.top - 2);
		textRect.offset (xOffset, yOffset);
		port.drawText (textRect, label, font, textBrush);
		
		crosshairBounds.top = textRect.bottom + ((textWidth > 10) ? 2 : -3);
		
		param->getString (label, param->getMin ());
		textWidth = port.getStringWidth (label, font);
		textRect.setWidth (textWidth + 1);
		xOffset = - textRect.left + int (0.5f * (owner->getWidth () - textWidth));
		yOffset = - textRect.top + owner->getHeight () - fontSize - 1;
		textRect.offset (xOffset, yOffset);
		port.drawText (textRect, label, font, textBrush);
		
		crosshairBounds.bottom = textRect.top - ((textWidth > 10) ? 2 : -3);
	}

	if(drawCrosshair)
	{
		Pen pen (crosshairColor);
		
		Point x1 (crosshairBounds.left, (dst.getHeight ()-1) / 2);
		Point x2 (crosshairBounds.right, x1.y);
		port.drawLine (x1, x2, pen);
		Point y1 (dst.getWidth () / 2, crosshairBounds.top);
		Point y2 (y1.x, crosshairBounds.bottom);
		port.drawLine (y1, y2, pen);
	}

	//handle
	Rect handleRect;
	if(getHandleRect (handleRect))
	{
		if(handle)
		{
			StringID frame = owner->isMouseDown () ? ThemeNames::kPressed : ThemeNames::kNormal;
			handle->setCurrentFrame (handle->getFrameIndex (frame));

			Rect src (0, 0, handle->getWidth (), handle->getHeight ());
			port.drawImage (handle, src, handleRect);
		}
		else
			port.drawEllipse (handleRect, Pen (handleColor, handleThickness));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int VectorPadRenderer::hitTest (PointRef where, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorPadRenderer::getPartRect (Rect& rect, int partCode)
{
	if(partCode == VectorPad::kPartHandle)
		return getHandleRect (rect);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorPadRenderer::getHandleRect (Rect& rect)
{
	if(owner->isEnabled () == false)
		return false;
	
	float x = (static_cast<VectorPad*> (owner))->getXValue ();
	float y = (static_cast<VectorPad*> (owner))->getYValue ();

	Rect clientRect;
	owner->getClientRect (clientRect);
	
	Point hotspot (Coord (x * clientRect.getWidth ()), Coord (y * clientRect.getHeight ()));

	if(handle)
	{
		rect.left = hotspot.x - int (handle->getWidth () * 0.5f);
		rect.right = rect.left + handle->getWidth ();
		rect.top = hotspot.y - int (handle->getHeight () * 0.5f);
		rect.bottom = rect.top + handle->getHeight ();
	}
	else
	{
		rect.left = rect.right = hotspot.x;
		rect.top = rect.bottom = hotspot.y;
		rect.expand (int (handleSize * 0.5f + 0.5f));
	}

	return true;
}
