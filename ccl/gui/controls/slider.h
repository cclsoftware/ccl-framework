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
// Filename    : ccl/gui/controls/slider.h
// Description : Slider Control
//
//************************************************************************************************

#ifndef _ccl_slider_h
#define _ccl_slider_h

#include "ccl/gui/controls/valuebar.h"

#include "ccl/base/storage/configuration.h"

namespace CCL {

//************************************************************************************************
// Slider
/** A Slider has a handle that can be moved linearly to set a value. 
Depending on the orientation (options "horizontal " or "vertical") the handle can be moved to adjust the parameter value.

The slider can have a background image or color.
An optional "colorname" parameter can be used to dynamically colorize the handle or bargraph.
Additionally to or instead of the handle, it can draw a bar graph (option "bargraph"), 
which is a partially filled rectangle that represents the current parameter value (like a ValueBar). */
//************************************************************************************************

class Slider: public ValueControl
{
public:
	DECLARE_CLASS (Slider, ValueControl)
	
	Slider (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = Styles::kVertical);
	
	enum PartCodes
	{
		kPartHandle,
		kPartTrack
	};
	
	DECLARE_STYLEDEF (customStyles)
	DECLARE_STYLEDEF (modes)
	
	PROPERTY_VARIABLE (int, instanceMode, Mode)
	
	void setXYEditDistance (Coord distance);
	
	// Control
	float getValue () const override;
	void setValue (float v, bool update = true) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool canHandleDoubleTap () const override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	ThemeRenderer* getRenderer () override;
	bool onMouseWheel (const MouseWheelEvent& event) override;

	bool onGesture (const GestureEvent& event) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;
	
	
protected:
	void getHandlerParams (const GUIEvent& event, const Point& where, int& mode, Point& clickOffset, int& max, bool& handleClicked);
	
	bool isTouchResetEnabled;
	Coord xyEditDistance;

	friend class SliderMouseHandler;
	friend class SliderRenderer;
	
	static const Configuration::IntValue sliderMode;
};

//************************************************************************************************
// RangeSlider
/** In addition to a slider a RangeSlider uses a second parameter, set by the xml-attribute "name2".
The RangeSlider itself always ensures a positive range.
The "validate" option ensures a positive range when values can be set externally.
The "limit" option delimits the value ranges. Changing the upper value cannot change the lower value and vice versa. */
//************************************************************************************************

class RangeSlider: public Slider
{
public:
	DECLARE_CLASS (RangeSlider, Slider)
	
	RangeSlider (const Rect& size = Rect (), IParameter* _param = nullptr, IParameter* _param2 = nullptr, StyleRef style = Styles::kVertical);
	~RangeSlider ();
	
	DECLARE_STYLEDEF (customStyles)
	
	enum RangeSliderPartCodes
	{
		kTrackBack,
		kFirstHandle,
		kSecondHandle
	};
	
	struct EditHandlerSetup
	{
		int max;
		IParameter* mainParam;
		IParameter* additionalParam;
		bool moveLowerHandle;
		bool moveUpperHandle;
		bool xyEditing;
		Coord xEditDistance;
		Coord yEditDistance;	
		bool useLimits;
		bool invertible;
		bool handleClicked;
		Point clickOffset;
	};
	
	float getSecondValue () const;
	void setSecondValue (float v, bool update);
	bool updateTooltip (bool showEditValue = false);

	// Slider
	void performReset () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	ThemeRenderer* getRenderer () override;
	void attached (View* parent) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;
	
protected:
	void updateStyle ();
	void validateParams (IParameter* master);
	EditHandlerSetup getEditHandlerSetup (const GUIEvent& event, PointRef where);
	
	String originalTooltip;
	
	IParameter* param2;
	int handleOutreach;
};

} // namespace CCL

#endif // _ccl_slider_h
