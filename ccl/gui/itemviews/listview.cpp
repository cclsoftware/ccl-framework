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
// Filename    : ccl/gui/itemviews/listview.cpp
// Description : List View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/itemviews/listview.h"
#include "ccl/gui/itemviews/headerview.h"
#include "ccl/gui/itemviews/itemviewaccessibility.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/framework/imenu.h"

using namespace CCL;

//************************************************************************************************
// ListStyle
/** Style attributes for a ListView. */
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListStyle, ItemStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (ListStyle, VisualStyle, "ListViewStyle")
	// icons
	ADD_VISUALSTYLE_METRIC ("icons.width")					///< for "viewtype="icons": total width of one item
	ADD_VISUALSTYLE_METRIC ("icons.height")					///< for "viewtype="icons": total height of one item
	ADD_VISUALSTYLE_METRIC ("icons.dataleft")				///< for "viewtype="icons": left of icon inside item rectangle
	ADD_VISUALSTYLE_METRIC ("icons.datatop")				///< for "viewtype="icons": top of icon inside item rectangle
	ADD_VISUALSTYLE_METRIC ("icons.datawidth")				///< for "viewtype="icons": width of icon
	ADD_VISUALSTYLE_METRIC ("icons.dataheight")				///< for "viewtype="icons": height of icon
	ADD_VISUALSTYLE_METRIC ("icons.textleft")				///< for "viewtype="icons": left of title inside item rectangle
	ADD_VISUALSTYLE_METRIC ("icons.texttop")				///< for "viewtype="icons": top of title inside item rectangle
	ADD_VISUALSTYLE_METRIC ("icons.textwidth")				///< for "viewtype="icons": width of title
	ADD_VISUALSTYLE_METRIC ("icons.textheight")				///< for "viewtype="icons": height of title
	ADD_VISUALSTYLE_METRIC ("icons.multiLineTitle")			///< for "viewtype="icons": draw title as multiline text
	ADD_VISUALSTYLE_METRIC ("icons.mirrored")				///< for "viewtype="icons": draw icon columns in reverse order
	ADD_VISUALSTYLE_METRIC ("icons.showthumbnails")			///< for "viewtype="icons": draw thumbnails (if available) instead of icons, 
	ADD_VISUALSTYLE_IMAGE  ("icons.background")				///< for "viewtype="icons": draw background for each cell (use second frame for selection)
	ADD_VISUALSTYLE_FONT   ("icons.textfont")				///< for "viewtype="icons": optional alternative font in icons mode (falls back to "textfont" if not specified)
	ADD_VISUALSTYLE_METRIC ("icons.iconsethalfsize")		///< for "viewtype="icons": alternative way of displaying iconset images in half-size
	ADD_VISUALSTYLE_METRIC ("icons.fixedColumns")			///< for "viewtype="icons": use a fixed number of columns
	// list
	ADD_VISUALSTYLE_METRIC ("list.dataleft")				///< for "viewtype="list": left of list inside item rectangle
	ADD_VISUALSTYLE_METRIC ("list.datatop")					///< for "viewtype="list": top of list inside item rectangle
	ADD_VISUALSTYLE_METRIC ("list.datawidth")				///< for "viewtype="list": width of list
	ADD_VISUALSTYLE_METRIC ("list.dataheight")				///< for "viewtype="list": height of list
	ADD_VISUALSTYLE_METRIC ("list.padding.bottom")			///< for "viewtype="list": bottom padding of list
	ADD_VISUALSTYLE_METRIC ("list.textmargin")				///< for "viewtype="list": left text margin in cell  
	ADD_VISUALSTYLE_IMAGE  ("list.selectionbackground")		///< for "viewtype="list": draw background for a selected item as an image
	ADD_VISUALSTYLE_IMAGE  ("list.itembackground")			///< for "viewtype="list": draw background for every unselected item in the list as an image
	ADD_VISUALSTYLE_ALIGN  ("list.textalign")				///< for "viewtype="list": set text alignment for cells (default: kLeftCenter)
END_VISUALSTYLE_CLASS (ListStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListStyle::ListStyle ()
: multiLineTitle (false),
  iconsMirrored (false),
  thumbnailsAsIcons (false),
  iconSetHalfSize (false),
  iconBackgroundImage (nullptr),
  iconOverlayImage (nullptr),
  iconOverlayFolderImage (nullptr),
  iconFocusImage (nullptr),
  fixedColumns (0),
  listPaddingBottom (0),
  listTextMargin (0),
  selectionBackground (nullptr),
  listItemBackground (nullptr),
  listTextAlignment (Alignment::kLeftCenter)
{
	itemSize[Styles::kListViewIcons].totalSize (76, 76);
	itemSize[Styles::kListViewIcons].dataRect (10, 12, 66, 47);
	itemSize[Styles::kListViewIcons].textRect (2, 48, 74, 74);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& ListStyle::getItemSize (int viewType) const
{ 
	return itemSize[viewType].totalSize; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Rect& ListStyle::getDataRect (int viewType) const
{ 
	return itemSize[viewType].dataRect; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Rect& ListStyle::getTextRect (int viewType) const 
{ 
	return itemSize[viewType].textRect; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListStyle::setItemSize (int viewType, PointRef size)
{
	itemSize[viewType].totalSize = size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListStyle::setDataRect (int viewType, RectRef rect)
{
	itemSize[viewType].dataRect = rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListStyle::setTextRect (int viewType, RectRef rect)
{
	itemSize[viewType].textRect = rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListStyle::updateStyle (const VisualStyle& style)
{
	ItemStyle::updateStyle (style);

	ItemSize& iconSize = itemSize[Styles::kListViewIcons];
	
	iconSize.totalSize.x = style.getMetric ("icons.width", iconSize.totalSize.x);
	iconSize.totalSize.y = style.getMetric ("icons.height", iconSize.totalSize.y);

	iconSize.dataRect.left = style.getMetric ("icons.dataleft", iconSize.dataRect.left);
	iconSize.dataRect.top = style.getMetric ("icons.datatop", iconSize.dataRect.top);
	iconSize.dataRect.setWidth (style.getMetric ("icons.datawidth", iconSize.dataRect.getWidth ()));
	iconSize.dataRect.setHeight (style.getMetric ("icons.dataheight", iconSize.dataRect.getHeight ()));

	iconSize.textRect.left = style.getMetric ("icons.textleft", iconSize.textRect.left);
	iconSize.textRect.top = style.getMetric ("icons.texttop", iconSize.textRect.top);
	iconSize.textRect.setWidth (style.getMetric ("icons.textwidth", iconSize.textRect.getWidth ()));
	iconSize.textRect.setHeight (style.getMetric ("icons.textheight", iconSize.textRect.getHeight ()));

	// icon size in list mode; default: empty (no icon)
	ItemSize& listSize = itemSize[Styles::kListViewList];
	listSize.dataRect.left = style.getMetric ("list.dataleft", listSize.dataRect.left);
	listSize.dataRect.top = style.getMetric ("list.datatop", listSize.dataRect.top);
	listSize.dataRect.setWidth (style.getMetric ("list.datawidth", listSize.dataRect.getWidth ()));
	listSize.dataRect.setHeight (style.getMetric ("list.dataheight", listSize.dataRect.getHeight ()));
	
	iconsMirrored = style.getMetric<bool> ("icons.mirrored", false);
	thumbnailsAsIcons = style.getMetric<bool> ("icons.showthumbnails", thumbnailsAsIcons);
	iconSetHalfSize = style.getMetric<bool> ("icons.iconsethalfsize", iconSetHalfSize);
	multiLineTitle = style.getMetric<bool> ("icons.multiLineTitle", 0);
	fixedColumns = style.getMetric ("icons.fixedColumns", fixedColumns);

	listPaddingBottom = style.getMetric ("list.padding.bottom", 0);
	listTextMargin = style.getMetric ("list.textmargin", 0);
	
	setIconSelectedIconColor (style.getColor ("icons.selectediconcolor", getSelectedIconColor ()));
	setIconBackgroundImage (style.getImage ("icons.background"));
	setIconOverlayImage (style.getImage ("icons.overlay"));
	setIconOverlayFolderImage (style.getImage ("icons.overlayfolder"));
	setIconFocusImage (style.getImage ("icons.focusframe"));
	setSelectionBackground (style.getImage ("list.selectionBackground"));
	setListItemBackground (style.getImage ("list.itemBackground"));
	setListTextAlignment (style.getOptions ("list.textalign", listTextAlignment.align));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListStyle::zoom (const ItemStyle& _original, float zoomFactor)
{
	ItemStyle::zoom (_original, zoomFactor);

	const ListStyle* original = ccl_cast<ListStyle> (&_original);
	ASSERT (original != nullptr)
	if(!original)
		return;

	// TODO: to be tested...
	Transform t;
	t.scale (zoomFactor, zoomFactor);
	for(int type = 0; type < Styles::kNumListViewTypes; type++)
	{
		Point totalSize (original->itemSize[type].totalSize);
		itemSize[type].totalSize = t.transform (totalSize);

		Rect dataRect (original->itemSize[type].dataRect);
		itemSize[type].dataRect = t.transform (dataRect);

		Rect textRect (original->itemSize[type].textRect);
		itemSize[type].textRect = t.transform (textRect);
	}
}

//************************************************************************************************
// ListControl
//************************************************************************************************

DEFINE_CLASS (ListControl, ItemControl)
DEFINE_CLASS_UID (ListControl, 0x5f53609a, 0xfbca, 0x4ccd, 0xb0, 0xb9, 0x9a, 0x5d, 0xee, 0xf4, 0x14, 0x60)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListControl::ListControl (const Rect& size, IItemModel* model, StyleRef listViewStyle, StyleRef scrollViewStyle)
: ItemControl (size,
			   NEW ListView (Rect (), model, listViewStyle),
			   scrollViewStyle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ListControl::queryInterface (UIDRef iid, void** ptr)
{
	// delegate to ListView
	if(iid == ccl_iid<IListView> () && getItemView ())
		return getItemView ()->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//************************************************************************************************
// ListView::RowIterator
//************************************************************************************************

class ListView::RowIterator
{
public:
	int row;		///< current row
	Coord top;		///< top of current row
	Coord bottom;	///< bottom of current row

	RowIterator (const ListView& view, Coord startCoord, Coord endCoord);
	bool nextRow ();

private:
	const Vector<Coord>& itemBottoms;
	Coord centerMargin;
	Coord startCoord;
	Coord endCoord;
	int numRows;
};

//************************************************************************************************
// ListView::RowIterator
//************************************************************************************************

inline ListView::RowIterator::RowIterator (const ListView& view, Coord startCoord, Coord endCoord)
: numRows (view.itemBottoms.count ()),
  row (-1),
  top (0),
  bottom (0),
  endCoord (endCoord),
  startCoord (startCoord),
  centerMargin (0),
  itemBottoms (view.itemBottoms)
{
	if(numRows > 0)
	{
		if(view.getStyle ().isCustomStyle (Styles::kListViewAppearanceCenterRows))
			bottom = top = centerMargin = (view.getHeight () - itemBottoms.last ()) / 2;
		
		// skip rows until next row is in requested range
		while(row + 1 < numRows - 1 && (itemBottoms[row + 1] + centerMargin) < startCoord)
		{
			row++;
			bottom = itemBottoms[row] + centerMargin;
		}
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ListView::RowIterator::nextRow ()
{
	// next row must exist and not start after end coord
	if(row >= numRows - 1 || bottom > endCoord)
		return false;

	row++;
	top = bottom;
	bottom = itemBottoms[row] + centerMargin;
	
	// it is the last row and it is above the area start
	if(row >= numRows - 1 && bottom < startCoord)
		return false;
	
	return true;
}

//************************************************************************************************
// ListView
//************************************************************************************************

BEGIN_STYLEDEF (ListView::customStyles)
	{"noclipcells",		Styles::kListViewAppearanceDontClipCells},
	{"nolinebreak",		Styles::kListViewAppearanceNoLineBreak},
	{"extendlastcolumn", Styles::kListViewAppearanceExtendLastColumn},
	{"autosizeitems", 	Styles::kListViewAppearanceAutoSizeItems},
	{"nodefaulticon", 	Styles::kListViewAppearanceNoDefaultIcon},
	{"autocentericons",	Styles::kListViewAppearanceAutoCenterIcons},
	{"drawcustomitem",	Styles::kListViewAppearanceDrawCustomItem},
	{"navigateflat",	Styles::kListViewBehaviorNavigateFlat},
	{"centerrows",		Styles::kListViewAppearanceCenterRows},
END_STYLEDEF

BEGIN_STYLEDEF (ListView::viewTypeNames)
	{"list",	Styles::kListViewList},
	{"details",	Styles::kListViewDetails},
	{"icons",	Styles::kListViewIcons},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ListView, ItemView)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListView::ListView (const Rect& size, IItemModel* model, StyleRef style)
: ItemView (size, style),
  viewType (Styles::kListViewList),
  textTrimMode (Font::kTrimModeDefault),
  anchorIndex (0)
{
	setItemStyle (NEW ListStyle);
	setModel (model);
	if(style.isCustomStyle (Styles::kListViewAppearanceAutoSizeItems|Styles::kListViewAppearanceAutoCenterIcons))
		setSizeMode (kAttachLeft|kAttachRight);

	if(style.isHorizontal () == false && style.isVertical () == false)
		this->style.setCommonStyle (Styles::kVertical);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ListView::getController () const
{
	if(!controller)
		controller = (ICommandHandler*)NEW ItemViewController (const_cast<ListView*>(this));
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListStyle& ListView::getListStyle () const
{
	return (ListStyle&)getItemStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Styles::ListViewType ListView::getViewType () const	
{ 
	return viewType; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListView::setViewType (Styles::ListViewType which)
{
	if(viewType != which)
	{
		viewType = which;
		
		updateSize ();
		if(isAttached ())
			invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListView::setTextTrimMode (int trimMode)
{
	textTrimMode = trimMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeaderList* ListView::getVisibleColumnList () const
{
	// ignore column list in icons mode
	return viewType < Styles::kListViewIcons ? columnList : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::invalidateItem (ItemIndexRef index)
{
	if(index.getObject ())
		invalidate ();
	else
	{
		Rect rect;
		getItemRect (rect, index);
		invalidate (rect);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	// invalidate last column if it resizes with the view
	if(delta.x != 0 && columnList && style.isCustomStyle (Styles::kListViewAppearanceExtendLastColumn))
	{
		Rect rect;
		getClientRect (rect);
		Coord dummy;
		columnList->getColumnRange (rect.left, dummy, columnList->getCount (true) - 1);
		invalidate (rect);
	}

	updateSize ();
	
	auto numberOfIconColumnsChanged = [&]()
	{
		if(viewType == Styles::kListViewIcons && !style.isCustomStyle (Styles::kItemViewAppearanceRedrawOnResize) && !style.isCustomStyle (Styles::kListViewAppearanceNoLineBreak))
		{
			int colW = getItemWidth ();
			if(colW != 0)
			{
				int newColumns = getWidth () / colW;
				int oldColumns = (getWidth () - delta.x) / colW;
				if(newColumns != oldColumns )
					return true;
			}
		}
		
		return false;
	};
	
	if(numberOfIconColumnsChanged () || style.isCustomStyle (Styles::kListViewAppearanceAutoSizeItems|Styles::kListViewAppearanceAutoCenterIcons))
		updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getItemWidth () const
{
	int itemWidth = getDefaultItemWidth ();
	
	if(style.isCustomStyle (Styles::kListViewAppearanceAutoSizeItems|Styles::kListViewAppearanceAutoCenterIcons))
	{
		int columnCount = countColumns ();
		
		int remainder = getWidth () - (columnCount * itemWidth);
		
		itemWidth += remainder / columnCount;
	}
	
	return itemWidth;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getDefaultItemWidth () const
{
	ListStyle& listStyle = getListStyle ();
	if(viewType >= Styles::kListViewIcons)
		return listStyle.getItemSize (viewType).x + 2 * listStyle.getMarginH ();
	return getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getItemHeight (ItemIndexRef index) const
{
	ListStyle& listStyle = getListStyle ();
	if(viewType >= Styles::kListViewIcons)
		return listStyle.getItemSize (viewType).y + 2 * listStyle.getMarginV ();
	else if(index.isValid ())
	{
		int i = index.getIndex ();
		if(i < itemBottoms.count ())
			return i > 0 ? itemBottoms[i] - itemBottoms[i - 1] : itemBottoms[0];
	}
	return listStyle.getRowHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::getRowRect (Rect& rect, ItemIndexRef index) const
{
	if(index.isValid ())
	{
		int i = -1;
		if(index.getIndex (i))
		{
			Coord centerMargin = 0;
			if(getStyle ().isCustomStyle (Styles::kListViewAppearanceCenterRows))
				centerMargin = (getHeight () - itemBottoms.last ()) / 2;
			
			if(i < itemBottoms.count ())
				rect (0, i > 0 ? itemBottoms[i - 1] : centerMargin, getWidth (), itemBottoms[i] + centerMargin);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getItemRow (ItemIndexRef index) const
{
	return index.getIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::countItems () const
{
	return model ? model->countFlatItems () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::countRows () const
{
	int numColumns = countColumns ();
	int numItems = countItems ();
	int numRows = numItems / numColumns;
	if(numItems % numColumns)
		numRows++;
	return numRows;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::countColumns () const
{
	int numColumns = 1;
	if(viewType >= Styles::kListViewIcons)
	{
		if(style.isCustomStyle (Styles::kListViewAppearanceNoLineBreak))
			numColumns = countItems ();
		else if(int fixedColumnCount = getListStyle ().getFixedColumns ())
			numColumns = fixedColumnCount;
		else
			numColumns = getWidth () / getDefaultItemWidth ();
	}
	if(numColumns < 1)
		numColumns = 1;
	return numColumns;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getCellIndex (int row, int col, bool strict) const
{
	int index = -1;
	if(viewType >= Styles::kListViewIcons)
	{
		if(style.isCustomStyle (Styles::kListViewAppearanceNoLineBreak))
			index = getIndexFromColumn (col);
		else
			index = row * countColumns () + getIndexFromColumn (col);
	}
	else
		index = row;

	if(strict)
	{
		if(index >= countItems () || index < 0)
			index = -1;
	}
	else
		index = ccl_bound (index, 0, countItems () - 1);
	return index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getIndexFromColumn (int _column) const
{
	if(getListStyle ().isIconsMirrored ())
		return countColumns () - 1 - _column;
	
	return _column;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::getCellAddress (int& row, int& column, int index) const
{
	if(viewType >= Styles::kListViewIcons)
	{
		int numCols = countColumns ();
		row    = index / numCols;
		column = index % numCols;
		
		if(getListStyle ().isIconsMirrored ())
			column = numCols - 1 - (index % numCols);
		else
			column = index % numCols;
	}
	else
	{
		row = index;
		column = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getCellAddress (int& row, int& column, PointRef where) const
{
	int c = 0;
	int r = 0;
	if(viewType >= Styles::kListViewIcons)
	{
		int itemW = ccl_max (1, getItemWidth ());
		int itemH = ccl_max (1, getItemHeight (ItemIndex ()));

		c = where.x / itemW;
		r = where.y / itemH;
	}
	else
	{
		// todo
	}

	column = ccl_bound (c, 0, countColumns () - 1);
	row    = ccl_bound (r, 0, countRows ()    - 1);
	return column == c && row == r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListView::getItemRect (Rect& rect, ItemIndexRef index, int column) const
{
	if(column != -1)
	{
		getCellRect (rect, index.getIndex (), column);
		return;
	}

	if(viewType >= Styles::kListViewIcons)
	{
		int rowH = getItemHeight (index);
		int colW = getItemWidth ();
		int row, col;
		getCellAddress (row, col, index.getIndex ());
		rect (0, 0, colW, rowH);
		rect.offset (col * colW, row * rowH);
	}
	else
		getRowRect (rect, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::getSizeInfo (SizeInfo& info)
{	
	int numItems = countItems ();
	int numColumns = countColumns ();
	int numRows = numItems / numColumns;
	if(numItems % numColumns)
		numRows++;

	Coord w = 0;
	Coord h = 0;
	
	if(viewType >= Styles::kListViewIcons)
	{
		h = numRows * getItemHeight (ItemIndex ());

		if(style.isCustomStyle (Styles::kListViewAppearanceNoLineBreak))
			w = numColumns * getItemWidth ();
		else
			w = getItemWidth ();

		// recalc focus row / column
		if(focusItem.index >= 0)
			getCellAddress (focusItem.row, focusItem.column, focusItem.index);
	}
	else
	{
		if(columnList)
		{
			w = columnList->getTotalWidth ();
		}
		else
		{
			Font font;
			getFont (font);

			int num = ccl_min (numItems, 200);
			for(int i = 0; i < num; i++)
			{
				String title;
				model->getItemTitle (title, ItemIndex (i));
				if(!title.isEmpty ())
				{
					Rect textSize;
					font.measureString (textSize, title, font);
					ccl_lower_limit (w, textSize.right);
				}
			}
			w += getListStyle ().getTextRect (viewType).left + 2;
		}

		// determine item heights and cache bottom coords
		itemBottoms.resize (numItems);
		itemBottoms.setCount (numItems);
		for(int i = 0; i < numItems; i++)
		{
			h += determineRowHeight (ItemIndex (i));
			itemBottoms[i] = h;
		}
	}

	info.width = w;
	info.height = h + getListStyle ().getListPaddingBottom ();
	info.hSnap = /*viewType >= Styles::kViewIcons ? getItemWidth () :*/ 1;
	info.vSnap = getListStyle ().isVSnapEnabled () ? getItemHeight (ItemIndex ()) : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::draw (const UpdateRgn& updateRgn)
{
	if(!model)
		return;
	
	GraphicsPort port (this);
	
	Font font;
	getFont (font);
	
	if(Image* bg = getItemStyle ().getBackgroundImage ())
	{
		Rect src (bg->getSize ());
		Rect dst;
		getClientRect (dst);
		port.drawImage (bg, src, dst);
	}
	else if(isLayerBackingEnabled () && getStyle ().isTransparent () == false)
	{
		Rect dst;
		getClientRect (dst);
		UpdateRgn targetRegion (dst);
		drawAlternatingBackground (targetRegion);
	}
	
	if(viewType >= Styles::kListViewIcons)
		drawItems (port, updateRgn, font);
	else
		drawListMatrix (port, updateRgn, font);
		
	View::draw (updateRgn); // draw children
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::findItems (const Rect& rect, IItemSelection& items) const
{
	int numFound = 0;
	if(viewType >= Styles::kListViewIcons)
	{
		int row1, row2, col1, col2;
		getCellAddress (row1, col1, rect.getLeftTop ());
		getCellAddress (row2, col2, rect.getRightBottom ());
		ccl_order (row1, row2);
		ccl_order (col1, col2);

		for(int r = row1; r <= row2; r++)
			for(int c = col1; c <= col2; c++)
			{
				int index = getCellIndex (r, c, true);
				if(index >= 0)
				{
					Rect cellRect;
					Rect size;
					getItemRect (cellRect, ItemIndex (index));
					if(!measureCellContent (size, r, c) || rect.intersect (size.offset (cellRect.getLeftTop ()) ))
					{
						items.select (ItemIndex (index));
						numFound++;
					}
				}
			}
	}
	else
	{
		RowIterator rowIter (*this, rect.top, rect.bottom);
		while(rowIter.nextRow ())
		{
			items.select (ItemIndex (rowIter.row));
			numFound++;
		}
	}
	return numFound > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::findItemCell (ItemIndex& row, int& column, const Point& where) const
{
	ItemInfo info;
	if(getItemInfo (info, where))
	{
		row = info.row;
		column = info.column;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point ListView::getBackgroundOffset () const
{
	if(viewType < Styles::kListViewIcons)
		return Point (); // no offset
	return SuperClass::getBackgroundOffset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Font& ListView::getFont (Font& font) const
{
	if(viewType == Styles::kListViewIcons)
		return font = getVisualStyle ().getFont ("icons.textfont", getVisualStyle ().getTextFont ());
	else
		return SuperClass::getFont (font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::getFocusItem (ItemIndex& index) const
{
	if(focusItem.index >= 0 && focusItem.index < countItems ())
	{
		index = ItemIndex (focusItem.index);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn)
{
	if(getFocusItem (item))
	{
		getCellRect (cellRect, focusItem.row, toModelColumnIndex (focusItem.column));
		editColumn = focusItem.column;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::setFocusItem (ItemIndexRef itemIndex, tbool selectExclusive)
{
	Window::UpdateCollector uc (getWindow ());

	int oldIndex = focusItem.index;
	int newIndex = itemIndex.getIndex ();
	if(newIndex != oldIndex)
	{
		invalidateItem (ItemIndex (focusItem.index));
		if(newIndex >= 0 && newIndex < countItems ())
		{
			int oldColumn = focusItem.column;

			focusItem.index = newIndex;
			getCellAddress (focusItem.row, focusItem.column, newIndex);
			if(viewType < Styles::kListViewIcons && oldColumn >= 0)
				focusItem.column = oldColumn;

			invalidateItem (itemIndex);
		}
		else
			focusItem.index = focusItem.row = focusItem.column = -1;
	}

	if(selectExclusive)
	{
		selectAll (false);
		selectItem (itemIndex, true);
	}

	if(itemIndex.isValid ())
	{
		if(focusItem.column >= 0)
		{
			Rect cellRect;
			getCellRect (cellRect, focusItem.row, toModelColumnIndex (focusItem.column));
			makeVisible (cellRect);
		}
		else
			makeItemVisible (itemIndex);
	}

	if(newIndex != oldIndex)
		onItemFocused (ItemIndex (focusItem.index));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::setFocusCell (int row, int column)
{
	if(row != focusItem.row || column != focusItem.column)
	{
		int oldFocusItemIndex = focusItem.index;
		invalidateItem (ItemIndex (focusItem.index));
		int index = getCellIndex (row, column, true);
		if(index >= 0)
		{
			focusItem.index  = index;
			focusItem.row    = row;
			focusItem.column = column;
			invalidateItem (ItemIndex (focusItem.index));
		}
		else
			focusItem.index = focusItem.row = focusItem.column = -1;

		if(focusItem.index != oldFocusItemIndex)
			onItemFocused (ItemIndex (focusItem.index));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getAnchorItem (ItemIndex& index) const
{
	index = anchorIndex;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::setAnchorItem (ItemIndexRef index)
{
	anchorIndex = ccl_bound (index.getIndex (), 0, countItems () -1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tbool CCL_API ListView::selectAll (tbool state)
{
	getSelection ();
	if(state)
	{
		if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
			return false;

		selection->unselectAll ();
		if(state)
			for(int i = 0; i <= countItems (); i++)
				if(model->canSelectItem (i))
					selection->select (i);
		invalidate ();
	}
	else
	{
		ForEachItem (*selection, idx)
			invalidateItem (idx);
		EndFor
		selection->unselectAll ();
	}
	signalSelectionChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::selectRange (ItemIndexRef index1, ItemIndexRef index2)
{
	int from = index1.getIndex ();
	int to = index2.getIndex ();
	ccl_order (from, to);
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
		to = from;

	getSelection ();
	for(int i = from; i <= to; i++)
	{
		if(model->canSelectItem (i) && selection->isSelected (i) == false)
		{
			selection->select (i);
			invalidateItem (ItemIndex (i));
		}
	}
	signalSelectionChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListView::removeItem (ItemIndexRef index)
{
	if(model && model->removeItem (index))
	{
		if(selection)
			selection->unselect (index);
		updateSize ();
		updateClient ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::findCell (const Point& where, int& row, int& column, Rect& rect) const
{
	ASSERT (viewType < Styles::kListViewIcons)
	
	RowIterator rowIter (*this, where.y, getHeight ());
	if(!rowIter.nextRow () || where.y < rowIter.top || where.y > rowIter.bottom)
		return false;

	row = rowIter.row;
	column = -1;

	int numItems = countItems ();
	bool result = row < numItems;

	if(columnList && viewType < Styles::kListViewIcons)
	{
		int left = 0;
		int i;
		int numVisibleCols = columnList->getCount (true);
		for(i = 0; i < numVisibleCols; i++)
		{
			int colW = columnList->getColumnAtPosition (i, true)->getWidth ();
			left += colW;
			if(where.x <= left)
			{
				column = columnList->getFlatPositionFromVisible (i); // model expects the flat column index
				rect (left - colW, rowIter.top,  left, rowIter.bottom);
				if(style.isCustomStyle (Styles::kListViewAppearanceExtendLastColumn) && i == numVisibleCols - 1)
					rect.right = getWidth ();
				break;
			}
		}

		if(column == -1) // after last column...
		{
			if(style.isCustomStyle (Styles::kListViewAppearanceExtendLastColumn))
			{
				column = columnList->getFlatPositionFromVisible (numVisibleCols - 1);
				getCellRect (rect, row, numVisibleCols - 1);
				rect.right = getWidth ();
			}
			else
			{
				// column = i; -> must be -1, 'i' is wrong when colums are hidden and last column should only be extended with kListViewExtendLastColumn
				rect (left, rowIter.top, getWidth (), rowIter.bottom);
			}
			//result = false;
		}
	}
	else // no columns, cell takes full width
	{
		column = 0;
		rect (0, rowIter.top, getWidth (), rowIter.bottom);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getCellRect (Rect& rect, int row, int column) const
{
	if(viewType >= Styles::kListViewIcons)
	{
		if(measureCellContent (rect, row, column))
		{
			int itemW = getItemWidth ();
			int itemH = getItemHeight (row);
			rect.moveTo (Point (column * itemW, row * itemH));
		}
	}
	else
	{
		getRowRect (rect, row);

		if(columnList)
		{
			columnList->getColumnRange (rect.left, rect.right, column);
			if(style.isCustomStyle (Styles::kListViewAppearanceExtendLastColumn) && columnList->columnIndexToPosition (column, true) == columnList->getCount (true) - 1)
				rect.right = getWidth ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getItemInfo (ItemInfo& info, const Point& where) const
{
	if(!model)
		return false;

	bool result = false;
	if(viewType < Styles::kListViewIcons)
	{
		result = findCell (where, info.row, info.column, info.rect);
		if(result)
			info.index = info.row;
	}
	else
	{
		if(getCellAddress (info.row, info.column, where))
		{
			info.index = info.row * countColumns () + info.column;

			int itemW = getItemWidth ();
			int itemH = getItemHeight (ItemIndex (info.row));

			info.rect.left = info.column * itemW;
			info.rect.right = info.rect.left + itemW;
			info.rect.top = info.row * itemH;
			info.rect.bottom = info.rect.top + itemH;

			ListStyle& listStyle = getListStyle ();
			if(where.x - info.rect.left > listStyle.getMarginH ()
				&& where.y - info.rect.top > listStyle.getMarginV ())
			result = info.index >= 0 && info.index < countItems ();
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getColumnIndex (PointRef where)
{
	ItemInfo info;
	if(getItemInfo (info, where))
		return info.column;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getLogicalColumnIndex (PointRef where)
{
	if(viewType == Styles::kListViewIcons)
		return 0; // ignore the layout based column

	return getColumnIndex (where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::getNextItem (ItemIndex& item, bool forNavigation)
{
	int numItems = countItems ();
	if(item.isValid ())
	{
		int index = item.getIndex () + 1;
		if(index < numItems)
		{
			item = index;
			return true;
		}
	}

	if(!forNavigation)
		return false;

	// wrap to first item
	item = 0;
	return numItems > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly)
{
	struct WrapRow
	{
		WrapRow (int& row, int& col, int numCols)
		{
			// wrap to next / previous row at column end / start
			if(col >= numCols)
				col = 0, row++;
			else if(col < 0)
				col = numCols - 1, row--;
		}
	};

	if((privateFlags & kActive) == 0)
		return false;

	if(checkOnly) // todo
		return true;

	if(!style.isCustomStyle (Styles::kItemViewBehaviorSelection))
		navigationMode = ItemView::kSkip;
	
	// ignore columnlist in icons mode
	ColumnHeaderList* columnList = viewType < Styles::kListViewIcons ? this->columnList : nullptr; 

	int numCols = columnList ? columnList->getCount (true) : countColumns ();
	if(numCols < 1)
		numCols = 1;

	int numRows = countRows ();
	int row = focusItem.row;
	int col = focusItem.column;
	if(columnList)
		col = columnList->getVisiblePositionFromFlat (col);

	if(rows == NumericLimits::kMaxInt)
	{
		row = numRows - 1;
		if(!style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
			col = numCols - 1;
	}
	else if(rows == NumericLimits::kMinInt) 
	{
		row = 0;
		if(!style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
			col = 0;
	}
	else
	{
		if(getStyle ().isCustomStyle (Styles::kListViewBehaviorNavigateFlat))
		{
			// item index based navigation (next / previous item)
			int delta = rows != 0 ? rows : columns;
			int index = focusItem.index + delta;

			getCellAddress (row, col, index); // bound row / col to actual item
		}
		else
		{
			row += rows;
			col += columns;

			if(style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
				col = ccl_bound (col, 0, numCols - 1);
			else
				WrapRow (row, col, numCols);
		}
	}

	int index = getCellIndex (row, col, false);
	if(viewType >= Styles::kListViewIcons)
		getCellAddress (row, col, index); // bound row / col to actual item
	ItemIndex itemIndex (index);

	while(!model->canSelectItem (itemIndex))
	{
		// skip unselectable items
		int skipDir = ccl_sign (columns ? columns : rows);

		// when targeting extreme positions: skip in other direction
		if(ccl_abs (rows) == NumericLimits::kMaxInt)
			skipDir *= -1;

		col += skipDir;
		WrapRow (row, col, numCols);

		if(row < 0 || row >= numRows)
			return false;

		index = getCellIndex (row, col, false);
		itemIndex = index;
	}

	Window::UpdateCollector uc (getWindow ());

	bool updateSelection = true;

	if(ColumnHeader* columnHeader = columnList ? columnList->getColumnAtPosition (col, true) : nullptr)
	{
		getSelection ();
		if(model->canSelectItem (itemIndex) && selection->isSelected (itemIndex))
			updateSelection = !columnHeader->canEditMultiple ();
	}
	
	switch(navigationMode)
	{
		case kSkip:
			anchorIndex = index;
			break;
		case kSelect:
			if(updateSelection)
			{
				selectAll (false);
				selectRange (index, index);
				anchorIndex = index;
			}
			break;
		case kSelectExtend:
			selectAll (false);
			// through
		case kSelectExtendAdd:
			selectRange (anchorIndex, index);
			break;
	}

	if(style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
	{
		// back to flat column index
		if(columnList)
			col = columnList->getFlatPositionFromVisible (col);

		setFocusCell (row, col);

		Rect cellRect;
		getCellRect (cellRect, row, toModelColumnIndex (col));
		makeVisible (cellRect);
	}
	else
		setFocusItem (itemIndex, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onEditDelete (const CommandMsg& args)
{
	if(!args.checkOnly ())
	{
		// first check if model handles delete command
		ItemIndex focusItem;
		getFocusItem (focusItem);
		if(model->interpretCommand (args, focusItem, getSelection ()))
			return true;

		LinkedList<int> toDelete;
		ForEachItem (getSelection (), idx)
			toDelete.addSorted (idx.getIndex ());
		EndFor

		// remove in reverse order to avoid index changes
		bool result = false;
		ListForEachReverse (toDelete, int, idx)
			if(model->removeItem (idx))
			{
				selectItem (idx, false);
				result = true;
			}
		EndFor
		return result;
	}
	return SuperClass::onEditDelete (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ListView::createTouchHandler (const TouchEvent& event)
{
	if(style.isCustomStyle (Styles::kListViewBehaviorSwipeToFocus))
	{
		GestureHandler* handler = NEW GestureHandler (this, GestureEvent::kSwipe);
		handler->addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
		return handler;
	}

	ITouchHandler* handler = SuperClass::createTouchHandler (event);
	if(GestureHandler* gestureHandler = unknown_cast<GestureHandler> (handler))
	{
		gestureHandler->addRequiredGesture (GestureEvent::kSwipe);
		if(style.isCustomStyle (Styles::kItemViewBehaviorSelection) && !style.isCustomStyle (Styles::kItemViewBehaviorNoDoubleClick))
			gestureHandler->addRequiredGesture (GestureEvent::kDoubleTap);
	}

	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onGesture (const GestureEvent& event)
{
	// first give model a chance
	ItemInfo info;
	
	if(getItemInfo (info, event.where) && editCell (info.index, info.column, info.rect, event))
		return true;

	switch(event.getType ())
	{
	case GestureEvent::kLongPress :
		if(!getStyle ().isCustomStyle (Styles::kItemViewBehaviorNoDrag)) // handle long press as swipe if not used for dragging (in ItemView)
			break;
	case GestureEvent::kSwipe :
		if(style.isCustomStyle (Styles::kListViewBehaviorSwipeToFocus))
		{
			ScopedFlag<Styles::kItemViewBehaviorAutoSelect> scope (style.custom);
			onMouseMove (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseMove, event));
		}
		break;

	case GestureEvent::kSingleTap :
	case GestureEvent::kDoubleTap :
		return onTap (event);
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onTap (const GestureEvent& event)
{
	if(!model)
		return false;
	
	ItemInfo info;
	
	if(getItemInfo (info, event.where))
	{
		ItemIndex clickedItem (info.index);
		if(isDeleteFocusItemMode ())
		{
			Font font;
			getFont (font);
			Rect r (0, info.rect.top, getWidth (), info.rect.bottom);
			if(getListStyle ().getDeleteButtonRect (r, font).pointInside (event.where))
				removeItem (focusItem.index);

			setDeleteFocusItemMode (false);
		}

		if(event.getType () == GestureEvent::kDoubleTap && event.getState () != GestureEvent::kPossible)
		{
			if(openItem (clickedItem, info.column, event))
				return true;
		}

		tbool canSelect = model->canSelectItem (clickedItem);
		if(canSelect)
		{
			setFocusCell (info.row, info.column);
			
			tbool isSelected = getSelection ().isSelected (clickedItem);
			bool canEditMultiple = false;
			if(getColumnHeaders ())
				if(ColumnHeader* columnHeader = getColumnHeaders ()->getColumnByIndex (info.column))
					canEditMultiple = columnHeader->canEditMultiple ();
			
			// keep selection for contextmenu & columns that canEditMultiple
			if(!(isSelected && canEditMultiple))
				doSelection (clickedItem, event);
		}
		else
			setFocusItem (-1, true);
		
		// edit cell
		//was: if(viewType < Styles::kViewIcons)
		if(editCell (clickedItem, info.column, info.rect, event))
			return true;
	}
	else
	{
		if(isDeleteFocusItemMode ())
			setDeleteFocusItemMode (false);
		
		selectAll (false);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onMouseDown (const MouseEvent& event)
{
	if(!model)
		return false;

	ItemInfo info;
	bool itemHit = getItemInfo (info, event.where);
	if(itemHit && !style.isCustomStyle (Styles::kItemViewBehaviorNoRubberband))
	{
		// check if actual cell content was hit
		Rect contentSize;
		itemHit = !measureCellContent (contentSize, info.row, info.column) || contentSize.offset (info.rect.getLeftTop ()).pointInside (event.where);
	}
	//todo: if columnList, select item if row was hit...

	if(itemHit)
	{
		if(!isFocused () && focusItem.row == info.row && focusItem.column == info.column)
			if(SuperClass::onMouseDown (event)) // give child views a chance
				return true;

		ItemIndex clickedItem (info.index);

		if(isDeleteFocusItemMode ())
		{
			Font font;
			getFont (font);
			Rect r (0, info.rect.top, getWidth (), info.rect.bottom);
			if(getListStyle ().getDeleteButtonRect (r, font).pointInside (event.where))
			{
				removeItem (focusItem.index);

				setDeleteFocusItemMode (false);
			}
			return true;
		}

		bool didEditCell = false;
		tbool canSelect = model->canSelectItem (clickedItem);
		if(canSelect)
		{
			SharedPtr<Unknown> lifeGuard (this);
			
			setFocusCell (info.row, info.column);

			bool editResult = false;

			// in list / details mode with columns, give editCell a chance before we try dragging
			if(viewType < Styles::kListViewIcons && !style.isCustomStyle (Styles::kItemViewBehaviorNoDrag) && getColumnHeaders () != nullptr)
			{
				didEditCell = true;
				editResult = editCell (clickedItem, info.column, info.rect, event);
				if(editResult && isAttached () == false)
					return true;
			}

			// try to drag the existing selection
			tbool isSelected = getSelection ().isSelected (clickedItem);
			if(isSelected && !editResult)
				if(tryDrag (event))
					return true;

			bool canEditMultiple = false;
			if(getColumnHeaders ())
				if(ColumnHeader* columnHeader = getColumnHeaders ()->getColumnByIndex (info.column))
					canEditMultiple = columnHeader->canEditMultiple ();
			
			// keep selection for contextmenu & columns that canEditMultiple
			if(!(isSelected && (event.keys.isSet (KeyState::kRButton) || canEditMultiple)))
				doSelection (clickedItem, event);

			if(editResult)
				return true;
		}
		else
			setFocusItem (-1, true);

		// edit cell
		if(!didEditCell && editCell (clickedItem, info.column, info.rect, event))
			return true;

		// try to open current item
		if(!style.isCustomStyle (Styles::kItemViewBehaviorNoDoubleClick))
		{
			bool rightClick = event.keys.isSet (KeyState::kRButton);
			if(!rightClick && detectDoubleClick (event))
				if(openItem (clickedItem, info.column, event))
					return true;
		}

		// drag the new selection
		return tryDrag (event);		
	}
	else
	{
		if(isDeleteFocusItemMode ())
			setDeleteFocusItemMode (false);

		if(tryRubberSelection (event))
			return true;
		
		if(!style.isCustomStyle (Styles::kItemViewBehaviorNoUnselect))
			selectAll (false);
		
		if(getStyle ().isCustomStyle (Styles::kListViewAppearanceCenterRows))
			return false; // create potential mouseHandler, when no item was hit because of "centerrows"
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onContextMenu (const ContextMenuEvent& event)
{
	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::modelChanged (int changeType, ItemIndexRef item)
{
	if(changeType == kItemRemoved)
	{
		if(item.getIndex () == anchorIndex)
			anchorIndex = 0;
		
		if(item.getIndex () == focusItem.index)
			focusItem = ItemInfo ();

		if(selection)
			selection->unselect (item);
	}
	else if(changeType == kModelChanged)
	{
		int firstSelected = -1;
		if(IItemSelection* modelSelection = model ? model->getSelection () : static_cast<IItemSelection*> (nullptr))
		{
			ForEachItem (*modelSelection, index)
				firstSelected = index.getIndex ();
				break;
			EndFor
		}
		if(firstSelected >= 0)
		{
			anchorIndex = firstSelected;
			focusItem.index = anchorIndex;
			getCellAddress (focusItem.row, focusItem.column, anchorIndex);			
		}
		else
		{
			anchorIndex = 0;
			ItemInfo info;
			info.index = -1;
			focusItem = info;
		}
	}
	ItemView::modelChanged (changeType, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::drawItems (GraphicsPort& port, const UpdateRgn& updateRgn, const Font& font)
{
	int rowH = getItemHeight (ItemIndex ());
	int colW = getItemWidth ();

	int numRows = countRows ();
	int numColumns = countColumns ();
	int numItems = countItems ();

	int firstRow = updateRgn.bounds.top / rowH;
	int lastRow  = ccl_min<int> (updateRgn.bounds.bottom / rowH, numRows - 1);
	
	bool done = false;
	for(int row = firstRow; row <= lastRow; row++)
	{
		int left = 0, top = row * rowH;

		for(int column = 0; column < numColumns; column++, left += colW)
		{
			if(left > updateRgn.bounds.right || left + colW < updateRgn.bounds.left)
				continue;

			int index = row * numColumns + getIndexFromColumn (column);
			if(index >= numItems)
				continue;
			
			Rect r (0, 0, colW, rowH);
			r.offset (left, top);

			ClipSetter cs (port, r);
			drawItem (port, r, index, font);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::autoCenterItemRect (Rect& rect)
{
	Coord itemWidth = getItemWidth ();
	Coord xOffset = ((itemWidth - rect.getWidth ()) / 2) - rect.left - getListStyle ().getMarginH ();
	rect.offset (xOffset, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::drawItem (GraphicsPort& port, const Rect& rect, int index, const Font& font)
{
	ItemIndex itemIndex (index);
	bool selected = getSelection ().isSelected (itemIndex) != 0;

	ListStyle& listStyle = getListStyle ();
	BrushRef textBrush (selected ? listStyle.getSelectedTextBrush () : listStyle.getTextBrush ());
	IItemModel::StyleInfo styleInfo = {font, textBrush, listStyle.getBackBrush1 (), listStyle.getIconColor ()};

	int state = selected ? IItemModel::DrawInfo::kItemSelectedState : 0;
	IItemModel::DrawInfo itemInfo = {this, port, rect, styleInfo, state};

	if(style.isCustomStyle (Styles::kListViewAppearanceDrawCustomItem))
	{
		model->drawItem (itemIndex, itemInfo);
		return;
	}

	// *** Draw Selection ***
	Rect outerRect (listStyle.getItemSize (viewType));
	Rect dataRect (listStyle.getDataRect (viewType));
	
	if(style.isCustomStyle (Styles::kListViewAppearanceAutoCenterIcons))
	{
		autoCenterItemRect (outerRect);
		autoCenterItemRect (dataRect);
	}
	
	outerRect.offset (rect.left + listStyle.getMarginH (), rect.top + listStyle.getMarginV ());
	dataRect.offset (rect.left + listStyle.getMarginH (), rect.top + listStyle.getMarginV ());
	
	auto drawListStyleImage = [&](Image* image, bool atTopLeft = false)
	{
		if(image->getFrameCount () > 1)
			image->setCurrentFrame (selected && style.isCustomStyle (Styles::kItemViewBehaviorSelection) ? 1 : 0);
		
		Rect src (image->getSize ());
		Rect dst (outerRect);
		
		if(atTopLeft)
		{
			dst.setWidth (src.getWidth ());
			dst.setHeight (src.getHeight ());
		}
		
		port.drawImage (image, src, dst);
	};
	
	if(Image* bg = unknown_cast<Image> (listStyle.getIconBackgroundImage ()))
	{
		drawListStyleImage (bg);
	}
	else if(selected && style.isCustomStyle (Styles::kItemViewBehaviorSelection))
	{
		port.fillRect (outerRect, listStyle.getSelectBrush ());
	}

	ImageMode mode (1.f, listStyle.isHighQualityMode () ? ImageMode::kInterpolationHighQuality : ImageMode::kInterpolationDefault);


	// *** Draw Icon ***
	auto getIcon = [&] (ItemIndexRef itemIndex)
	{
		if(listStyle.isThumbnailsAsIcons ())
			if(IImage* thumb = model->getItemThumbnail (itemIndex))
				return thumb;

		IImage* icon = model->getItemIcon (itemIndex);
		if(!icon)
			icon = style.isCustomStyle (Styles::kListViewAppearanceNoDefaultIcon) ? nullptr : listStyle.getDefaultIcon ();
		return icon;
	};
	if(IImage* icon = getIcon (itemIndex))
	{
		bool scaleDown = (listStyle.isIconSetHalfSize () && (icon->getType () == IImage::kMultiple));
		if(scaleDown)
			dataRect.contract (dataRect.getWidth () / 4);

		getTheme ().getPainter ().drawBestMatchingFrame (port, icon, dataRect, &mode, selected ? listStyle.getIconSelectedIconColor () : listStyle.getIconColor (), scaleDown);
	}

	// *** Draw Overlay ***
	if(Image* overlay = unknown_cast<Image> (listStyle.getIconOverlayImage ()))
	{
		drawListStyleImage (overlay);
		
		if(model->isItemFolder (itemIndex))
			if(Image* folder = unknown_cast<Image> (listStyle.getIconOverlayFolderImage ()))
				drawListStyleImage (folder, true);
	}

	IItemModel::DrawInfo info = {this, port, dataRect, styleInfo, state};
	model->drawIconOverlay (itemIndex, info);

	// *** Draw Title ***
	String title;
	model->getItemTitle (title, itemIndex);
	if(!title.isEmpty ())
	{
		Rect textRect (listStyle.getTextRect (viewType));
		
		if(style.isCustomStyle (Styles::kListViewAppearanceAutoSizeItems))
			textRect.setWidth (getItemWidth () - (2 * listStyle.getMarginH ()));
		else if(style.isCustomStyle (Styles::kListViewAppearanceAutoCenterIcons))
		{
			autoCenterItemRect (textRect);
		}
		
		textRect.offset (rect.left + listStyle.getMarginH (), rect.top + listStyle.getMarginV ());

		if(listStyle.isMultiLineTitle ())
		{
			Rect neededSize;
			Font::measureText (neededSize, textRect.getWidth (), title, font);
			
			auto numLinesFromHeight = [&](int height)
			{
				static const float kSpaceToNextLine = 3.3f;
				return ccl_to_int (height / (font.getSize () + kSpaceToNextLine));
			};
			
			int neededLines = numLinesFromHeight (neededSize.getHeight ());
			int availableLines = numLinesFromHeight (textRect.getHeight ());
			
			String drawTitle (title);
			Alignment alignment (Alignment::kTop|Alignment::kHCenter);
			
			if(neededLines < availableLines)
			{
				alignment = Alignment::kCenter;
			}
			else if (neededLines > availableLines)
			{
				static const int kSpaceWastedByWordWrappingHeuristic = 23;
				int kLineWidthDeduction = (availableLines > 1) ? kSpaceWastedByWordWrappingHeuristic : 0;
				Font::collapseString (drawTitle, (textRect.getWidth () - kLineWidthDeduction) * availableLines, font, Font::TrimMode::kTrimModeRight);
			}
			
			port.drawText (textRect, drawTitle, font, textBrush, TextFormat (alignment, TextFormat::kWordBreak));
		}
		else
		{
			Font::collapseString (title, textRect.getWidth (), font, textTrimMode);
			port.drawString (textRect, title, font, textBrush, Alignment::kCenter);
		}
	}

	// *** Draw Focus ***
	if(index == focusItem.index)
	{
		if(Image* focus = unknown_cast<Image> (listStyle.getIconFocusImage ()))
		{
			if(shouldDrawFocus ())
				drawListStyleImage (focus);
		}
		else
		{
			Rect focusRect (outerRect);
			focusRect.contract (1);
			drawFocusRect (port, focusRect);
		}
	}

	#if DEBUG_LOG
	Rect cell (rect);
	port.drawRect (cell, Pen (Colors::kRed));

	int mH = getListStyle ().getMarginH ();
	int mV = getListStyle ().getMarginV ();

	cell.offset (mH, mV);
	cell.setWidth (cell.getWidth () - 2 * mH);
	cell.setHeight (cell.getHeight () - 2 * mV);

	port.drawRect (cell, Pen (Colors::kGreen));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::drawCell (GraphicsPort& port, const Rect& rect, int row, int column, int state, FontRef font, BrushRef textBrush)
{
	ItemIndex itemIndex (row);
	bool selected = (state & IItemModel::DrawInfo::kItemSelectedState) ? true : false;
	Color adaptiveColor = selected ? getItemStyle ().getSelectedIconColor () : getItemStyle ().getIconColor ();
	IItemModel::StyleInfo styleInfo = {font, textBrush, getItemStyle ().getBackBrush1 (), adaptiveColor};
	IItemModel::DrawInfo info = {this, port, rect, styleInfo, state};
	if(!model->drawCell (ItemIndex (row), column, info))
	{
		if(column == 0)
		{
			String title;
			model->getItemTitle (title, ItemIndex (row));
			ListStyle& listStyle = getListStyle ();

			Rect iconSize (listStyle.getDataRect (Styles::kListViewList));
			ItemStyle::CustomBackground* bg = getCustomBackground (getModel ()->getItemBackground (ItemIndex (row)));
			if(bg && bg->iconSize >= iconSize.getWidth ())
			{
				iconSize.top = 0;
				iconSize.setWidth (bg->iconSize);
				iconSize.setHeight (bg->iconSize);
				iconSize.offset (0, (rect.getHeight () - bg->iconSize) / 2);
			}
			
			if(!iconSize.isEmpty ())
			{
				Rect iconRect (iconSize);
				iconRect.offset (rect.left, rect.top);

				Image* icon = unknown_cast<Image> (model->getItemIcon (itemIndex));
				if(!icon)
					icon = style.isCustomStyle (Styles::kListViewAppearanceNoDefaultIcon) ? nullptr : listStyle.getDefaultIcon ();
				if(icon)
				{
					getTheme ().getPainter ().drawBestMatchingFrame (port, icon, iconRect, nullptr, selected ? listStyle.getSelectedIconColor () : listStyle.getIconColor ());

					// draw overlay
					IItemModel::StyleInfo styleInfo = {font, textBrush, getItemStyle ().getBackBrush1 (), getItemStyle ().getIconColor ()};
					IItemModel::DrawInfo info = {this, port, iconRect, styleInfo, 0};
					model->drawIconOverlay (itemIndex, info);
				}

				Rect titleRect (rect);
				titleRect.left = iconRect.right + listStyle.getMarginH ();

				if(!title.isEmpty ())
					port.drawString (titleRect, title, font, textBrush, listStyle.getListTextAlignment ());
			}
			else if(!title.isEmpty ())
			{
				Rect titleRect (rect);
				titleRect.left += listStyle.getListTextMargin ();
				port.drawString (titleRect, title, font, textBrush, listStyle.getListTextAlignment ());
			}
		}
	}

	if(style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus) && row == focusItem.row && column == focusItem.column)
		drawFocusRect (port, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::measureCellContent (Rect& size, int row, int column) const
{
	if(viewType >= Styles::kListViewIcons)
	{
		int index = getCellIndex (row, column, true);
		if(index >= 0)
		{
			const ItemStyle& style = getItemStyle ();
			int marginH = style.getMarginH ();
			int marginV = style.getMarginV ();
			size (marginH, marginV, getItemWidth () - marginH, getItemHeight (ItemIndex ()) - marginV);
			return true;
		}
	}
	else
	{
		// todo: let model->measureCellContent
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::drawListMatrix (GraphicsPort& port, const UpdateRgn& updateRgn, FontRef _font)
{
	bool noClip = style.isCustomStyle (Styles::kListViewAppearanceDontClipCells);
	bool extendLastColumn = style.isCustomStyle (Styles::kListViewAppearanceExtendLastColumn);

	int numColumns = columnList ? columnList->getCount (true) : 0;
	if(numColumns < 1)
		numColumns = 1;

	RowIterator rowIter (*this, updateRgn.bounds.top, updateRgn.bounds.bottom);
	while(rowIter.nextRow ())
	{
		int row = rowIter.row;
		Coord rowRight = getWidth ();
		ItemIndex itemIndex (row);

		Rect r (0, rowIter.top, rowRight, rowIter.bottom);
		ItemStyle::CustomBackground* customBackground = getCustomBackground (model->getItemBackground (itemIndex));
		FontRef font = customBackground && customBackground->textFont ? *customBackground->textFont : _font;

		// draw selection or custom background
		bool selected = style.isCustomStyle (Styles::kItemViewBehaviorSelection) && getSelection ().isSelected (itemIndex);
		if(selected)
		{
			IImage* selectionBackground = getListStyle ().getSelectionBackground ();
			if(selectionBackground)
			{
				Rect src (0,0, selectionBackground->getWidth (), selectionBackground->getHeight ());
				port.drawImage (selectionBackground, src, r);
			}
			else
				port.fillRect (r, getItemStyle ().getSelectBrush ());

		}
		else if(IImage* listBackground = getListStyle ().getListItemBackground ())
		{
			Rect src (0,0, listBackground->getWidth (), listBackground->getHeight ());
			port.drawImage (listBackground, src, r);			
		}
		else if(!hasAlternatingBackground ()) // already drawn
		{
			if(customBackground)
			{
				if(SolidBrush* brush = customBackground->brush [row % 2])
					port.fillRect (r, *brush);
				//if(bg->separatorPen)
				//	port.drawRect (Rect (r).setHeight (1), *bg->separatorPen);
			}
		}

		if(row == focusItem.row && !style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
			drawFocusRect (port, r);

		if(row == focusItem.row)
		{
			if(isDeleteFocusItemMode ())
			{
				Rect deleteRect (getListStyle ().getDeleteButtonRect (r, font));
				getListStyle ().drawDeleteButton (port, deleteRect, font);

				r.right = rowRight = deleteRect.left;
				if(r.getWidth () <= 0)
					continue;
			}
		}

		Image* thumbnailImage = unknown_cast<Image> (getThumbnail (itemIndex));
		if(thumbnailImage)
			r.bottom -= getThumbnailAreaHeight (thumbnailImage);

		BrushRef textBrush (selected ? getItemStyle ().getSelectedTextBrush () : getItemStyle ().getTextBrush ());
		Coord colW;

		for(int column = 0; column < numColumns; column++)
		{
			ColumnHeader* c = columnList ? columnList->getColumnAtPosition (column, true) : nullptr;
			if(extendLastColumn && column == numColumns - 1)
				colW = rowRight - r.left; // full remaining width for last column
			else
				colW = c ? c->getWidth () : rowRight;

			if(colW <= 0)
				continue;
			
			if(noClip == false && r.left + colW < updateRgn.bounds.left)
			{
				r.left += colW;
				continue;
			}

			r.setWidth (colW);
			ccl_upper_limit (r.right, rowRight);
			if(r.right <= r.left)
				break;

			int columnIndex = c ? c->getIndex () : column;
			int state = selected ? IItemModel::DrawInfo::kItemSelectedState : 0;
			
			drawCell (port, r, row, columnIndex, state, font, textBrush);

			r.left += colW;

			if(r.left > updateRgn.bounds.right)
				break;
		}

		if(thumbnailImage)
		{
			Point pos (getItemStyle ().getMarginH (), r.bottom + getItemStyle ().getThumbnailPaddingTop ());
			drawThumbnail (port, *thumbnailImage, pos);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ListView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ListViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

