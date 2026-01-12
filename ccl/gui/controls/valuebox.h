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
// Filename    : ccl/gui/controls/valuebox.h
// Description : Value Box
//
//************************************************************************************************

#ifndef _ccl_valuebox_h
#define _ccl_valuebox_h

#include "ccl/gui/controls/editbox.h"

namespace CCL {

//************************************************************************************************
// ValueBox
/** A ValueBox is an EditBox with a special mouse editing ability for numeric paramters.
In addition to an EditBox for displaying and editing the text representation of a parameter, 
the ValueBox allows changing the value by dragging the mouse up and down, like a Knob does.  */
//************************************************************************************************

class ValueBox: public EditBox
{
public:
	DECLARE_CLASS (ValueBox, EditBox)

	ValueBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	
	DECLARE_STYLEDEF (customStyles)
	
	// Control
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool canHandleDoubleTap () const override;
	
	void setXYEditDistance (Coord distance);

private:
	int xyEditDistance;

	friend class ValueBoxTouchHandler;
};

} // namespace CCL

#endif // _ccl_valuebox_h
