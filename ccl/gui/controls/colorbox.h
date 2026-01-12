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
// Filename    : ccl/gui/controls/colorbox.h
// Description : Color Box
//
//************************************************************************************************

#ifndef _ccl_colorbox_h
#define _ccl_colorbox_h

#include "ccl/gui/controls/control.h"
#include "ccl/gui/graphics/imaging/coloredbitmap.h"

namespace CCL {
	
//************************************************************************************************
// ColorBox
/** A simple colored area where the color can be derived from a color parameter. 
A ColorBox fills it's whole area with a color.

The parameter can be a special color parameter that directly tells the color, 
or an integer parameter that encodes the color as a 32 bit integer code.

Additional to the parameter that defines color, the ColorBox can have an additional parameter "selectname". 
A click in the ColorBox sets that parameter to it's maximum value.

With the option "border", the ColorBox draws frame of "strokewidth" pixels in the "forecolor", 
or "hilitecolor" if the "selectname" parameter has it's maximum value. */
//************************************************************************************************

class ColorBox: public Control
{
public:
	DECLARE_CLASS (ColorBox, Control)

	ColorBox (const Rect& size = Rect (), IParameter* colorParam = nullptr, StyleRef style = 0);
	~ColorBox ();
	
	DECLARE_STYLEDEF (customStyles)

	IParameter* getSelectParam () const;
	void setSelectParam (IParameter* selectParam);

	PROPERTY_VARIABLE (Coord, radius, Radius)	///< radius for rounded corners (default is 0)
	
	// Control
	void draw (const UpdateRgn& updateRgn) override;
	bool onMouseDown  (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;

protected:

	AutoPtr<ColoredBitmap> coloredBackground;
	IParameter* selectParam;
};

} // namespace CCL

#endif // _ccl_colorbox_h
