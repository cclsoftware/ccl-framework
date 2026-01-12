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
// Filename    : ccl/gui/layout/alignview.cpp
// Description : View with switchable alignment for childs
//
//************************************************************************************************

#include "ccl/gui/layout/alignview.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/views/mousehandler.h"

#include "ccl/app/params.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/commanddispatch.h"

using namespace CCL;

//************************************************************************************************
// AlignView::ChildResizeHandler
//************************************************************************************************

class AlignView::ChildResizeHandler: public MouseHandler
{
public:
	ChildResizeHandler (AlignView* alignView, bool isStartDivider, bool isHorizontal, bool isCenter)
	: MouseHandler (alignView),
	childSize (alignView->getFirst ()->getSize ()),
	isStartDivider (isStartDivider),
	isHorizontal (isHorizontal),
	isCenter (isCenter)
	{
	}
	
	bool onMove (int moveFlags) override
	{
		if(previousWhere.isNull ())
			previousWhere = current.where;
		
		Point p (current.where - previousWhere);
		
		if(isHorizontal)
		{
			int offset = isStartDivider ? -p.x : p.x;
			offset *= isCenter ? 2 : 1;
			childSize.right += offset;
		}
		else
		{
			int offset = isStartDivider ? -p.y : p.y;
			offset *= isCenter ? 2 : 1;
			childSize.bottom += offset;
		}
		
		((AlignView*)view)->resizeChild (isHorizontal ? childSize.getWidth () : childSize.getHeight ());
		
		previousWhere = current.where;
		
		return true;
	}
	
protected:
	Point previousWhere;
	Rect childSize;
	bool isHorizontal;
	bool isCenter;
	bool isStartDivider;
};

//************************************************************************************************
// AlignView
//************************************************************************************************

BEGIN_STYLEDEF (AlignView::customStyles)
	{"childsizable",	   AlignView::kChildSizable},
	{"passcontextmenu",	   AlignView::kPassContextMenu},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (AlignView, Control)
DEFINE_CLASS_UID (AlignView, 0x1ED60128, 0xBF6E, 0x4F16, 0x99, 0xEE, 0xD3, 0xFC, 0x52, 0x2E, 0x4F, 0xAC)

//////////////////////////////////////////////////////////////////////////////////////////////////

AlignView::AlignView (const Rect& size, IParameter* param, StyleRef style)
: Control (size, param ? param : AutoPtr<IntParam> (NEW IntParam (0, 2)), style),
  alignment (Alignment::kHMask|Alignment::kVMask), // using the mask values for ignoring the corresponding direction
  storedChildSize (0),
  dividerSize (0),
  dividerOffset (0),
  dividerOutreach (0)
{
	setWheelEnabled (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* AlignView::getViewState (tbool create)
{
	if(!persistenceID.isEmpty ())
	{
#if 1
		// store in global WindowState settings
		Settings& settings = Window::getWindowSettings ();
		String settingsID (CCLSTR ("Alignment"));
		settingsID.append (CCLSTR ("/"));
		settingsID.append (String (persistenceID));
		return &settings.getAttributes (settingsID);
		
#else // todo: option
		// store in layoutstate
		if(ILayoutStateProvider* provider = GetViewInterfaceUpwards<ILayoutStateProvider> (this))
			return provider->getLayoutState (persistenceID, create);
#endif
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::restoreState ()
{
	if(IAttributeList* attribs = getViewState (false))
	{
		AttributeAccessor a (*attribs);
		int value = 0;
		if(a.getInt (value, "align"))
			getParameter ()->setValue (value, true);
		if(a.getInt (value, "childSize"))
			storedChildSize = value;
	}
	paramChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::attached (View* parent)
{
	updateStyle ();
	
	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::updateStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();
	
	dividerStartImage = vs.getImage ("divider.start");
	dividerEndImage = vs.getImage ("divider.end");
	dividerSize = vs.getMetric<int> ("divider.size", getTheme ().getThemeMetric (ThemeElements::kDividerSize));
	dividerOffset = vs.getMetric<int> ("divider.offset", 0);
	dividerOutreach = vs.getMetric<int> ("divider.outreach", getTheme ().getThemeMetric (ThemeElements::kDividerOutreach));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::draw (const UpdateRgn& updateRgn)
{
	View::draw (updateRgn);
	
	GraphicsPort port (this);
	if(getStyle ().isCustomStyle (kChildSizable))
	{
		Rect dstRect;
		bool isStartDivider;
		if(getDividerRect (dstRect, isStartDivider))
		{
			if(IImage* dividerImage = isStartDivider ? dividerStartImage : dividerEndImage)
			{
				IImage::Selector (dividerImage, getMouseState () == IView::kMouseOver ? ThemeNames::kMouseOver : ThemeNames::kNormal);
				Rect srcRect (Point (dividerImage->getWidth (), dividerImage->getHeight ()));
				port.drawImage (dividerImage, srcRect, dstRect);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::getDividerRect (Rect& rect, bool& isStartDivider, int outreachPoints)
{
	View* first = getFirst ();
	if(!first)
		return false;
	
	rect = first->getSize ();
	
	if(getStyle ().isVertical ())
	{
		switch(alignment.getAlignV ())
		{
			case Alignment::kBottom :
			{
				rect.bottom = rect.top + outreachPoints + dividerOffset;
				rect.top -= (dividerSize + outreachPoints - dividerOffset);
				isStartDivider = true;
				break;
			}
			default:
			{
				rect.top = rect.bottom - outreachPoints - dividerOffset;
				rect.bottom += (dividerSize + outreachPoints - dividerOffset);
				isStartDivider = false;
				break;
			}
		}
	}
	else
	{
		switch(alignment.getAlignH ())
		{
			case Alignment::kRight :
			{
				rect.right = rect.left + outreachPoints + dividerOffset;
				rect.left -= (dividerSize + outreachPoints - dividerOffset);
				isStartDivider = true;
				break;
			}
			default:
			{
				rect.left = rect.right - outreachPoints - dividerOffset;
				rect.right += (dividerSize + outreachPoints - dividerOffset);
				isStartDivider = false;
				break;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::onContextMenu (const ContextMenuEvent& event)
{
	if(isContextMenuEnabled ())
	{
		for(int i = 0; i <= 2; i++)
		{
			String string;
			getParameter ()->getString (string, i);
			event.contextMenu.addCommandItem (string, "View", "Align", CommandDelegate<AlignView>::make (this, &AlignView::setAlignment, i));
		}
		if(!style.isCustomStyle (kPassContextMenu))
			return true;
	}
	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::setAlignment (CmdArgs args, VariantRef data)
{
	if(args.checkOnly ())
	{
		if(data == getParameter ()->getValue ())
		{
			UnknownPtr<IMenuItem> menuItem (args.invoker);
			if(menuItem)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, true);
		}
	}
	else
		getParameter ()->setValue (data, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::paramChanged ()
{
	static const int alignmentsH [] = { Alignment::kLeft, Alignment::kHCenter, Alignment::kRight };
	static const int alignmentsV [] = { Alignment::kTop,  Alignment::kVCenter, Alignment::kBottom };

	int value = param->getValue ();
	if(getStyle ().isVertical ())
		alignment.setAlignV (alignmentsV[ccl_bound (value, 0, 2)]);
	else
		alignment.setAlignH (alignmentsH[ccl_bound (value, 0, 2)]);

	if(IAttributeList* attribs = getViewState (true))
		attribs->setAttribute ("align", value);

	doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::onSize (const Point& delta)
{
	checkInvalidate (delta);

	// todo: usual attachment in other direction

	doLayout ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::onChildSized (View* child, const Point& delta)
{
	SuperClass::onChildSized (child, delta);

	doLayout ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void AlignView::calcSizeLimits ()
{
	Coord minSize = kMaxCoord;
	
	// calc largest min. size of childs
	ListForEachLinkableFast (views, View, v)
	ccl_upper_limit (minSize, Direction::getMin (v->getSizeLimits ()));
	
	// usual limits resulting from the attachment in other direction
	LayoutPrimitives::joinSubViewLimits<typename Direction::OtherDirection> (getSize (), sizeLimits, v);
	EndFor
	
	Direction::getMin (sizeLimits) = minSize;
	
	// fitsize in other direction
	if((sizeMode & Direction::OtherDirection::kFitSize) && ((sizeMode & (Direction::OtherDirection::kAttachStart|Direction::OtherDirection::kAttachEnd)) == 0))
		LayoutPrimitives::setFixedLength<typename Direction::OtherDirection> (sizeLimits, LayoutPrimitives::getMaxCoord<typename Direction::OtherDirection> (this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::calcSizeLimits ()
{
	sizeLimits.setUnlimited ();
	
	if(!views.isEmpty ())
	{
		if(getStyle ().isVertical ())
			calcSizeLimits<VerticalDirection> ();
		else
			calcSizeLimits<HorizontalDirection> ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef AlignView::getHelpIdentifier () const
{
	return View::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::resizeChild (Coord extend)
{
	storedChildSize = extend;

	if(IAttributeList* attribs = getViewState (true))
		attribs->setAttribute ("childSize", storedChildSize);
	
	doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::onMouseEnter (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (kChildSizable))
		invalidate ();
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::onMouseMove (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (kChildSizable))
	{
		Rect dstRect;
		bool isStartDivider;
		if(getDividerRect (dstRect, isStartDivider, dividerOutreach))
		{
			if(dstRect.pointInside (event.where))
			{
				ThemeCursorID cursor = getStyle ().isCommonStyle (Styles::kVertical) ? ThemeElements::kSizeVerticalCursor : ThemeElements::kSizeHorizontalCursor;
				setCursor (getTheme ().getThemeCursor (cursor));
			}
			else
			{
				setCursor ((MouseCursor*)nullptr);
			}
			
			Rect mouseOverRect;
			getClientRect (mouseOverRect);
			
			if(isStartDivider)
				mouseOverRect.right = dstRect.right;
			else
				mouseOverRect.left = dstRect.left;
			
			if(mouseOverRect.pointInside (event.where))
			{
				setMouseState (IView::kMouseOver);
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignView::onMouseLeave (const MouseEvent& event)
{
	setCursor ((MouseCursor*)nullptr);
	if(getStyle ().isCustomStyle (kChildSizable))
	{
		setMouseState (IView::kMouseNone);
		invalidate ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* AlignView::createMouseHandler (const MouseEvent& event)
{
	MouseHandler* handler = nullptr;
	if(getStyle ().isCustomStyle (kChildSizable))
	{
		Rect dstRect;
		bool isStartDivider;
		if(getDividerRect (dstRect, isStartDivider, dividerOutreach))
		{
			if(dstRect.pointInside (event.where))
			{
				bool isHorizontal = getStyle ().isCommonStyle (Styles::kHorizontal) ? true : false;
				bool isCenter = (getParameter ()->getValue ().asInt () == 1) ? true : false;
				handler = NEW ChildResizeHandler (this, isStartDivider, isHorizontal, isCenter);
			}
		}
	}
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::doLayout ()
{
	Rect clientRect;
	getClientRect (clientRect);

	if(getStyle ().isCustomStyle (kChildSizable))
	{
		if(View* view = getFirst ())
		{
			Rect rect (view->getSize ().getSize ());
			if(storedChildSize != 0)
			{
				if(getStyle ().isVertical ())
					rect.setHeight (storedChildSize);
				else
					rect.setWidth (storedChildSize);
				
				rect.bound (clientRect);
			}
			else if(view->getSizeMode () & IView::kFill)
				rect = clientRect;
			
			view->getSizeLimits ().makeValid (rect);
			
			// align child rect
			rect.align (clientRect, alignment);
			view->setSize (rect);
		}
		invalidate ();
	}
	else
	{
		ListForEachLinkableFast (views, View, v)
			Rect rect (v->getSize ());

			// try to assign full width to child if it has the "fill" option
			if(v->getSizeMode () & IView::kFill)
			{
				rect = clientRect;
				v->getSizeLimits ().makeValid (rect);
			}

			// align child rect
			rect.align (clientRect, alignment);
			v->setSize (rect);
		EndFor
	}
}
