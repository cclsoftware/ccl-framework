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
// Filename    : ccl/gui/theme/renderer/buttonrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/scrollbarrenderer.h"

#include "ccl/gui/controls/scrollbar.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;


//************************************************************************************************
// ScrollBarRenderer
//************************************************************************************************

struct CCL::ScrollBarDrawState
{
	Rect handleRect;
	Rect buttonDownRect;
	Rect buttonUpRect;
	bool drawButtons;
	bool drawHandle;
	int scrollRange;
	int scrollStart;

	ScrollBarDrawState ()
	: drawButtons (false),
	  drawHandle (true),
	  scrollRange (0),
	  scrollStart (0)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/** A Scrollbar indicates the scrolling position of a scroll view.
A scroll bar has a background, up/down buttons (vertical) and a handle (thumb). */

BEGIN_VISUALSTYLE_CLASS (ScrollBar, VisualStyle, "ScrollBarStyle")
	ADD_VISUALSTYLE_IMAGE  ("vButtonUp")		///< "up" button (vertical bar)
	ADD_VISUALSTYLE_IMAGE  ("vButtonDown")		///< "down" button (vertical bar)
	ADD_VISUALSTYLE_IMAGE  ("vThumb")			///< handle (vertical bar)
	ADD_VISUALSTYLE_IMAGE  ("vBack")			///< background (vertical bar)
	ADD_VISUALSTYLE_IMAGE  ("vSmallThumb")		///< small handle (vertical bar with option "small")
	ADD_VISUALSTYLE_IMAGE  ("vSmallBack")		///< background (vertical bar with option "small")
	ADD_VISUALSTYLE_IMAGE  ("hButtonUp")		///< "left" button (horizontal bar)
	ADD_VISUALSTYLE_IMAGE  ("hButtonDown")		///< "right" button (horizontal bar)
	ADD_VISUALSTYLE_IMAGE  ("hThumb")			///< handle (horizontal bar)
	ADD_VISUALSTYLE_IMAGE  ("hBack")			///< background (horizontal bar)
	ADD_VISUALSTYLE_IMAGE  ("hSmallThumb")		///< small handle (horizontal bar with option "small")
	ADD_VISUALSTYLE_IMAGE  ("hSmallBack")		///< background (horizontal bar with option "small")
	ADD_VISUALSTYLE_METRIC ("clipBackground")	///< background is only drawn outside of handle area
END_VISUALSTYLE_CLASS (ScrollBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollBarRenderer::ScrollBarRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  imagesLoaded (false),
  clipBackground (false)
{
	inset = (float)visualStyle->getMetric ("inset", 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollBarRenderer::loadImages ()
{
	vImages.buttonUp    = visualStyle->getImage ("vButtonUp");
	vImages.buttonDown  = visualStyle->getImage ("vButtonDown");
	vImages.thumb       = visualStyle->getImage ("vThumb");
	vImages.back        = visualStyle->getImage ("vBack");

	vSmallImages.thumb  = visualStyle->getImage ("vSmallThumb");
	vSmallImages.back   = visualStyle->getImage ("vSmallBack");

	hImages.buttonUp    = visualStyle->getImage ("hButtonUp");
	hImages.buttonDown  = visualStyle->getImage ("hButtonDown");
	hImages.thumb       = visualStyle->getImage ("hThumb");
	hImages.back        = visualStyle->getImage ("hBack");
	
	hSmallImages.thumb  = visualStyle->getImage ("hSmallThumb");
	hSmallImages.back   = visualStyle->getImage ("hSmallBack");

	clipBackground = visualStyle->getMetric<bool> ("clipBackground", false);

	imagesLoaded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollBarRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	if(!imagesLoaded)
		loadImages ();

	ScrollBar* scrollBar = (ScrollBar*)view;

	StyleRef style = scrollBar->getStyle ();
	ScrollBarImages& images = getImages (style);
	ScrollBarDrawState state;
	bool canScroll = getDrawState (scrollBar, state, images);
	bool disabled = !scrollBar->isEnabled () || !canScroll;

	GraphicsPort port (scrollBar);

	// *** Draw Background ***
	if(style.isOpaque ())
	{
		Rect cr;
		scrollBar->getClientRect (cr);

		IImage* image = images.back;
		if(image)
		{
			MutableCString frame;
			
			if(disabled)
				frame = ThemeNames::kDisabled;
			else if(scrollBar->getMouseState () == ScrollBar::kHandlePressed)
				frame = ThemeNames::kPressed;
			else if(scrollBar->getMouseState () == ThemeElements::kMouseOver)
				frame = ThemeNames::kMouseOver;
			else
				frame = ThemeNames::kNormal;
		
			IImage::Selector (image, frame);
			Rect src (0, 0, image->getWidth (), image->getHeight ());
			if(clipBackground)
			{
				// only draw background outside of handle rect; todo: for non-unicolor images, adjust source rect (intercept theorem)
				Rect r (cr);
				r.right = state.handleRect.left;
				port.drawImage (image, src, r);

				r.right = cr.right;
				r.left = state.handleRect.right;
				port.drawImage (image, src, r);
			}
			else
				port.drawImage (image, src, cr);
		}
		else
			port.drawRect (cr, Pen (Colors::kBlack));
	}

	// *** Draw Handle ***
	if(state.drawHandle && !disabled)
	{
		IImage* image = images.thumb;
		if(image)
		{
			MutableCString frame;
			
			if(scrollBar->getMouseState () == ScrollBar::kHandlePressed)
				frame = ThemeNames::kPressed;
			else
				frame = ThemeNames::kNormal;
			
			if(scrollBar->getMouseState () == ThemeElements::kMouseOver)
			{
				if(state.handleRect.pointInside (scrollBar->getMouseOverPosition ()))
					frame = ThemeNames::kMouseOver;
			}
		
			IImage::Selector (image, frame);
			port.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), state.handleRect);
		}
		else
			port.drawRect (state.handleRect, Pen (Colors::kBlack));
	}

	// *** Draw Buttons ***
	if(state.drawButtons)
	{
		IImage* image = images.buttonDown;
		if(image)
		{
			CString buttonState;
			if(disabled)
				buttonState = ThemeNames::kDisabled;
			else if (scrollBar->getMouseState () == ScrollBar::kButtonDownPressed)
				buttonState = ThemeNames::kPressed;
			else
				buttonState = ThemeNames::kNormal;

			if(scrollBar->getMouseState () == ThemeElements::kMouseOver)
			{
				if(state.buttonDownRect.pointInside (scrollBar->getMouseOverPosition ()))
					buttonState = ThemeNames::kMouseOver;
			}
			
			IImage::Selector (image, buttonState);
			port.drawImage (image, state.buttonDownRect.getLeftTop ());
		}
		else
			port.drawRect (state.buttonDownRect, Pen (Colors::kBlack));

		image = images.buttonUp;
		if(image)
		{
			CString buttonState;
			if(disabled)
				buttonState = ThemeNames::kDisabled;
			else if (scrollBar->getMouseState () == ScrollBar::kButtonUpPressed)
				buttonState = ThemeNames::kPressed;
			else
				buttonState = ThemeNames::kNormal;

			if(scrollBar->getMouseState () == ThemeElements::kMouseOver)
			{
				if(state.buttonUpRect.pointInside (scrollBar->getMouseOverPosition ()))
					buttonState = ThemeNames::kMouseOver;
			}
			
			IImage::Selector (image, buttonState);
			port.drawImage (image, state.buttonUpRect.getLeftTop ());
		}
		else
			port.drawRect (state.buttonUpRect, Pen (Colors::kBlack));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollBarRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	ScrollBar* scrollBar = (ScrollBar*)view;
	
	StyleRef style = scrollBar->getStyle ();
	ScrollBarImages& images = getImages (style);
	ScrollBarDrawState state;
	if(!getDrawState (scrollBar, state, images))
		return 0;

	if(state.drawHandle && state.handleRect.pointInside (loc))
	{
		if (clickOffset)
		{
			clickOffset->x = loc.x - state.handleRect.left;
			clickOffset->y = loc.y - state.handleRect.top;
		}
		return ScrollBar::kPartHandle;
	}	
	else if(state.drawButtons && state.buttonDownRect.pointInside (loc))
		return ScrollBar::kPartButtonDown;
		
	else if(state.drawButtons && state.buttonUpRect.pointInside (loc))
		return ScrollBar::kPartButtonUp;
		
	else
	{
		CCL::Rect r;
		getPartRect (scrollBar, ScrollBar::kPartPageUp, r);
		if(r.pointInside (loc))
			return ScrollBar::kPartPageUp;

		getPartRect (scrollBar, ScrollBar::kPartPageDown, r);
		if(r.pointInside (loc))
			return ScrollBar::kPartPageDown;
	}
	return ScrollBar::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBarRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	ScrollBar* scrollBar = (ScrollBar*)view;
	
	StyleRef style = scrollBar->getStyle ();
	ScrollBarImages& images = getImages (style);
	ScrollBarDrawState drawState;
	if(!getDrawState (scrollBar, drawState, images))
		return false;
	
	switch (partCode)
	{
		case ScrollBar::kPartHandle:      rect = drawState.handleRect; return true;
		case ScrollBar::kPartButtonDown:  rect = drawState.buttonDownRect; return true;
		case ScrollBar::kPartButtonUp:    rect = drawState.buttonUpRect; return true;
		case ScrollBar::kPartPageDown:
		{
			rect = drawState.handleRect;
			if(style.isVertical ())
			{
				rect.top = drawState.buttonDownRect.bottom;
				rect.bottom = drawState.handleRect.top;
			}
			else
			{
				rect.left = drawState.buttonDownRect.right;
				rect.right = drawState.handleRect.left;
			}
			return true;
		}
		case ScrollBar::kPartPageUp:
		{
			rect = drawState.handleRect;
			if(style.isVertical ())
			{
				rect.top = drawState.handleRect.bottom;
				rect.bottom = drawState.buttonUpRect.top;
			}
			else
			{
				rect.left = drawState.handleRect.right;
				rect.right = drawState.buttonUpRect.left;
			}
			return true;
		}
		case ScrollBar::kPartTrackingArea:
		{
			rect = drawState.handleRect;
			if(style.isVertical ())
			{
				rect.top = drawState.buttonDownRect.bottom;
				rect.bottom = drawState.buttonUpRect.top;
			}
			else
			{
				rect.left = drawState.buttonDownRect.right;
				rect.right = drawState.buttonUpRect.left;
			}
			return true;
		}
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBarRenderer::getDrawState (ScrollBar* scrollBar, ScrollBarDrawState& state, ScrollBarImages& images) const
{
	IParameter* param = scrollBar->getParameter ();
	float nPos = param ? param->getNormalized () : 0.f;
	IScrollParameter* sParam = scrollBar->getScrollParam ();
	float nPage = sParam ? sParam->getPageSize () : 0.f;

	StyleRef style = scrollBar->getStyle();
	int sbSize = scrollBar->getTheme ().getThemeMetric (ThemeElements::kScrollBarSize);
	if(style.isSmall ())
		sbSize /= 2;
	int maxPixels = style.isVertical () ? scrollBar->getHeight () : scrollBar->getWidth ();

	state.drawButtons = !style.isSmall ();
	int btnSize = 0;
	if(state.drawButtons)
		btnSize = images.buttonUp ? (style.isVertical () ? images.buttonUp->getHeight () : images.buttonUp->getWidth ()) : sbSize;

	if(maxPixels < 2 * sbSize && state.drawButtons)
	{
		state.drawHandle = false;
		btnSize = maxPixels / 2;
	}

	if(style.isVertical ())
	{
		state.buttonDownRect (0, 0, scrollBar->getWidth (), btnSize);
		state.buttonUpRect (0, scrollBar->getHeight () - btnSize, scrollBar->getWidth (), scrollBar->getHeight ());
	}
	else
	{
		state.buttonDownRect (0, 0, btnSize, scrollBar->getHeight ());
		state.buttonUpRect (scrollBar->getWidth () - btnSize, 0, scrollBar->getWidth (), scrollBar->getHeight ());
	}

	if(state.drawButtons) // space for up/down buttons
		maxPixels -= 2 * btnSize;
	maxPixels -= (int)(inset * 2.);

	int pagePixels = ccl_to_int (nPage * maxPixels);
	if(pagePixels < sbSize/2)
		pagePixels = sbSize/2;
	if(!state.drawHandle)
		pagePixels = 0;

	state.scrollRange = maxPixels - pagePixels;
	if(state.scrollRange < 0)
		state.scrollRange = 0;

	int posPixels = ccl_to_int (nPos * state.scrollRange);
	if(state.drawButtons)
		posPixels += btnSize;
	posPixels += (int)inset;

	if(style.isVertical ())
	{
		state.handleRect (
			images.thumb ? (scrollBar->getWidth () - images.thumb->getWidth ()) / 2 : 0, 
			posPixels, 
			images.thumb ? images.thumb->getWidth () : scrollBar->getWidth (), 
			posPixels + pagePixels);
	}
	else
		state.handleRect (posPixels, 
			images.thumb ? (scrollBar->getHeight () - images.thumb->getHeight ()) / 2 : 0, 
			posPixels + pagePixels, 
			images.thumb ? images.thumb->getHeight () : scrollBar->getHeight ());

	state.scrollStart = state.drawButtons ? btnSize : 0;
	return nPage > 0.f && nPage < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollBarImages& ScrollBarRenderer::getImages (StyleRef style)
{
	return style.isVertical () ? (style.isSmall () ? vSmallImages : vImages) : (style.isSmall () ? hSmallImages : hImages);
//	return style.isVertical () ? vImages : hImages;
}

//************************************************************************************************
// ScrollButtonRenderer
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (ScrollButton, VisualStyle, "ScrollButtonStyle")
	ADD_VISUALSTYLE_IMAGE  ("hButtonDown")	///< "left" button background (horizontal button)
	ADD_VISUALSTYLE_IMAGE  ("hButtonUp")	///< "right" button background (horizontal button)
	ADD_VISUALSTYLE_IMAGE  ("hIconDown")	///< "left" icon (horizontal button)
	ADD_VISUALSTYLE_IMAGE  ("hIconUp")		///< "right" icon (horizontal button)
	ADD_VISUALSTYLE_IMAGE  ("vButtonDown")	///< "down" button background (vertical button)
	ADD_VISUALSTYLE_IMAGE  ("vButtonUp")	///< "up" button background (vertical button)
	ADD_VISUALSTYLE_IMAGE  ("vIconDown")	///< "down" icon (vertical button)
	ADD_VISUALSTYLE_IMAGE  ("vIconUp")		///< "up" icon (vertical button)
END_VISUALSTYLE_CLASS (ScrollButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollButtonRenderer::drawTriangleIcon (IGraphics& port, RectRef clientRect, int orientation)
{
	enum { kIconWidth = 8 };
	Point points[3];
	if(orientation == Alignment::kLeft || orientation == Alignment::kRight)
	{
		Rect t (0, 0, kIconWidth / 2, kIconWidth);
		t.center (clientRect);
		Coord cy = (t.top + t.bottom) / 2;

		if(orientation == Alignment::kLeft)
			points[0] (t.left, cy), points[1] (t.right, t.top), points[2](t.right, t.bottom);
		else
			points[0] (t.left, t.top), points[1] (t.right, cy), points[2](t.left, t.bottom);
	}
	else
	{
		Rect t (0, 0, kIconWidth, kIconWidth / 2);
		t.center (clientRect);
		Coord cx = (t.left + t.right) / 2;

		if(orientation == Alignment::kTop)
			points[0] (t.left, t.bottom), points[1] (cx, t.top), points[2](t.right, t.bottom);
		else
			points[0] (t.left, t.top), points[1] (cx, t.bottom), points[2](t.right, t.top);
	}

	port.fillTriangle (points, SolidBrush (Colors::kWhite));
	port.drawTriangle (points, Pen (Colors::kBlack));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollButtonRenderer::ScrollButtonRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  imagesLoaded (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollButtonRenderer::loadImages (ScrollButton* view)
{
	if(visualStyle)
	{
		if(view->getStyle ().isHorizontal ())
		{
			if(view->getPartCode () == ScrollButton::kPartButtonDown)
			{
				buttonImage = visualStyle->getImage ("hButtonDown");
				icon = visualStyle->getImage ("hIconDown");
			}
			else
			{
				buttonImage = visualStyle->getImage ("hButtonUp");
				icon = visualStyle->getImage ("hIconUp");
			}
		}
		else
		{
			if(view->getPartCode () == ScrollButton::kPartButtonDown)
			{
				buttonImage = visualStyle->getImage ("vButtonDown");
				icon = visualStyle->getImage ("vIconDown");
			}
			else
			{
				buttonImage = visualStyle->getImage ("vButtonUp");
				icon = visualStyle->getImage ("vIconUp");
			}
		}
	}

	if(!buttonImage)
	{
		// borrow background image from standard button renderer
		AutoPtr<ThemeRenderer> buttonRenderer (view->getTheme ().createRenderer (ThemePainter::kButtonRenderer, nullptr));
		if(buttonRenderer)
			if(VisualStyle* style = buttonRenderer->getVisualStyle ())
				buttonImage = style->getBackgroundImage ();
	}

	imagesLoaded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollButtonRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	ScrollButton* scrollButton = (ScrollButton*)view;
	if(!imagesLoaded)
		loadImages (scrollButton);

	// StyleRef style = scrollButton->getStyle ();
	bool disabled = !scrollButton->isEnabled ();

	Rect r;
	scrollButton->getClientRect (r);
	GraphicsPort port (scrollButton);

	// determine buton state
	int pressedState = (scrollButton->getPartCode () == ScrollBar::kPartButtonDown) ? ScrollBar::kButtonDownPressed : ScrollBar::kButtonUpPressed;
	CString buttonState;
	if(disabled)
		buttonState = ThemeNames::kDisabled;
	else if (scrollButton->getMouseState () == pressedState)
		buttonState = ThemeNames::kPressed;
	else
		buttonState = ThemeNames::kNormal;

	// *** Draw Button ***
	if(buttonImage)
	{
		IImage::Selector (buttonImage, buttonState);
		port.drawImage (buttonImage, Rect (0, 0, buttonImage->getWidth (), buttonImage->getHeight ()), r);
	}
	else
		port.drawRect (r, Pen (Colors::kBlack));

	// *** Draw triangle icon ***
	if(icon)
	{
		Rect iconSize (0, 0, icon->getWidth (), icon->getHeight ());
		Rect iconRect (iconSize);
		iconRect.center (r);

		IImage::Selector (icon, buttonState);
		port.drawImage (icon, iconSize, iconRect);
	}
	else
		drawTriangle (port, r, scrollButton);

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollButtonRenderer::drawTriangle (IGraphics& port, RectRef clientRect, ScrollButton* scrollButton)
{
	int orientation = 0;
	if(scrollButton->getStyle ().isHorizontal ())
	{
		if(scrollButton->getPartCode () == ScrollButton::kPartButtonDown)
			orientation = Alignment::kLeft;
		else
			orientation = Alignment::kRight;
	}
	else
	{
		if(scrollButton->getPartCode () == ScrollButton::kPartButtonDown)
			orientation = Alignment::kTop;
		else
			orientation = Alignment::kBottom;
	}

	drawTriangleIcon (port, clientRect, orientation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollButtonRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	ScrollButton* scrollButton = (ScrollButton*)view;
	return scrollButton->getPartCode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollButtonRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	return false;
}

//************************************************************************************************
// PageControlRenderer
/** A PageControl draws a row of dots, with one dot per scroll page. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (PageControl, VisualStyle, "PageControlStyle")
	ADD_VISUALSTYLE_IMAGE  ("background")	///< image drawn undeneath the dots
	ADD_VISUALSTYLE_IMAGE  ("dot")			///< dot image with frames "normal" and "normalOn" for the current page
	ADD_VISUALSTYLE_METRIC ("spacing")		///< spacing (in pixels) between the dots
END_VISUALSTYLE_CLASS (PageControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

PageControlRenderer::PageControlRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  spacing (0),
  imagesLoaded (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PageControlRenderer::loadImages ()
{
	background	= visualStyle->getImage ("background");
	dotImage	= visualStyle->getImage ("dot");
	spacing		= visualStyle->getMetric ("spacing", spacing);
	imagesLoaded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PageControlRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(!imagesLoaded)
		loadImages ();

	Rect client;
	view->getClientRect (client);
	GraphicsPort port (view);

	// *** Draw background ***
	if(background)
		port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), client);

	PageControl* control = (PageControl*)view;
	
	int numPages = control->getNumPages ();
	int currentPage = control->getCurrentPage ();
	
	if(numPages <= 0)
		return;

	// *** Draw dots ***
	if(dotImage)
	{
		float zoomFactor = control->getZoomFactor ();
		Rect dotRect;
		getPartRect (view, PageControl::kPartHandle, dotRect);
		Rect dotSize (0, 0, dotImage->getWidth (), dotImage->getHeight ());
		dotRect.setSize (dotSize.getSize () * zoomFactor);
		
		for(int i = 0; i < numPages; i++)
		{
			CString state = (i == currentPage) ? ThemeNames::kNormalOn : ThemeNames::kNormal;
			IImage::Selector (dotImage, state);
			port.drawImage (dotImage, dotSize, dotRect);
			dotRect.offset ((dotSize.right + spacing) * zoomFactor, 0);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int PageControlRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	Rect r;
	getPartRect (view, PageControl::kPartHandle, r);
	if(r.pointInside (loc))
		return PageControl::kPartHandle;
	
	return PageControl::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PageControlRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	if(!imagesLoaded)
		loadImages ();
	
	const PageControl* control = static_cast<const PageControl*> (view);
	int numPages = control->getNumPages ();
	float zoomFactor = control->getZoomFactor ();
	
	Rect dotSize (0, 0, dotImage ? dotImage->getWidth () : 8, dotImage ? dotImage->getHeight () : 8);
	Rect dotsArea (0, 0, (dotSize.right * numPages + spacing * (numPages - 1)) * zoomFactor, dotSize.bottom * zoomFactor);
	
	Rect client;
	view->getClientRect (client);
	dotsArea.center (client);
	
	rect = dotsArea;
	return (partCode == PageControl::kPartHandle) ? true : false;
}
