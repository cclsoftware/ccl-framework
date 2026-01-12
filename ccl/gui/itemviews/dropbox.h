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
// Filename    : ccl/gui/itemviews/dropbox.h
// Description : Drop Box
//
//************************************************************************************************

#ifndef _ccl_dropbox_h
#define _ccl_dropbox_h

#include "ccl/gui/itemviews/itemviewbase.h"
#include "ccl/public/gui/framework/idropbox.h"
#include "ccl/public/gui/framework/idragndrop.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

class BoxLayoutView;

//************************************************************************************************
// DropBox
//************************************************************************************************

class DropBox: public ItemViewBase,
			   public IDropBox
{
public:
	DECLARE_CLASS (DropBox, ItemViewBase)

	DropBox (const Rect& size = Rect (), StyleRef style = 0);
	~DropBox ();

	DECLARE_STYLEDEF (customStyles)

	// IDropBox
	IView* CCL_API getViewItem (ItemIndexRef index) override;

	// ItemViewBase
	void setStyle (StyleRef style) override;
	bool onMouseDown (const MouseEvent& event) override;
    bool onGesture (const GestureEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IUnknown* CCL_API getController () const override;

	CLASS_INTERFACE (IDropBox, ItemViewBase)

	void resetClientLimits ();

protected:
	BoxLayoutView* clientView;
	ObjectArray itemViews;
	Coord snap;
	Coord spacing;
	Coord freeSpace;

	void recalcSnap ();
	void adjustViewSize (Rect& size) const;
    bool dragItem (ItemIndexRef index, int dragDevice = IDragSession::kMouseInput);
	void notifyRemove (ItemIndexRef index);
	View* getViewByObject (IUnknown* object) const;
	View* createView (IUnknown* object) const;
	
	// ItemViewBase
	void modelChanged (int changeType, ItemIndexRef item) override;
	void onSize (const Point& delta) override;
	void onChildSized (View* child, const Point& delta) override;
	tbool CCL_API findItem (ItemIndex& index, const Point& where) const override;
	void CCL_API getItemRect (Rect& rect, ItemIndexRef index, int column = -1) const override;
	void getSizeInfo (SizeInfo& info) override;
	void onVisualStyleChanged () override;
	void calcSizeLimits () override;

	class DeleteItemDragHandler;
};

//************************************************************************************************
// DropBoxControl
/** A specialized scrollable view that manages a dynamic list of views.
The controller must provide a special "item model" as object that is referenced with the DropBox name. 
For each of the items in the model, a View is created, with the item as controller and a Form from the skin. 
The form name for the items is the DropBox name appended with "Item". */
//************************************************************************************************

class DropBoxControl: public ItemControlBase
{
public:
	DECLARE_CLASS (DropBoxControl, ItemControlBase)

	DropBoxControl (const Rect& size = Rect (), 
				 StyleRef dropBoxStyle = 0,
				 StyleRef scrollViewStyle = Styles::kTransparent);

	~DropBoxControl ();
	
	Attributes& getDropBoxArguments () { return dropBoxArguments; }
	
	// View
	void attached (View* parent) override;
	void calcSizeLimits () override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	void onSize (PointRef delta) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	DECLARE_STRINGID_MEMBER (kResizeDropBox)
	
	Attributes dropBoxArguments;
	void resizeDropBox ();
	int countItems () const;
	Point getItemSize () const;
	Coord getItemSpacing () const;
	void updateScrollViewSize (Rect& size, int itemCount, Coord itemSpacing) const;
	Coord getMinItemWidth (Coord itemWidth) const;
	
	bool shouldAutoResize;
	mutable int lastColumnCount;
};

DECLARE_VISUALSTYLE_CLASS (DropBoxStyle)
	
} // namespace CCL

#endif // _ccl_dropbox_h
