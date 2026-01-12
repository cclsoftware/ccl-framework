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
// Filename    : ccl/gui/layout/divider.h
// Description : Divider
//
//************************************************************************************************

#ifndef _ccl_divider_h
#define _ccl_divider_h

#include "ccl/gui/controls/control.h"

namespace CCL {

class AnchorLayoutView;

//************************************************************************************************
// Divider
/** A view that can be dragged to divide screen space between two neighbour views.
	To be used inside a layout view */
//************************************************************************************************

class Divider: public Control
{
public:
	DECLARE_CLASS (Divider, Control)
	DECLARE_METHOD_NAMES (Divider)
	DECLARE_STYLEDEF (customStyles)

	Divider (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	PROPERTY_VARIABLE (Coord, outreach, Outreach) ///< range outside where the divider can still be operated

	void moveTo (Coord pos); ///< left/top edge of divider
	void moveBy (Coord offset);
	
	void triggerSyncSlaves ();
	bool canResizeViews () const;

	DECLARE_STRINGID_MEMBER (kQueryDividerLimits)
	DECLARE_STRINGID_MEMBER (kQueryPreferredSizes)
	DECLARE_STRINGID_MEMBER (kSyncSlaves)
	DECLARE_STRINGID_MEMBER (kHasLayoutState)

	// Control
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	void attached (View* parent) override;
	void onMove (const Point& delta) override;
	void onSize (const Point& delta) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	void paramChanged () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	struct Context;
	struct Limits;
	struct PreferredSizes;
	template<int direction> class DividerMouseHandler;

	enum { kIsSyncing = 1<<(kLastPrivateFlag + 1) }; ///< to distinguish user manipulation from syncing to parameter

	void onManipulationDone ();
	Coord getPosition () const; ///< left or top edge
	template<class Direction> AnchorLayoutView* findLayoutContext (View*& leftView, View*& rightView);
	template<class Direction> void moveBy (Coord offset);

	int positionToValue (Coord position) const;
	Coord valueToPosition (int value) const;
	
	void jump (int direction, bool canInvert);
};

} // namespace CCL

#endif // _ccl_divider_h
