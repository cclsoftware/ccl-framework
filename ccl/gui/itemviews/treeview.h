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
// Filename    : ccl/gui/itemviews/treeview.h
// Description : Tree View
//
//************************************************************************************************

#ifndef _ccl_treeview_h
#define _ccl_treeview_h

#include "ccl/gui/itemviews/itemview.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

class Tree;
class TreeItem;

//************************************************************************************************
// TreeStyle
//************************************************************************************************

class TreeStyle: public ItemStyle
{
public:
	DECLARE_CLASS (TreeStyle, ItemStyle)

	TreeStyle ();
	TreeStyle (const TreeStyle&);

	PROPERTY_VARIABLE (int, itemInset, ItemInset)
	PROPERTY_VARIABLE (int, leafInset, LeafInset) ///< inset for leaves (non-expandable), by default the same as expandSize
	PROPERTY_OBJECT (Point, iconSize, IconSize)
	PROPERTY_OBJECT (Point, expandSize, ExpandSize)
	PROPERTY_OBJECT (SolidBrush, selectOverlayBrush, SelectOverlayBrush)

	// ItemStyle
	void updateStyle (const VisualStyle& style) override;
	void zoom (const ItemStyle& original, float zoomFactor) override;
};

DECLARE_VISUALSTYLE_CLASS (TreeStyle)

//************************************************************************************************
// TreeView
//************************************************************************************************

class TreeView: public ItemView,
				public ITreeView,
				public IdleClient
{
public:
	DECLARE_CLASS (TreeView, ItemView)

	TreeView (const Rect& size = Rect (), IItemModel* model = nullptr, StyleRef style = 0);
	~TreeView ();

	DECLARE_STYLEDEF (customStyles)

	Tree& getTree () const;
	TreeStyle& getTreeStyle () const;

	void setFocusTreeItem (TreeItem* item);
	void invalidateTreeItem (TreeItem* item);

	TreeItem* findTreeItem (const Point& where) const;
	TreeItem* getFirstTreeItem ();
	void determineIconSize (Point& iconSize, ItemIndexRef itemIndex);

	// ITreeView
	ITreeItem* CCL_API getRootItem () override;
	void CCL_API expandItem (ITreeItem* item, tbool state = true, int expandMode = 0) override;
	void CCL_API refreshItem (ITreeItem* item) override;
	void CCL_API setItemFilter (IObjectFilter* filter) override;
	void CCL_API setTree (ITree* tree) override;
	void CCL_API updateThumbnails () override;
	int CCL_API getItemTextInset (ITreeItem* item) override;

	// ItemView
	void CCL_API setModel (IItemModel* model) override;
	tbool CCL_API getFocusItem (ItemIndex& index) const override;
	tbool CCL_API setFocusItem (ItemIndexRef index, tbool selectExclusive = true) override;
	tbool CCL_API selectItem (ItemIndexRef index, tbool state) override;
	tbool CCL_API selectAll (tbool state) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API invalidateItem (ItemIndexRef index) override;
	void CCL_API makeItemVisible (ItemIndexRef index) override;
	tbool CCL_API findItems (const Rect& rect, IItemSelection& items) const override;
	void onVisualStyleChanged () override;
	void setStyle (StyleRef style) override;
	bool getAnchorItem (ItemIndex& index) const override;
	bool setAnchorItem (ItemIndexRef index) override;
	bool openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect = Rect ()) override;
	bool getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn) override;
	void draw (const UpdateRgn& updateRgn) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	void onDragOverItem (const DragEvent& event, ItemIndexRef index) override;
	bool onDragLeave (const DragEvent& event) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	void attached (View* parent) override;
	void modelChanged (int changeType, ItemIndexRef item) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool onGesture (const GestureEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;

	// IdleClient
	void onIdleTimer () override;

	CLASS_INTERFACE (ITreeView, ItemView)

protected:
	Tree* tree;
	TreeItem* focusItem;
	TreeItem* anchorItem;
	TreeItem* dragOverItem;
	bool ownTree;
	bool avoidScrolling;	///< avoid scrolling in getSizeInfo by adding the required extraHeight
	Coord extraHeight;		///< temporary extra height to avoid slipping away of a collapsed item
	int editColumn;
	DECLARE_STRINGID_MEMBER (kUpdateSize)

	struct TDrawState
	{
		TDrawState (GraphicsPort& port, 
					const UpdateRgn& updateRgn,
					const Font& font,
					const SolidBrush& textBrush)
		: port (port),
		  updateRgn (updateRgn),
		  font (font),
		  textBrush (textBrush)
		{}

		GraphicsPort& port;
		const UpdateRgn& updateRgn;
		Point indent;
		Font font;
		SolidBrush textBrush;
	};

	void setRootItem (IUnknown* data);
	void verifyAnchorItem ();
	void expandItemChecked (TreeItem& item, bool deep);
	void expandItem (TreeItem& item, bool state = true, int expandMode = 0);
	void selectRange (TreeItem* item1, TreeItem* item2);
	TreeItem* skipItems (TreeItem* startItem, int rows, bool onlySelectable);

	Image* getIcon (TreeItem* item);
	Image* getThumbnail (TreeItem* item);
	ITextLayout* getTextLayout (TreeItem* item);
	Coord getStringWidth (TreeItem* item);
	int getItemIndex (const Point& where) const;
	bool getItemRect (Rect&, TreeItem* item) const;
	void getActiveRect (Rect& activeRect, RectRef itemRect, TreeItem* item);
	bool getColumnRange (Coord& left, Coord& right, int& columnIndex, Coord x);
	bool isExpandHit (RectRef columnRect, TreeItem* item, PointRef where);

	bool drawItem (TreeItem* item, TDrawState& state);
	bool drawSubItems (TreeItem* item, TDrawState& state);
	
	void drawItemBackground (TreeItem* item, TDrawState& state, RectRef itemRect);
	void drawItemContent (GraphicsPort& port, RectRef itemRect, RectRef cellRect, TreeItem* item, 
						  FontRef font, BrushRef textBrush, const UpdateRgn& updateRgn); 
	void drawExpandButton (GraphicsPort& port, const Rect& rect, bool drawSelected, bool expanded);
	bool onTap (const GestureEvent& event);

	// ItemView
	void CCL_API setZoomFactor (float factor) override;
	void CCL_API getItemRect (Rect& rect, ItemIndexRef index, int column = -1) const override;
	bool selectRange (ItemIndexRef fromIndex, ItemIndexRef toIndex) override;
	bool navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly) override;
	int getColumnIndex (PointRef where) override;
	void getSizeInfo (SizeInfo& info) override;
	int getItemHeight (ItemIndexRef index) const override;
	int getItemRow (ItemIndexRef index) const override;
	int getStandardStyleIndex () const override;
	bool getNextItem (ItemIndex& item, bool forNavigation = true) override;

	friend struct TreeGetTotalSize;
};

//************************************************************************************************
// TreeControl
/** A specialized scrollable view that manages a dynamic tree structure of items with icons & text. */
//************************************************************************************************

class TreeControl: public ItemControl
{
public:
	DECLARE_CLASS (TreeControl, ItemControl)

	TreeControl (const Rect& size = Rect (), 
				 IItemModel* model = nullptr,
				 StyleRef treeViewStyle = 0,
				 StyleRef scrollViewStyle = 0);
};

} // namespace CCL

#endif // _ccl_treeview_h
