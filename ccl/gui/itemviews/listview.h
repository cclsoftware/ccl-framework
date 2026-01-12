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
// Filename    : ccl/gui/itemviews/listview.h
// Description : List View
//
//************************************************************************************************

#ifndef _ccl_listview_h
#define _ccl_listview_h

#include "ccl/gui/itemviews/itemview.h"

#include "ccl/gui/views/scrollview.h"

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

//************************************************************************************************
// ListStyle
//************************************************************************************************

class ListStyle: public ItemStyle
{
public:
	DECLARE_CLASS (ListStyle, ItemStyle)

	ListStyle ();

	PROPERTY_BOOL (multiLineTitle, MultiLineTitle)
	PROPERTY_BOOL (iconsMirrored, IconsMirrored)
	PROPERTY_BOOL (thumbnailsAsIcons, ThumbnailsAsIcons)
	PROPERTY_BOOL (iconSetHalfSize, IconSetHalfSize)
	PROPERTY_VARIABLE (int, fixedColumns, FixedColumns)
	PROPERTY_VARIABLE (int, listPaddingBottom, ListPaddingBottom)
	PROPERTY_VARIABLE (int, listTextMargin, ListTextMargin)
	PROPERTY_VARIABLE (Alignment, listTextAlignment, ListTextAlignment)
	
	const Point& getItemSize (int viewType) const;
	const Rect& getDataRect (int viewType) const;
	const Rect& getTextRect (int viewType) const;

	void setItemSize (int viewType, PointRef size);
	void setDataRect (int viewType, RectRef rect);
	void setTextRect (int viewType, RectRef rect);

	PROPERTY_SHARED_AUTO (IImage, iconBackgroundImage, IconBackgroundImage)
	PROPERTY_SHARED_AUTO (IImage, iconOverlayImage, IconOverlayImage)
	PROPERTY_SHARED_AUTO (IImage, iconOverlayFolderImage, IconOverlayFolderImage)
	PROPERTY_SHARED_AUTO (IImage, iconFocusImage, IconFocusImage)
	PROPERTY_SHARED_AUTO (IImage, selectionBackground, SelectionBackground)
	PROPERTY_SHARED_AUTO (IImage, listItemBackground, ListItemBackground)
	PROPERTY_OBJECT (Color, iconSelectedIconColor, IconSelectedIconColor)
	PROPERTY_OBJECT (Font, iconFont, IconFont)
	
	// ItemStyle
	void updateStyle (const VisualStyle& style) override;
	void zoom (const ItemStyle& original, float zoomFactor) override;

protected:
	struct ItemSize
	{
		Point totalSize;	///< bounding box
		Rect dataRect;		///< icon
		Rect textRect;		///< text
	};

	ItemSize itemSize[Styles::kNumListViewTypes];
};

DECLARE_VISUALSTYLE_CLASS (ListStyle)

//************************************************************************************************
// ListView
//************************************************************************************************

class ListView: public ItemView,
				public IListView
{
public:
	DECLARE_CLASS (ListView, ItemView)

	ListView (const Rect& size = Rect (), IItemModel* model = nullptr, StyleRef style = 0);

	DECLARE_STYLEDEF (customStyles)
	DECLARE_STYLEDEF (viewTypeNames)

	ListStyle& getListStyle () const;

	Styles::ListViewType getViewType () const;
	void CCL_API setViewType (Styles::ListViewType which) override; ///< IListView

	void CCL_API setTextTrimMode (int trimMode) override; ///< IListView

	// ItemView
	Point getBackgroundOffset () const override;
	tbool CCL_API getFocusItem (ItemIndex& index) const override;
	tbool CCL_API setFocusItem (ItemIndexRef index, tbool selectExclusive = true) override;
	tbool CCL_API selectAll (tbool state) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API invalidateItem (ItemIndexRef index) override;
	tbool CCL_API findItems (const Rect& rect, IItemSelection& items) const override;
	tbool CCL_API findItemCell (ItemIndex& row, int& column, const Point& where) const override;
	void CCL_API getItemRect (Rect& rect, ItemIndexRef index, int column = -1) const override;
	void modelChanged (int changeType, ItemIndexRef item) override;
	bool onEditDelete (const CommandMsg& args) override;
	AccessibilityProvider* getAccessibilityProvider () override;

	// View
	void onSize (const Point& delta) override;
	void draw (const UpdateRgn& updateRgn) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	IUnknown* CCL_API getController () const override;

	CLASS_INTERFACE (IListView, ItemView)

protected:
	struct ItemInfo
	{
		int row;
		int column;
		int index;
		Rect rect;

		ItemInfo ()
		: row (0),
		  column (0),
		  index (0)
		{}
	};

	class RowIterator;
	friend class RowIterator;

	Styles::ListViewType viewType;
	int textTrimMode;
	int anchorIndex; ///< the starting item for a range selection
	ItemInfo focusItem; ///< the item that has keyboard focus (rect not used)
	Vector<Coord> itemBottoms;
	
	int getItemWidth () const;
	int getDefaultItemWidth () const;
	int getItemHeight (ItemIndexRef index) const override;
	int getItemRow (ItemIndexRef index) const override;
	int countItems () const;
	int countRows () const;
	int countColumns () const;
	void autoCenterItemRect (Rect& rect);
	
	void drawListMatrix (GraphicsPort& port, const UpdateRgn& updateRgn, FontRef font);
	void drawCell (GraphicsPort& port, const Rect& rect, int row, int column, int state, FontRef font, BrushRef textBrush);
	void drawItems (GraphicsPort& port, const UpdateRgn& updateRgn, const Font& font);
	void drawItem (GraphicsPort& port, const Rect& rect, int index, const Font& font);

	bool findCell (const Point& where, int& row, int& column, Rect& rect) const;
	bool getItemInfo (ItemInfo& info, const Point& where) const;
	bool getCellRect (Rect& size, int row, int column) const;
	void getRowRect (Rect& rect, ItemIndexRef index) const;
	bool measureCellContent (Rect& size, int row, int column) const;
	int getCellIndex (int row, int col, bool strict) const;
	int getIndexFromColumn (int _column) const;
	void getCellAddress (int& row, int& col, int index) const;
	bool getCellAddress (int& row, int& column, PointRef where) const;
	void setFocusCell (int row, int column);
	bool onTap (const GestureEvent& event);

	// ItemView
	bool getAnchorItem (ItemIndex& index) const override;
	bool setAnchorItem (ItemIndexRef index) override;
	bool selectRange (ItemIndexRef fromIndex, ItemIndexRef toIndex) override;
	bool navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly) override;
	void getSizeInfo (SizeInfo& info) override;
	int getColumnIndex (PointRef where) override;
	int getLogicalColumnIndex (PointRef where) override;
	ColumnHeaderList* getVisibleColumnList () const override;
	bool getNextItem (ItemIndex& item, bool forNavigation = true) override;
	bool getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn) override;
	Font& getFont (Font& font) const override;
};

//************************************************************************************************
// ListControl
/** A specialized scrollable view that manages a dynamic list of items with icons & text. Scrollable list view.
The controller must provide a special "item model" as object that is referenced with the ListView name.
The ListView has the 3 different view types that define the basic layout of items. */
//************************************************************************************************

class ListControl: public ItemControl
{
public:
	DECLARE_CLASS (ListControl, ItemControl)

	ListControl (const Rect& size = Rect (), 
				 IItemModel* model = nullptr,
				 StyleRef listViewStyle = 0,
				 StyleRef scrollViewStyle = 0);

	// IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
};

} // namespace CCL

#endif // _ccl_listview_h
