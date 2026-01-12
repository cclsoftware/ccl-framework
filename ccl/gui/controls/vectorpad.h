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
// Filename    : ccl/gui/controls/vectorpad.h
// Description : Vector Pad (XY-Control)
//
//************************************************************************************************

#ifndef _ccl_vectorpad_h
#define _ccl_vectorpad_h

#include "ccl/gui/controls/valuebar.h"
#include "ccl/base/storage/configuration.h"

namespace CCL {

//************************************************************************************************
// VectorPad
/** A Control with two parameters interpreted as coordinates of a handle.
The values of the two parameters are interpreteted as x and y coordinates in the view area.
A handle is displayed at the resulting point and can be moved using the mouse. */
//************************************************************************************************

class VectorPad: public Control
{
public:
	DECLARE_CLASS (VectorPad, Control)

	VectorPad (RectRef size = Rect (), IParameter* param = nullptr, IParameter* yParam = nullptr, StyleRef style = 0);
	~VectorPad ();

	enum PartCodes
	{
		kPartHandle,
		kPartBack
	};

	IParameter* getYParameter () const;
	void setYParameter (IParameter* yParam);

	float getYValue () const;
	void setYValue (float v, bool update = true);

	float getXValue () const;
	void setXValue (float v, bool update = true);

	// Control
	void onSize (const Point& delta) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool canHandleDoubleTap () const override;
	void performReset () override;
	void paramChanged () override;

protected:
	IParameter* yParam;
	static const Configuration::IntValue sliderMode;
};

} // namespace CCL

#endif // _ccl_vectorpad_h
