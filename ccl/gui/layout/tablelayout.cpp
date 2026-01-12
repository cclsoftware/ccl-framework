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
// Filename    : ccl/gui/layout/tablelayout.cpp
// Description : TableLayout
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/layout/tablelayout.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/layout/layoutprimitives.h"

#include "ccl/gui/skin/skinattributes.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

namespace CCL {

//************************************************************************************************
// TableLayout
//************************************************************************************************

class TableLayoutAlgorithm: public AnchorLayoutAlgorithm
{
public:
	TableLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData, TableLayout& tableLayout);

	// LayoutAlgorithm
	void doLayout () override;
	void calcSizeLimits (SizeLimit& limits) override;
	void onChildSized (View* child, const Point& delta) override;
	AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) override;
	AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) override;

private:
	TableLayout& tableLayout;
	void onViewsChanged ();

	ObjectArray rowItems;
	ObjectArray colItems;
};


//************************************************************************************************
// RowColumnIteraor
//************************************************************************************************

class RowColumnIteraor
{
public:
	RowColumnIteraor (TableLayout& layout);

	void next ();

	int rowIndex;
	int colIndex;
private:
	int numRows;
	int numCols;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

RowColumnIteraor::RowColumnIteraor (TableLayout& layout)
: numRows (layout.getNumRows ()),
  numCols (layout.getNumColumns ()),
  rowIndex (0),
  colIndex (0)
{
	if(numCols == 0 && numRows == 0)
		numCols = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RowColumnIteraor::next ()
{
	if(numCols == 0)
	{
		rowIndex++;
		if(rowIndex == numRows)
		{
			rowIndex = 0;
			colIndex++;
		}
	}
	else
	{
		colIndex++;
		if(colIndex == numCols)
		{
			colIndex = 0;
			rowIndex++;
		}
	}
}

} // namespace CCL

//************************************************************************************************
// TableLayout
//************************************************************************************************

DEFINE_CLASS (TableLayout, AnchorLayout)

//////////////////////////////////////////////////////////////////////////////////////////////////

TableLayout::TableLayout ()
: numRows (0),
  numCols (0),
  cellRatio (0),
  minCellRatio (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TableLayout::setAttributes (const SkinAttributes& a)
{
	numRows = a.getInt (ATTR_ROWS, 0);
	numCols = a.getInt (ATTR_COLUMNS, 0);
	cellRatio = a.getFloat (ATTR_CELLRATIO, 0);
	minCellRatio = a.getFloat (ATTR_MINCELLRATIO, cellRatio);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TableLayout::getAttributes (SkinAttributes& a) const
{
	if(numRows >= 0)
		a.setInt (ATTR_ROWS, numRows);
	else
		a.setString (ATTR_ROWS, String::kEmpty);

	if(numCols >= 0)
		a.setInt (ATTR_COLUMNS, numCols);
	else
		a.setString (ATTR_COLUMNS, String::kEmpty);

	if(cellRatio != 0)
		a.setFloat (ATTR_CELLRATIO, cellRatio);
	
	if(minCellRatio != 0)
		a.setFloat (ATTR_MINCELLRATIO, minCellRatio);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TableLayout::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == ATTR_ROWS)
	{
		var = numRows;
		return true;
	}
	else if(propertyId == ATTR_COLUMNS)
	{
		var = numCols;
		return true;
	}
	else if(propertyId == ATTR_CELLRATIO)
	{
		var = cellRatio;
		return true;
	}
	else if(propertyId == ATTR_MINCELLRATIO)
	{
		var = minCellRatio;
		return true;
	}
	
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TableLayout::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == ATTR_ROWS)
	{
		setNumRows (var.asInt ());
		return true;
	}
	else if(propertyId == ATTR_COLUMNS)
	{
		setNumColumns (var.asInt ());
		return true;
	}
	else if(propertyId == ATTR_CELLRATIO)
	{
		setCellRatio (var.asInt ());
		return true;
	}
	else if(propertyId == ATTR_MINCELLRATIO)
	{
		setMinCellRatio (var.asInt ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* TableLayout::createAlgorithm (LayoutContext* context)
{
	if(auto* anchorLayoutContext = ccl_cast<AnchorLayoutContext> (context))
		return NEW TableLayoutAlgorithm (anchorLayoutContext, layoutData, *this);

	return nullptr;
}

//************************************************************************************************
// TableLayoutAlgorithm
//************************************************************************************************

TableLayoutAlgorithm::TableLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData, TableLayout& tableLayout)
: AnchorLayoutAlgorithm (context, layoutData),
  tableLayout (tableLayout)
{
	rowItems.objectCleanup (true);
	colItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutAlgorithm* TableLayoutAlgorithm::onViewAdded (int index, AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutAlgorithm* TableLayoutAlgorithm::onViewRemoved (AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TableLayoutAlgorithm::onViewsChanged ()
{
	// recreate LayoutItems for rows and columns
	rowItems.removeAll ();
	colItems.removeAll ();

	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());
	
	// accumulate size limits and preferred sizes for rows and columns
	RowColumnIteraor iterator (tableLayout);
	ArrayForEach (context->getLayoutItems (), AnchorLayoutItem, item)
		AnchorLayoutItem* rowItem = static_cast<AnchorLayoutItem*> (rowItems.at (iterator.rowIndex));
		AnchorLayoutItem* colItem = static_cast<AnchorLayoutItem*> (colItems.at (iterator.colIndex));
		if(!rowItem)
		{
			rowItems.add (rowItem = NEW AnchorLayoutItem);
			rowItem->sizeLimits.maxHeight = 0;
		}
		if(!colItem)
		{
			colItems.add (colItem = NEW AnchorLayoutItem);
			colItem->sizeLimits.maxWidth = 0;
		}

		ccl_lower_limit (colItem->sizeLimits.minWidth,  item->sizeLimits.minWidth);
		ccl_lower_limit (colItem->sizeLimits.maxWidth,  item->sizeLimits.maxWidth);
		ccl_lower_limit (rowItem->sizeLimits.minHeight, item->sizeLimits.minHeight);
		ccl_lower_limit (rowItem->sizeLimits.maxHeight, item->sizeLimits.maxHeight);

		ccl_lower_limit (colItem->preferredSize.x, item->preferredSize.x);
		ccl_lower_limit (rowItem->preferredSize.y, item->preferredSize.y);

		iterator.next ();
	EndFor

	// ensure that the prefered size for each column / row is inside it's limits
	ArrayForEach (colItems, AnchorLayoutItem, item)
		LayoutPrimitives::checkMinSize<HorizontalDirection> (item->preferredSize, item->sizeLimits);
		LayoutPrimitives::checkMaxSize<HorizontalDirection> (item->preferredSize, item->sizeLimits);
	EndFor
	ArrayForEach (rowItems, AnchorLayoutItem, item)
		LayoutPrimitives::checkMinSize<VerticalDirection> (item->preferredSize, item->sizeLimits);
		LayoutPrimitives::checkMaxSize<VerticalDirection> (item->preferredSize, item->sizeLimits);
	EndFor

	#if DEBUG_LOG
	CCL_PRINTF ("TableLayout: %d rows, %d columns\n", rowItems.count (), colItems.count ())
	int i = 0;
	ArrayForEach (colItems, LayoutItem, item)
		const SizeLimit& limits = item->sizeLimits;
		CCL_PRINTF ("  Col. %d: pref: %d, min: %d, max: %d\n", i, item->preferredSize.x, limits.minWidth, limits.maxWidth);
		i++;
	EndFor
	i = 0;
	ArrayForEach (rowItems, LayoutItem, item)
		const SizeLimit& limits = item->sizeLimits;
		CCL_PRINTF ("  Row. %d: pref: %d, min: %d, max: %d\n", i, item->preferredSize.y, limits.minHeight, limits.maxHeight);
		i++;
	EndFor
	#endif

	preferredSize (
		LayoutPrimitives::accumulatePreferredSize<HorizontalDirection> (colItems, margin, spacing),
		LayoutPrimitives::accumulatePreferredSize<VerticalDirection> (rowItems, margin, spacing));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TableLayoutAlgorithm::doLayout ()
{
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());
	
	// layout the rows amd columns separately
	LayoutPrimitives::calcBoxLayout<VerticalDirection>   (rowItems, context->getLayoutHeight (), margin, spacing);
	LayoutPrimitives::calcBoxLayout<HorizontalDirection> (colItems, context->getLayoutWidth (),  margin, spacing);

	// apply the calculated layout
	RowColumnIteraor iterator (tableLayout);
	ArrayForEach (context->getLayoutItems (), AnchorLayoutItem, item)
		AnchorLayoutItem* rowItem = static_cast<AnchorLayoutItem*> (rowItems.at (iterator.rowIndex));
		AnchorLayoutItem* colItem = static_cast<AnchorLayoutItem*> (colItems.at (iterator.colIndex));
		if(rowItem && colItem)
		{
			const Rect& colRect = colItem->workRect;
			const Rect& rowRect = rowItem->workRect;

			View* view = item->getView ();
			int sizeMode = view->getSizeMode ();

			// each view gets it's preferred size, or stretched to the cell if attached to both edges
			Rect cell (colRect.left, rowRect.top, colRect.right, rowRect.bottom);		
			Rect r (cell);

			if(!LayoutPrimitives::isSizeMode<IView::kAttachLeft|IView::kAttachRight> (sizeMode))
				r.setWidth (item->preferredSize.x);

			if(!LayoutPrimitives::isSizeMode<IView::kAttachTop|IView::kAttachBottom> (sizeMode))
				r.setHeight (item->preferredSize.y);

			// check limits
			const SizeLimit& limits = item->sizeLimits;
			LayoutPrimitives::checkMaxSize<HorizontalDirection> (r, limits);
			LayoutPrimitives::checkMaxSize<VerticalDirection>   (r, limits);

			if(sizeMode != 0)
			{
				if(tableLayout.getCellRatio () != 0 && LayoutPrimitives::isSizeMode<IView::kAttachAll> (sizeMode))
				{
					// reduce either width or height (from full cell rect) to reach the given cell ratio
					// todo: result could conflict with min. size
					Coord width = coordFToInt (r.getHeight () * tableLayout.getCellRatio ());
					if(width < r.getWidth ())
						r.setWidth (width);
					else
					{
						Coord height = coordFToInt (r.getWidth () / tableLayout.getMinCellRatio ());
						if(height < r.getHeight ())
							r.setHeight (height);
					}
					sizeMode = IView::kHCenter|IView::kVCenter;
				}

				// use sizeMode flags for alignment in cell
				if((sizeMode & (IView::kAttachLeft|IView::kAttachRight)) == IView::kAttachRight)
					r.offset (cell.right - r.right, 0);
				else if(sizeMode & IView::kHCenter)
					r.centerH (cell);

				if((sizeMode & (IView::kAttachTop|IView::kAttachBottom)) == IView::kAttachBottom)
					r.offset (0, cell.bottom - r.bottom);
				else if(sizeMode & IView::kVCenter)
					r.centerV (cell);
			}
			view->setSize (r);
		}
		iterator.next ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TableLayoutAlgorithm::calcSizeLimits (SizeLimit& limits)
{
	limits.setUnlimited ();
	
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());
	
	LayoutPrimitives::accumulateSizeLimits<HorizontalDirection> (limits, colItems, margin, spacing);
	LayoutPrimitives::accumulateSizeLimits<VerticalDirection>   (limits, rowItems, margin, spacing);

	#if DEBUG_LOG
	CCL_PRINTF ("TableLayout::getSizelimits: width: %d, %d    height: %d, %d\n", limits.minWidth, limits.maxWidth, limits.minHeight, limits.maxHeight);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TableLayoutAlgorithm::onChildSized (View* child, const Point& delta)
{
	AnchorLayoutItem* item = static_cast<AnchorLayoutItem*> (context->findLayoutItem (child));

	if(item)
	{
		item->updateSize ();
		item->updateSizeLimits ();

		onViewsChanged (); // recalc row / col items
	}

	if(isSizeMode (View::kHFitSize) || isSizeMode (View::kVFitSize))
		context->requestAutoSize ();
}
