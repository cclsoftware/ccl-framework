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
// Filename    : ccl/gui/controls/scrollbar.h
// Description : Scrollbar
//
//************************************************************************************************

#ifndef _ccl_scrollbar_h
#define _ccl_scrollbar_h

#include "ccl/gui/controls/control.h"

namespace CCL {

interface IScrollParameter;

//************************************************************************************************
// ScrollBar styles
//************************************************************************************************

namespace Styles
{
	enum ScrollBarStyles
	{
		kScrollBarBehaviorJump		= 1<<1,	///< jump to clicked position when clicked outside handle (instead of page scrolling)
		kScrollBarBehaviorPassive	= 1<<2	///< scrollbar (pagecontrol) is used as passive control - mouse clicks fall thru
	};
};

//************************************************************************************************
// ScrollBar
/** Scrollbars are used in a ScrollView to indicate and manipulate the scrolling postion. 
Note: A Scrollview can automatically create ScrollBars or ScrollButtons.  */
//************************************************************************************************

class ScrollBar: public Control
{
public:
	DECLARE_CLASS (ScrollBar, Control)

	ScrollBar (const Rect& size = Rect (), IParameter* param = nullptr,
			   StyleRef style = StyleFlags (Styles::kVertical));

	IScrollParameter* getScrollParam () const;

	enum ScrollBarMouseStates
	{
		kHandlePressed = 100,
		kButtonDownPressed,
		kButtonUpPressed,
		kPageDownPressed,
		kPageUpPressed
	};
	
	enum ScrollBarParts
	{
		kPartNone = 0,
		kPartHandle = 1,		///< scrollbar handle
		kPartButtonDown = 2,	///< down or left button
		kPartButtonUp = 3,		///< up or right button
		kPartPageUp = 4,		///< page up or page left button
		kPartPageDown = 5,		///< page down or page right button
		kPartTrackingArea = 6	///< area where scrollbar handle can be moved
	};

	DECLARE_STYLEDEF (customStyles)
	DECLARE_STYLEDEF (partNames)

	PROPERTY_OBJECT (Point, mouseOverPosition, MouseOverPosition)

	// Control
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	void attached (View* parent) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	void onSize (const Point& delta) override;
};

//************************************************************************************************
// ScrollButton
/** A Button that can be used to perform a single scroll step per click. 
When connected to a scroll parameter, this button perfroms a single scroll step when clicked.

The attribute "part" ("buttonup", "buttondown") specifies the scroll direction 
(these 2 values are also used for a horizontal scrollview for the left / right direction). */
//************************************************************************************************

class ScrollButton: public ScrollBar
{
public:
	DECLARE_CLASS (ScrollButton, ScrollBar)

	ScrollButton (const Rect& size = Rect (), IParameter* param = nullptr, int partCode = kPartButtonDown);

	PROPERTY_VARIABLE (int, partCode, PartCode)

	// Control
	ThemeRenderer* getRenderer () override;
};

//************************************************************************************************
// PageControl
/** A PageControl indicates the scroll position of a ScrollView.
The PageControl must be used with a scroll parameter. 
It draws a row of dots, with one dot per scroll page, where the current page is highlited.*/
//************************************************************************************************

class PageControl: public ScrollBar
{
public:
	DECLARE_CLASS_ABSTRACT (PageControl, ScrollBar)

	PageControl (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = StyleFlags (Styles::kVertical));
	
	void push ();
	int getNumPages () const;
	int getCurrentPage () const;
	
	// ScrollBar
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ThemeRenderer* getRenderer () override;
};

} // namespace CCL

#endif // _ccl_scrollbar_h
