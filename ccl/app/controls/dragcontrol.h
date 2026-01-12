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
// Filename    : ccl/app/controls/dragcontrol.h
// Description : Drag Control
//
//************************************************************************************************

#ifndef _ccl_dragcontrol_h
#define _ccl_dragcontrol_h

#include "ccl/app/controls/usercontrol.h"

namespace CCL {

interface IHelpInfoCollection;

//************************************************************************************************
// DragControl
//************************************************************************************************

class DragControl: public UserControl
{
public:
	DECLARE_CLASS_ABSTRACT (DragControl, UserControl)

	DragControl (RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~DragControl ();

	PROPERTY_STRING (dragTooltip, DragTooltip)			///< tooltip shown when drag is possible
	PROPERTY_MUTABLE_CSTRING (cursorName, CursorName)	///< mouse cursor shown when drag is possible
	PROPERTY_VARIABLE (int, modifier, Modifier)			///< modifier key required fro dragging (KeyState::Flags), default: 0
	PROPERTY_SHARED_AUTO (IUnknown, source, Source)		///< (optional) source object passsed to IDragSession in prepareDrag
	PROPERTY_SHARED_AUTO (IUnknown, dataItem, DataItem)	///< (optional) passed as data item to IDragSession in prepareDrag

private:
	bool isArmed;
	String originalTooltip;
	IHelpInfoCollection* helpCollection;

	bool checkArmed (const MouseEvent& event);

protected:
	// UserControl
	void attached (IView* parent) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event) override;
	void draw (const DrawEvent& event) override;

	void doDrag (int inputDevice);
	void updateHelp (const MouseEvent& event);
	void updateStyle ();

	/// to be overridden:
	virtual bool canDrag (const MouseEvent& event);				///< decide if dragging is allowed (e.g. depening on modifiers)
	virtual void beforeDrag (const MouseEvent& event);			///< called on mousedown before detectDrag (for immediate actions)
	virtual void prepareDrag (IDragSession& session);			///< called on mousedown before starting to drag: set data items and source here; default impl. uses source and dataItem
	
	bool retriggerTooltip;
	int paddingX;
	int paddingY;
	SharedPtr<CCL::IImage> backgroundImage;
};

} // namespace CCL

#endif // _ccl_dragcontrol_h
