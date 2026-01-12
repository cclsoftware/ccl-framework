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
// Filename    : ccl/gui/controls/updownbox.h
// Description : Up/Down Box
//
//************************************************************************************************

#ifndef _ccl_updownbox_h
#define _ccl_updownbox_h

#include "ccl/gui/controls/button.h"

namespace CCL {

//************************************************************************************************
// UpDownButton
/** Increments or decrements a parameter. 
On each click the parameter is incremented (option "increment", the default) or decremented (option "decrement"). */
//************************************************************************************************

class UpDownButton: public Button
{
public:
	DECLARE_CLASS (UpDownButton, Button)
	DECLARE_STYLEDEF (customStyles)

	UpDownButton (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	// Button
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ThemeRenderer* getRenderer () override;
	void CCL_API push () override;
};

//************************************************************************************************
// UpDownBox
/** Combination of two UpDownButtons. 
Contains an Up button and a Down button, horizontally or vertcically arranged.
\see UpDownButton */
//************************************************************************************************

class UpDownBox: public View
{
public:
	UpDownBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	// View
	void onVisualStyleChanged () override;

private:
	void setButtonStyle (UpDownButton* button, StringID backgroundName, VisualStyle* visualStyle);
};

} // namespace CCL

#endif // _ccl_updownbox_h
