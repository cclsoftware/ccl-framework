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
// Filename    : ccl/gui/controls/knob.h
// Description : Knob Control
//
//************************************************************************************************

#ifndef _ccl_knob_h
#define _ccl_knob_h

#include "ccl/gui/controls/slider.h"

namespace CCL {

//************************************************************************************************
// Knob
/** A Knob is a circular element that rotates depending on the parameter value. 
A typical knob draws a static background image, with a partial circle around the center of the knob (set via option or visualstyle "circle").
The circle color is the "hilitecolor" or set from the optional "colorname" parameter.
The visible angle range represents the parameter value. 
Additionally, a line from the center to the end of the circle can be drawn (option "indicator").

A completely different approach to display a knob can be taken with the "filmstrip" option. 
In this case the full knob area is filled with one frame of the "filmstrip" bitmap, 
with the frame index being chosen based on the parameter value. */
//************************************************************************************************

class Knob: public Slider
{
public:
	DECLARE_CLASS (Knob, Slider)

	Knob (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	~Knob ();

	DECLARE_STYLEDEF (customStyles)

	void setOffsetReferenceParameter (IParameter* p);
	float getOffsetReferenceValue () const;
	bool hasOffsetReference () const { return offsetReference ? true : false; }
	
	// Control
	void calcAutoSize (Rect& r) override;
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;

protected:
	friend class KnobMouseHandler;

	IParameter* offsetReference;
};

} // namespace CCL

#endif // _ccl_knob_h
