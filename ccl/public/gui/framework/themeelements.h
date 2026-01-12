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
// Filename    : ccl/public/gui/framework/themeelements.h
// Description : Theme Elements
//
//************************************************************************************************

#ifndef _ccl_themeelements_h
#define _ccl_themeelements_h

#include "ccl/public/text/cstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Element Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ThemeElements
{
	/** Metric constants. */
	DEFINE_ENUM (MetricID)
	{
		kLayoutMargin,				///< margin
		kLayoutSpacing,				///< spacing
		kButtonWidth,				///< button width
		kButtonHeight,				///< button height
		kTextBoxHeight,				///< textbox height
		kCheckBoxSize,				///< checkbox size
		kScrollBarSize,				///< scrollbar size
		kSliderHandleSize,			///< slider handle size
		kDividerSize,				///< divider size
		kDividerOutreach,			///< range outside a divider where it can still be operated
		kHeaderHeight,			    ///< list header size
		kBorderSize,                ///< size of border of listview etc.
		kSystemStatusBarHeight,		///< height of system status bar (typically on mobile platforms), or 0 if not available
		kSystemNavigationBarHeight,	///< height of system navigation bar (typically on mobile platforms), or 0 if not available
		kSystemMarginLeft,			///< width of reserved system area on left screen side (typically on mobile platforms)
		kSystemMarginRight,			///< width of reserved system area on right screen side (typically on mobile platforms),
		kTitleBarHeight,			///< height of a window title bar
		kNumMetrics
	};

	/** Color constants. */
	DEFINE_ENUM (ColorID)
	{
		kSelectionColor,			///< solid selection color
		kSelectionTextColor,		///< selection text color
		kAlphaSelectionColor,		///< color for translucent (overlayed) selections
		kAlphaCursorColor,			///< color for translucent (overlayed) cursors
		kHyperlinkColor,			///< hyperlink color
		kTooltipBackColor,			///< tooltip background color
		kTooltipTextColor,			///< tooltip text color
		kListViewBackColor,			///< color for listview background
		kPushButtonTextColor,		///< text color for buttons
		kNumColors
	};
	
	/** Font constants. */
	DEFINE_ENUM (FontID)
	{
		kMenuFont,					///< font used by menus
		kNumFonts
	};

	/** Cursor constants. */
	DEFINE_ENUM (CursorID)
	{
		// Cursors supported on all platforms.
		kArrowCursor,				///< arrow cursor
		kWaitCursor,				///< wait cursor (hourglass)
		kCrosshairCursor,			///< crosshair cursor
		kPointhandCursor,			///< pointhand cursor
		kSizeHorizontalCursor,		///< horizontal sizing cursor
		kSizeVerticalCursor,		///< vertical sizing cursor
		kSizeLeftUpRightDownCursor,	///< diagonal sizing cursor from upper left to lower right
		kSizeLeftDownRightUpCursor,	///< diagonal sizing cursor from lower left to upper right
		kTextCursor,				///< text edit cursor

		// Cursors supported on all platforms but might differ in their visual representation
		kSizeUpCursor,				///< an arrow pointing upwards or a vertical double-arrow (kSizeVerticalCursor)
		kSizeRightCursor,			///< an arrow pointing to the right or a horizontal double-arrow (kSizeHorizontalCursor)
		kSizeDownCursor,			///< an arrow pointing downwards or a vertical double-arrow (kSizeVerticalCursor)
		kSizeLeftCursor,			///< an arrow pointing to the left or a horizontal double-arrow (kSizeHorizontalCursor)
		kSizeLeftUpCursor,			///< an arrow pointing to the upper left or a diagonal double-arrow (kSizeLeftUpRightDownCursor)
		kSizeLeftDownCursor,		///< an arrow pointing to the lower left or a diagonal double-arrow (kSizeLeftDownRightUpCursor)
		kSizeRightUpCursor,			///< an arrow pointing to the upper right or a diagonal double-arrow (kSizeLeftDownRightUpCursor)
		kSizeRightDownCursor,		///< an arrow pointing to the lower right or a diagonal double-arrow (kSizeLeftUpRightDownCursor)

		// Optional cursors not supported on all platforms. Use with caution (!)
		kCopyCursor,				///< copy cursor (+)
		kNoDropCursor,				///< dropping is not allowed cursor
		kGrabCursor,				///< cursor indicating that a draggable element is below the cursor, e.g. an open hand
		kGrabbingCursor,			///< cursor indicating that the user is dragging an element, e.g. a closed hand
		kZoomInCursor,				///< zoom-in cursor, e.g. a magnifier glass with a (+)
		kZoomOutCursor,				///< zoom-out cursor, e.g. a magnifier glass with a (-)

		kNumCursors
	};

	/** Element states. */
	DEFINE_ENUM (ElementState)
	{
		kNormal,					///< normal
		kPressed,					///< pressed (mouse down)
		kMouseOver,					///< mouse over, but not pressed
		kDisabled,					///< disabled
		kFocused,					///< has focus
		kNumElementStates
	};

	/** Element constants. */
	DEFINE_ENUM (ElementID)
	{
		kPushButton,				///< push button
		kPushButtonOn,				///< push button, on state
		kCheckBoxNormal,			///< checkbox (unchecked)
		kCheckBoxChecked,			///< checkbox (with checkmark)
		kRadioButtonNormal,			///< radio button (unchecked)
		kRadioButtonChecked,		///< radio button (with checkmark)
		kTreeViewExpandButton,		///< treeview expand button
		kTreeViewExpandButtonOn,	///< treeview expand button, on state

		kNumThemeElements
	};

	/** TreeView element states. */
	enum TreeElementStates
	{
		kTreeItemExpanded  = kPressed,	///< expanded tree item
		kTreeItemCollapsed = kNormal	///< collapsed tree item
	};

} // namespace ThemeElements

/** Theme metric type. */
typedef ThemeElements::MetricID ThemeMetricID;

/** Theme color type. */
typedef ThemeElements::ColorID ThemeColorID;

/** Theme font type. */
typedef ThemeElements::FontID ThemeFontID;

/** Theme cursor type. */
typedef ThemeElements::CursorID ThemeCursorID;

/** Theme element type. */
typedef ThemeElements::ElementID ThemeElementID;

/** Theme element state. */
typedef ThemeElements::ElementState ThemeElementState;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Name Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ThemeNames
{
	DEFINE_STRINGID (kNormal, "normal")
	DEFINE_STRINGID (kPressed, "pressed")
	DEFINE_STRINGID (kMouseOver, "mouseover")
	DEFINE_STRINGID (kDisabled, "disabled")
	DEFINE_STRINGID (kFocused, "focus")
	
	DEFINE_STRINGID (kNormalOn, "normalOn")
	DEFINE_STRINGID (kPressedOn, "pressedOn")
	DEFINE_STRINGID (kMouseOverOn, "mouseoverOn")
	DEFINE_STRINGID (kDisabledOn, "disabledOn")
	DEFINE_STRINGID (kFocusedOn, "focus") // same as kFocused
	
	DEFINE_STRINGID (kStandard, "Standard")
	DEFINE_STRINGID (kStandardFont, "StandardUI")

	DEFINE_STRINGID (kMain, "Main") ///< main color scheme

	// images currently available in framwork theme - to be cleaned up at some point.
	DEFINE_STRINGID (kItemViewFolderIcon, "ItemView.FolderIcon")
	DEFINE_STRINGID (kItemViewFolderIconOpen, "ItemView.FolderIconOpen")
	DEFINE_STRINGID (kUserOptionIcon, "UserOptionIcon")		

} // namespace ThemeNames

} // namespace CCL

#endif // _ccl_themeelements_h
