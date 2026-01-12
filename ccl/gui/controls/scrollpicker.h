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
// Filename    : ccl/gui/controls/scrollpicker.h
// Description : Scroll Picker
//
//************************************************************************************************

#ifndef _ccl_scrollpicker_h
#define _ccl_scrollpicker_h

#include "ccl/gui/controls/editbox.h"

#include "ccl/gui/views/overscrollanimator.h"

namespace CCL {

//************************************************************************************************
// ScrollPicker
/** A ScrollPicker is value is a control displaying its current value in the center of an underlying scrollview.
	Like ValueBox or Knob it allows changing the value by dragging the mouse.
	Can be used in vertical (default) or horizontal orientation, w/o warparound option (endless dial).
 
 \code{.xml}
 # standard scrollpicker usage
 <ScrollPicker name="foo" size="0,0,110,110"/>

 # Second horizontal example using the wraparound option
 <Style name="HorizontalScrollPickerStyle" inherit="Standard.ScrollPicker">
	 <Metric name="itemSize" value="30"/>
	 <Metric name="centerSize" value="35"/>
 </Style>
 <ScrollPicker name="bar" options="wraparound horizontal" style="HorizontalScrollPickerStyle" size="0,0,110,20"/>
 \endcode
 */
//************************************************************************************************

class ScrollPicker: public Control,
					public IOverScrollAnimatable,
					public ITextParamProvider
{
public:
	DECLARE_CLASS (ScrollPicker, Control)

	ScrollPicker (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	~ScrollPicker ();

	DECLARE_STYLEDEF (customStyles)
	
	Coord getItemSize () const;
	int getScrollPosition () const;
	int getValueCount () const;
	float getVisibleItemCountFlat () const;
	Coord getOverScrollMargin () const;
	bool isEditing () const;
	bool isCarouselMode () const;
	void setApplyParameter (IParameter* p);
	
	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	
	enum ScrollPickerParts
	{
		kPartNone = 0,
		kPartCenter = 1,
		kPartUpperLeft = 2,
		kPartLowerRight = 3
	};

	// IOverScrollAnimatable
	IParameter* getScrollParameter (bool verticalDirection) const override;
	void getOverScrollMargins (Rect& margins) const override;
	void getScrollRange (Point& range) const override;
	void getSnapSize (Point& snapSize) const override;
	bool isWrapAround () const override;
	void onOverScroll (bool verticalDirection, Coord scrollPosition) override;
	
	// ITextParamProvider
	IParameter* getTextParameter () const override;
	
	// Control
	ThemeRenderer* getRenderer () override;
	void attached (View* parent) override;
	void onSize (const Point& delta) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	
	CLASS_INTERFACE2 (IOverScrollAnimatable, ITextParamProvider, Object)
	
protected:
	AutoPtr<OverScrollAnimator> animator;
	
	Coord itemSize;
	Coord minimalEditWidth;
	bool wrapAround;
	float visibleItems;
	EditBox* editBox;
	IParameter* applyParameter;
	
	bool canCreateHandler ();
	void updateStyle ();
	int getValueRange () const;
	void setupOverScrollAnimator ();

	ClickAction* createClickAction (PointRef where);
};

DECLARE_VISUALSTYLE_CLASS (ScrollPickerStyle)

} // namespace CCL

#endif // _ccl_scrollpicker_h
