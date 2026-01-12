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
// Filename    : ccl/gui/controls/scrollbar.cpp
// Description : Scrollbar
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/scrollbar.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/scrollview.h"
#include "ccl/gui/theme/renderer/scrollbarrenderer.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/app/params.h"

namespace CCL {

using namespace ThemeElements;

//************************************************************************************************
// ScrollBarMouseHandler
//************************************************************************************************

class ScrollBarMouseHandler: public MouseHandler
{
public:
	ScrollBarMouseHandler (ScrollBar* scrollBar, const Point& clickOffset)
	: MouseHandler (scrollBar),
	  clickOffset (clickOffset)
	{}

	// MouseHandler
	void onBegin () override
	{   
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->beginEdit ();
		scrollBar->setMouseState (ScrollBar::kHandlePressed); 

		// experimental:
		if(ScrollView* scrollView = ccl_cast<ScrollView> (scrollBar->getParent ()))
			scrollView->setScrolling (true);

		onMove (0);
	} 
	
	void onRelease (bool) override
	{ 
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->endEdit ();
		scrollBar->setMouseState (ScrollBar::kMouseNone); 

		// experimental:
		if(ScrollView* scrollView = ccl_cast<ScrollView> (scrollBar->getParent ()))
			scrollView->setScrolling (false);
	}
	
	bool onMove (int moveFlags) override
	{
		ScrollBar* scrollBar = (ScrollBar*)view;
		ThemeRenderer* renderer = scrollBar->getRenderer ();
		
		Rect trackingRect;
		renderer->getPartRect (scrollBar, ScrollBar::kPartTrackingArea, trackingRect);
		Rect handleRect;
		renderer->getPartRect (scrollBar, ScrollBar::kPartHandle, handleRect);

		float pos = 0.f;
		if(scrollBar->getStyle ().isVertical ())
			pos = (float)(current.where.y - trackingRect.top - clickOffset.y) / (float)(trackingRect.getHeight () - handleRect.getHeight ());
		else
			pos = (float)(current.where.x - trackingRect.left - clickOffset.x) / (float)(trackingRect.getWidth () - handleRect.getWidth ());

		scrollBar->getParameter ()->setNormalized (pos, true);
		return true;
	}

protected:
	Point clickOffset;
};

//************************************************************************************************
// ScrollBarButtonHandler
//************************************************************************************************

class ScrollBarButtonHandler: public PeriodicMouseHandler
{
public:
	ScrollBarButtonHandler (ScrollBar* scrollBar, int hit)
	: PeriodicMouseHandler (scrollBar),
	  hit (hit)
	{
		setWaitAfterRepeat (50);
	}

	// MouseHandler
	void onBegin () override
	{   
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->beginEdit ();
		scrollBar->setMouseState (ScrollBar::kHandlePressed); 
	} 

	void onRelease (bool) override
	{ 
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->endEdit ();
		scrollBar->setMouseState (ScrollBar::kMouseNone); 
	}

	bool onPeriodic () override
	{
		ScrollBar* scrollBar = (ScrollBar*)view;
		ThemeRenderer* renderer = scrollBar->getRenderer ();
		
		int partCode = renderer->hitTest (scrollBar, current.where, nullptr);

		bool inside = 
			(hit == ScrollBar::kButtonDownPressed && partCode == ScrollBar::kPartButtonDown) ||
			(hit == ScrollBar::kButtonUpPressed && partCode == ScrollBar::kPartButtonUp);
		
		scrollBar->setMouseState (inside ? hit : ScrollBar::kMouseNone);
		if(inside)
		{
			if(hit == ScrollBar::kButtonDownPressed)
				scrollBar->getParameter ()->decrement ();
			else
				scrollBar->getParameter ()->increment ();
		}

		return true;
	}

protected:
	int hit;
};

//************************************************************************************************
// ScrollBarPageHandler
//************************************************************************************************

class ScrollBarPageHandler: public PeriodicMouseHandler
{
public:
	ScrollBarPageHandler (ScrollBar* scrollBar, int hit)
	: PeriodicMouseHandler (scrollBar),
	  hit (hit)
	{}

	// MouseHandler
	void onBegin () override
	{   
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->beginEdit ();
	} 

	void onRelease (bool) override
	{ 
		ScrollBar* scrollBar = (ScrollBar*)view;
		scrollBar->getParameter ()->endEdit ();
		scrollBar->setMouseState (ScrollBar::kMouseNone); 
	}

	bool onPeriodic () override
	{
		ScrollBar* scrollBar = (ScrollBar*)view;
		ThemeRenderer* renderer = scrollBar->getRenderer ();
		
		Point clickOffset;
		int partCode = renderer->hitTest (scrollBar, current.where, &clickOffset);

		if(partCode == ScrollBar::kPartHandle)
		{
			// "picked up" the handle: switch mousehandler
			if(Window* window = scrollBar->getWindow ())
				window->setMouseHandler (NEW ScrollBarMouseHandler (scrollBar, clickOffset));
			return true;
		}

		bool inside = 
			(hit == ScrollBar::kPageDownPressed && partCode == ScrollBar::kPartPageDown) ||
			(hit == ScrollBar::kPageUpPressed && partCode == ScrollBar::kPartPageUp);
		
		scrollBar->setMouseState (inside ? hit : ScrollBar::kMouseNone);
		if(inside)
		{
			IParameter* param = scrollBar->getParameter ();
			UnknownPtr<IScrollParameter> scrollParam = param;
			double pageSize = ((double)param->getMax () - (double)param->getMin ()) * scrollParam->getPageSize ();
			if(hit == ScrollBar::kPageDownPressed)
				param->setValue ((double)param->getValue () - pageSize, true);
			else
				param->setValue ((double)param->getValue () + pageSize, true);
		}

		return true;
	}

protected:
	int hit;
};

//************************************************************************************************
// PageControlMouseHandler
//************************************************************************************************

class PageControlMouseHandler: public MouseHandler
{
public:
	PageControlMouseHandler (PageControl* pageControl)
	: MouseHandler (pageControl)
	{
	}

	// MouseHandler
	void onBegin () override
	{
		PageControl* pageControl = static_cast<PageControl*> (view);
		pageControl->getParameter ()->beginEdit ();
		pageControl->push ();
	}

	void onRelease (bool) override
	{
		PageControl* pageControl = static_cast<PageControl*> (view);
		pageControl->getParameter ()->endEdit ();
	}
};

//************************************************************************************************
// ScrollBar
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (ScrollBar::customStyles)
	{"jump",	Styles::kScrollBarBehaviorJump},
	{"passive",	Styles::kScrollBarBehaviorPassive},
END_STYLEDEF

BEGIN_STYLEDEF (ScrollBar::partNames)
	{"handle",		ScrollBar::kPartHandle},
	{"buttondown",	ScrollBar::kPartButtonDown},
	{"buttonup",	ScrollBar::kPartButtonUp},
	{"pageup",		ScrollBar::kPartPageUp},
	{"pagedown",	ScrollBar::kPartPageDown},
	{"trackarea",	ScrollBar::kPartTrackingArea},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ScrollBar, Control)
DEFINE_CLASS_UID (ScrollBar, 0x21679c33, 0xd0ea, 0x4368, 0xa0, 0x63, 0x74, 0x9b, 0x9a, 0xf9, 0x50, 0xdb)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollBar::ScrollBar (const Rect& size, IParameter* _param, StyleRef style)
: Control (size, nullptr, style)
{
	if(_param)
		setParameter (_param);
	else
	{
		setParameter (NEW ScrollParam);
		param->release ();
	}

	ignoresFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScrollParameter* ScrollBar::getScrollParam () const
{
	return UnknownPtr<IScrollParameter> (param); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollBar::attached (View* parent)
{
	SuperClass::attached (parent); // might create layer
	
	getRenderer (); // might apply triggers
	
	signal (Message (kOnAttached));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ScrollBar::createMouseHandler (const MouseEvent& event)
{
	getRenderer ();
	if(!renderer || getStyle ().isCustomStyle (Styles::kScrollBarBehaviorPassive))
		return nullptr;

	Point clickOffset;
	int partCode = renderer->hitTest (this, event.where, &clickOffset);
	
	switch(partCode)
	{
	case kPartHandle:
		return NEW ScrollBarMouseHandler (this, clickOffset);
	
	case kPartButtonDown:
		return NEW ScrollBarButtonHandler (this, kButtonDownPressed);
		
	case kPartButtonUp:
		return NEW ScrollBarButtonHandler (this, kButtonUpPressed);

	case kPartPageUp:
	case kPartPageDown:
		{
			if(style.isCustomStyle (Styles::kScrollBarBehaviorJump))
			{
				// directly jump to mouse position and move from there (as if center of handle was clicked)
				Rect handle;
				renderer->getPartRect (this, kPartHandle, handle);
				return NEW ScrollBarMouseHandler (this, handle.getSize () * 0.5);
			}
			else
				return NEW ScrollBarPageHandler (this, partCode == kPartPageUp ? kPageUpPressed : kPageDownPressed);
	
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBar::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
	{
		bool inverse = true; // generally invert direction on a scrollbar to match behavior in the client view
		if(event.isHorizontal ())
		{
			MouseWheelEvent& me = const_cast<MouseWheelEvent&> (event);
			if(me.eventType == MouseWheelEvent::kWheelRight)
				me.eventType = MouseWheelEvent::kWheelUp;
			else
				me.eventType = MouseWheelEvent::kWheelDown;

			// don't invert when scrolling horizontally on a horizontal scrollbar
			inverse = !getStyle ().isCommonStyle (Styles::kHorizontal);
		}
		return tryWheelParam (event, inverse);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBar::onMouseEnter (const MouseEvent& event)
{
	// LATER TODO: need to track mouse position and hilite handle & buttons separately!!
	// return false;
	setMouseOverPosition (event.where);
	setMouseState (kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBar::onMouseMove (const MouseEvent& event)
{
	setMouseOverPosition (event.where);
	invalidate ();
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBar::onMouseLeave (const MouseEvent& event)
{
	setMouseState (kMouseNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollBar::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ScrollBar::getRenderer ()
{
	if(renderer == nullptr)
	{
		renderer = getTheme ().createRenderer (ThemePainter::kScrollBarRenderer, visualStyle);
		
		if(!visualStyle)
			if(VisualStyle* vs = renderer->getVisualStyle ())
				if(vs->getTrigger ())
					vs->getTrigger ()->applyTrigger (this);
	}
	return renderer;
}

//************************************************************************************************
// ScrollButton
//************************************************************************************************

DEFINE_CLASS (ScrollButton, ScrollBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollButton::ScrollButton (const Rect& size, IParameter* param, int partCode)
: ScrollBar (size, param),
  partCode (partCode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ScrollButton::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kScrollButtonRenderer, visualStyle);
	return renderer;
}


//************************************************************************************************
// PageControl
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PageControl, ScrollBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

PageControl::PageControl (const Rect& size, IParameter* param, StyleRef style)
: ScrollBar (size, param, style)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* PageControl::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kPageControlRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* PageControl::createMouseHandler (const MouseEvent& event)
{
	getRenderer ();
	if(!renderer || getStyle ().isCustomStyle (Styles::kScrollBarBehaviorPassive))
		return nullptr;

	Point clickOffset;
	int partCode = renderer->hitTest (this, event.where, &clickOffset);
	
	if(partCode == kPartHandle)
		return NEW PageControlMouseHandler (this);
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PageControl::push ()
{
	if(ScrollView* scrollView = ccl_cast<ScrollView> (getParent ()))
	{
		int numPages = getNumPages ();
		int currentPage = getCurrentPage ();
		
		if(currentPage == (numPages - 1)) // back to first page
		{
			float duration = numPages * 0.15;
			float velocity = 50.;
			
			scrollView->scrollTo (Point (0,0), duration, velocity);
		}
		else // advance to next page
		{
			ScrollManipulator manipulator (scrollView);
			Point delta (getStyle ().isVertical () ? 0 : 1, getStyle ().isVertical () ? 1 : 0);
			manipulator.push (delta);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PageControl::getNumPages () const
{
	if(IScrollParameter* scrollParam = getScrollParam ())
		return (int)ceil (1.f / scrollParam->getPageSize ());
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PageControl::getCurrentPage () const
{
	if(IScrollParameter* scrollParam = getScrollParam ())
		return ccl_min <int> (getNumPages () - 1, int (getParameter ()->getNormalized () / ccl_min (1.f, scrollParam->getPageSize ())));
	
	return 0;
}

} // namespace CCL
