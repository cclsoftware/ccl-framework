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
// Filename    : ccl/gui/theme/renderer/backgroundrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/backgroundrenderer.h"

#include "ccl/gui/windows/dialog.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// BackgroundRenderer
/** Draws a background image. Two separate images can be specified for normal windows and dialogs.
if no image is available, "backcolor" is used. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Background, VisualStyle, "WindowBackgroundStyle")
	ADD_VISUALSTYLE_IMAGE  ("DialogBack")	///< used when window is a dialog
	ADD_VISUALSTYLE_IMAGE  ("WindowBack")	///< used when window is not a dialog
END_VISUALSTYLE_CLASS (Background)

//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundRenderer::BackgroundRenderer (VisualStyle* visualStyle)
:	ThemeRenderer (visualStyle)
{
	dialogImage = visualStyle->getImage ("DialogBack");
	windowImage = visualStyle->getImage ("WindowBack");
	backBrush = SolidBrush (visualStyle->getColor ("backcolor", Colors::kWhite));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	StyleRef style = view->getStyle ();
	if(style.isOpaque ())
	{
		GraphicsPort port (view);
		port.addClip (updateRgn.bounds);
		Rect rect;
		view->getClientRect (rect);
		
		Dialog* dialog = ccl_cast<Dialog> (view);
		if(dialogImage && dialog)
			port.drawImage (dialogImage, Rect (0, 0, dialogImage->getWidth (), dialogImage->getHeight ()), rect);
		else
		{
			if(windowImage)
				port.drawImage (windowImage, Rect (0, 0, windowImage->getWidth (), windowImage->getHeight ()), rect);
			else
				port.fillRect (updateRgn.bounds, backBrush);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BackgroundRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
