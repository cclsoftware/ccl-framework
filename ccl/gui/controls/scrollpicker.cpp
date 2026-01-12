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
// Filename    : ccl/gui/controls/scrollpicker.cpp
// Description : Scroll Picker
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/scrollpicker.h"
#include "ccl/gui/controls/controlaccessibility.h"

#include "ccl/gui/theme/renderer/scrollpickerrenderer.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"

using namespace CCL;

//************************************************************************************************
// ScrollPicker
//************************************************************************************************

BEGIN_STYLEDEF (ScrollPicker::customStyles)
	{"wraparound",	Styles::kScrollPickerBehaviorWrapAround},
	{"flat",		Styles::kScrollPickerAppearanceFlatBarrel},
	{"digitmode",	Styles::kScrollPickerAppearanceDigitMode},
	{"hideimage",	Styles::kScrollPickerAppearanceHideImage},
END_STYLEDEF

BEGIN_VISUALSTYLE_CLASS (ScrollPickerStyle, VisualStyle, "ScrollPickerStyle")
	ADD_VISUALSTYLE_METRIC ("itemSize")				///< optional explicit item size (height/width in vertical/horizontal case)
	ADD_VISUALSTYLE_COLOR ("backcolor")				///< backcolor for the control
	ADD_VISUALSTYLE_COLOR ("centerBackcolor")		///< backcolor for the center view
	ADD_VISUALSTYLE_METRIC ("centerSize")			///< optional height/width of the vertical/horizontal center view, where the current value is displayed
	ADD_VISUALSTYLE_METRIC ("minimalEditWidth")		///< optional minimal width for the horizontal scrollpicker edit control
	ADD_VISUALSTYLE_METRIC ("centerOffset")			///< optional vertical/horizontal offset of the center view - default is (clientSize - centerSize) / 2
	ADD_VISUALSTYLE_METRIC ("gradientThickness")	///< thickness of the fade-out area at the border of the barrel
	ADD_VISUALSTYLE_COLOR ("centerTextColor")		///< textcolor for the center view
	ADD_VISUALSTYLE_COLOR ("barrelTextColor")		///< textcolor for the neighboring unselected values on the barrel
	ADD_VISUALSTYLE_FONT ("centerTextFont")			///< center view font
	ADD_VISUALSTYLE_FONT ("barrelTextFont")			///< barrel font
	ADD_VISUALSTYLE_IMAGE ("centerOverlayImage")	///< additional optional overlay for the center view
END_VISUALSTYLE_CLASS (ScrollPickerStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ScrollPicker, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollPicker::ScrollPicker (RectRef bounds, IParameter* param, StyleRef style)
: Control (bounds, param, style),
  visibleItems (-1),
  itemSize (20),
  minimalEditWidth (20),
  wrapAround (false),
  editBox (nullptr),
  applyParameter (nullptr),
  returnKeyType (Styles::kReturnKeyDefault)
{
	wantsFocus (true);

	bool valid = (getParameter ()->getType () == IParameter::kInteger) || (getParameter ()->getType () == IParameter::kList);
	ASSERT (valid) // parameter should be an intParam
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollPicker::~ScrollPicker ()
{
	if(animator)
	{
		animator->stopAnimation (true);
		animator->stopAnimation (false);
	}

	if(editBox)
	{
		removeView (editBox);
		safe_release (editBox);
	}
	
	setApplyParameter (nullptr);

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::setApplyParameter (IParameter* p)
{
	if(applyParameter != p)
		share_and_observe_unknown (this, applyParameter, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPicker::getItemSize () const
{
	return itemSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ScrollPicker::getVisibleItemCountFlat () const
{
	return (visibleItems > 0) ? visibleItems : (getStyle ().isVertical () ? size.getHeight () : size.getWidth ()) / (float)getItemSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollPicker::getScrollPosition () const
{
	return animator ? animator->getOverScrollPosition (getStyle ().isVertical ()) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollPicker::getValueCount () const
{
	return getValueRange () + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollPicker::getValueRange () const
{
	int range = param->getMax ().asInt () - param->getMin ().asInt ();
	
	if(range <= 0)
	{
		ASSERT (range)
		return 1;
	}
	
	return range;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollPicker::getOverScrollMargin () const
{
	static const int kNumberOfItemsForOverScroll = 3;
	return kNumberOfItemsForOverScroll * getItemSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::isEditing () const
{
	return editBox && editBox->isEditing ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::isCarouselMode () const
{
	if(UnknownPtr<IPaletteProvider> p = getParameter ())
	{
		if(style.isCustomStyle (Styles::kScrollPickerAppearanceHideImage) == false)
			return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ScrollPicker::getScrollParameter (bool verticalDirection) const
{
	bool vertical = getStyle ().isVertical ();
	if(vertical == verticalDirection)
		return getParameter ();
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::getOverScrollMargins (Rect& margins) const
{
	if(getStyle ().isVertical ())
	{
		margins.left = 0;
		margins.top = getOverScrollMargin ();
		margins.right = 0;
		margins.bottom = getOverScrollMargin ();
	}
	else
	{
		margins.left = getOverScrollMargin ();
		margins.top = 0;
		margins.right = getOverScrollMargin ();
		margins.bottom = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::getScrollRange (Point& range) const
{
	if(getStyle ().isVertical ())
	{
		range.x = 0;
		range.y = getItemSize () * getValueRange ();
	}
	else
	{
		range.x = getItemSize () * getValueRange ();
		range.y = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::getSnapSize (Point& snapSize) const
{
	snapSize.x = getItemSize ();
	snapSize.y = getItemSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::isWrapAround () const
{
	return wrapAround;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::onOverScroll (bool, Coord)
{
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ScrollPicker::getTextParameter () const
{
	return getParameter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ScrollPicker::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kScrollPickerRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::attached (View* parent)
{
	updateStyle ();
	setupOverScrollAnimator ();

	if(applyParameter == nullptr && isCarouselMode () == false)
	{
		static_cast<ScrollPickerRenderer*> (getRenderer ())->init (this);
		Rect r;
		getRenderer ()->getPartRect (this, kPartCenter, r);
		
		if(getStyle ().isVertical () == false)
		{
			if(minimalEditWidth > r.getWidth ())
			{
				r.offset ((r.getWidth () - minimalEditWidth) / 2);
				r.setWidth (minimalEditWidth);
			}
		}
		
		StyleFlags editBoxStyle (getStyle ());
		editBoxStyle.setCommonStyle (Styles::kTransparent);
		editBoxStyle.setCustomStyle (Styles::kEditBoxAppearanceHideText);
		editBoxStyle.setCustomStyle (Styles::kTextBoxAppearanceMultiLine, false);

		editBox = NEW EditBox (r, param, editBoxStyle);
		AutoPtr<VisualStyle> editBoxVisualStyle (NEW VisualStyle);
		editBoxVisualStyle->copyFrom (getVisualStyle ());
		editBox->setVisualStyle (editBoxVisualStyle);
		addView (editBox);
	}

	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::onSize (const Point& delta)
{
	// discard renderer when size changes
	safe_release (renderer);
	invalidate ();
	
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::onKeyDown (const KeyEvent& event)
{
	if(animator == nullptr)
		return SuperClass::onKeyDown (event);
	
	switch(event.vKey)
	{
	case VKey::kLeft : 
		if(getStyle ().isHorizontal ())
		{
			animator->decrement ();
			return true;
		}
		break;
	case VKey::kRight : 
		if(getStyle ().isHorizontal ())
		{
			animator->increment ();
			return true;
		}
		break;
	case VKey::kUp : 
		if(getStyle ().isVertical ())
		{
			animator->decrement ();
			return true;
		}
		break;
	case VKey::kDown : 
		if(getStyle ().isVertical ())
		{
			animator->increment ();
			return true;
		}
		break;
	}
	
	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::onMouseWheel (const MouseWheelEvent& event)
{	
	if(animator)
		return animator->onMouseWheel (event);
		
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ScrollPicker::createMouseHandler (const MouseEvent& event)
{
	if(canCreateHandler ())
		return animator->createMouseHandler (this, createClickAction (event.where));
	
	return NEW NullMouseHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ScrollPicker::createTouchHandler (const TouchEvent& event)
{
	auto touch = event.touches.getTouchInfoByID (event.touchID);
	if(canCreateHandler () && touch)
	{
		Point where (touch->where);
		windowToClient (where);
		return animator->createTouchHandler (this, createClickAction (where));
	}

	return NEW NullTouchHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::canCreateHandler ()
{
	if(getRenderer () == nullptr || animator == nullptr)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClickAction* ScrollPicker::createClickAction (PointRef where)
{
	Point clickOffset;
	int partCode = getRenderer ()->hitTest (this, where, &clickOffset);
	
	switch(partCode)
	{
	case kPartUpperLeft:
		return ClickAction::make ([this](){animator->decrement ();});
	case kPartLowerRight:
		return ClickAction::make ([this](){animator->increment ();});
	case kPartCenter:
		if(applyParameter)
			return ClickAction::make ([this]()
			{
				applyParameter->setValue (applyParameter->getMax (), true);
				applyParameter->setValue (applyParameter->getMin ());
			});
		return nullptr;
			
	default:
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	if(event.eventType == DisplayChangedEvent::kResolutionChanged)
	{
		safe_release (renderer);
		invalidate ();
	}
	else
		SuperClass::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPicker::onFocus (const FocusEvent& event)
{
	Control::onFocus (event); // update focus + invalidate

	if(event.eventType == FocusEvent::kKillFocus)
	{
		if(editBox)
			editBox->onFocus (event);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::updateStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();
	itemSize = vs.getMetric<Coord> ("itemSize", 20);
	minimalEditWidth = vs.getMetric<Coord> ("minimalEditWidth", vs.getMetric<Coord> ("centerSize", itemSize));

	itemSize *= getZoomFactor ();
	minimalEditWidth *= getZoomFactor ();

	// options
	wrapAround = style.isCustomStyle (Styles::kScrollPickerBehaviorWrapAround);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollPicker::setupOverScrollAnimator ()
{
	animator = NEW OverScrollAnimator (this, getStyle ().isVertical () ? Styles::kVertical : Styles::kHorizontal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ScrollPicker::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ValueControlAccessibilityProvider (*this);
	return accessibilityProvider;
}
