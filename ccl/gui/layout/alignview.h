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
// Filename    : ccl/gui/layout/alignview.h
// Description : View with switchable alignment for childs
//
//************************************************************************************************

#ifndef _ccl_alignview_h
#define _ccl_alignview_h

#include "ccl/gui/controls/control.h"

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

//************************************************************************************************
// AlignView
/** Aligns child views according to an alignment parameter.
Depending on the AlignView's orientation ("horizontal" or "vertical"), the parameter selects one of the aligmnents
"left", "hcenter", "right" or "top", "vcenter", "bottom" to align the child views relative to the parent area.
The user can choose the aligment via the context menu. */
//************************************************************************************************

class AlignView: public Control
{
public:
	DECLARE_CLASS (AlignView, Control)

	AlignView (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	PROPERTY_OBJECT (Alignment, alignment, Alignment)
	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	void restoreState ();

	// Control
	bool onContextMenu (const ContextMenuEvent& event) override;
	void paramChanged () override;
	void onSize (const Point& delta) override;
	void onChildSized (View* child, const Point& delta) override;
	void calcSizeLimits () override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;
	void attached (View* parent) override;

	StringRef getHelpIdentifier () const override;

	enum AlignViewStyles
	{
		kChildSizable = 1<<0,		///< child is sizable on the edge
		kPassContextMenu = 1<<1		///< allow other context menu handler to constribute to menu
	};

	DECLARE_STYLEDEF (customStyles)

private:
	void updateStyle ();
	template<class Direction> void calcSizeLimits ();
	void doLayout ();
	IAttributeList* getViewState (tbool create);
	bool setAlignment (CmdArgs, VariantRef data);
	void resizeChild (Coord extend);

	class ChildResizeHandler;

	bool getDividerRect (Rect& rect, bool& isStartDivider, int outreachPoints = 0);

	Coord storedChildSize;
	SharedPtr<IImage> dividerStartImage;
	SharedPtr<IImage> dividerEndImage;
	Coord dividerSize;
	Coord dividerOffset;
	Coord dividerOutreach;
};

} // namespace CCL

#endif // _ccl_alignview_h
