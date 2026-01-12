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
// Filename    : ccl/gui/layout/boxlayout.cpp
// Description : BoxLayout (hbox, vbox)
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/layout/boxlayout.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/public/text/cstring.h"

#if DEBUG_LOG
#define DIR_CHAR (context->getStyle ().isCommonStyle (Styles::kVertical) ? 'V' : 'H')
#define PRINT_THIS { MutableCString s (context->getTitle ());	CCL_PRINTF ("%sBoxLayout<%c>%s", CCL_INDENT, DIR_CHAR, s.str ()) }
#else
#define PRINT_THIS
#endif

namespace CCL {

//************************************************************************************************
// BoxLayoutAlgorithm
//************************************************************************************************

template<typename Direction>
class BoxLayoutAlgorithm: public AnchorLayoutAlgorithm
{
public:
	BoxLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData);

	// LayoutAlgorithm
	void doLayout () override;
	void calcSizeLimits (SizeLimit& limits) override;
	void onChildSized (View* child, const Point& delta) override;
	void onSize (const Point& delta) override;
	AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) override;
	AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) override;

protected:
	virtual void constrainWorkRect (AnchorLayoutItem& item);
	void checkGroupItems (ObjectList& workItems);

private:
	void setOtherDirEndCoord (AnchorLayoutItem* item, Coord end);
};

//************************************************************************************************
// AdaptiveAlgorithm
//************************************************************************************************

template<typename Direction>
class AdaptiveAlgorithm: public BoxLayoutAlgorithm<Direction>
{
public:
	using BoxLayoutAlgorithm<Direction>::BoxLayoutAlgorithm;

	// BoxLayoutAlgorithm
	void constrainWorkRect (AnchorLayoutItem& item) override;
	void onSize (const Point& delta) override;
	void constrainSize (Rect& rect) override;

private:
	Point layoutInternal (RectRef containerSize);
};

//************************************************************************************************
// RigidBoxLayoutAlgorithm
//** Does not change the sizes of the views, only arranges them. */
//************************************************************************************************

template<typename Direction>
class RigidBoxesAlgorithm: public AnchorLayoutAlgorithm
{
public:
	RigidBoxesAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData);

	// LayoutAlgorithm
	void doLayout () override;
	void calcSizeLimits (SizeLimit& limits) override;
	void onChildSized (View* child, const Point& delta) override;
	AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) override;
	AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) override;

private:
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
inline void calcMaxItemEndCoord (AnchorLayoutContext* context, Point& preferredSize)
{
	Coord& max = Direction::getCoord (preferredSize);
	max = 0;
	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		ccl_lower_limit (max, Direction::getEndCoord (item->workRect));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void centerRect (RectRef parentRect, Rect& r)
{
	Coord len = Direction::getLength (r);
	Coord containerLen = Direction::getLength (parentRect);
	if(len < containerLen)
		Direction::getStartCoord (r) = (containerLen - len) / 2;
	else
		Direction::getStartCoord (r) = 0; // align left if container is too small

	Direction::setLength (r, len);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void centerView (RectRef parentRect, AnchorLayoutItem* item)
{
	centerRect<Direction> (parentRect, item->workRect);
	item->getView ()->setSize (item->workRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void resizeContainer (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
{
	tbool fitH = (context->getSizeMode () & View::kHFitSize) != 0;
	tbool fitV = (context->getSizeMode () & View::kVFitSize) != 0;
	if(fitH || fitV)
		context->requestAutoSize (fitH, fitV);
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// BoxLayout
//************************************************************************************************

BEGIN_STYLEDEF (BoxLayout::customStyles)
	{"reverse",			Styles::kLayoutReverse},
	{"unifysizes",		Styles::kLayoutUnifySizes},
	{"wrap",			Styles::kLayoutWrap},
	{"hidepriority",	Styles::kLayoutHidePriority},
	{"no-minlimit",		Styles::kLayoutNoMinLimit},
	{"adaptive",		Styles::kLayoutAdaptive},
	{"commonbasesize",	Styles::kLayoutCommonBaseSize},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (BoxLayout, AnchorLayout)

//////////////////////////////////////////////////////////////////////////////////////////////////

BoxLayout::BoxLayout ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* BoxLayout::getCustomStyles () const
{
	return BoxLayout::customStyles;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* BoxLayout::createAlgorithm (LayoutContext* context)
{
	auto* anchorLayoutContext = ccl_cast<AnchorLayoutContext> (context);
	ASSERT (anchorLayoutContext)
	if(!anchorLayoutContext)
		return nullptr;

	StyleRef style = anchorLayoutContext->getStyle ();
	if(style.isCustomStyle (Styles::kLayoutAdaptive))
	{
		if(style.isCommonStyle (Styles::kVertical))
			return NEW AdaptiveAlgorithm<VerticalDirection> (anchorLayoutContext, layoutData);
		else
			return NEW AdaptiveAlgorithm<HorizontalDirection> (anchorLayoutContext, layoutData);
	}

	bool rigidBoxes = true;
	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		if(item->getView ()->getSizeMode () != 0)
		{
			rigidBoxes = false;
			break;
		}
	EndFor

	if(style.custom & (Styles::kLayoutUnifySizes|Styles::kLayoutWrap|Styles::kLayoutHidePriority))
		rigidBoxes = false;

	if(rigidBoxes)
	{
		if(style.isCommonStyle (Styles::kVertical))
			return NEW RigidBoxesAlgorithm<VerticalDirection> (anchorLayoutContext, layoutData);
		else
			return NEW RigidBoxesAlgorithm<HorizontalDirection> (anchorLayoutContext, layoutData);
	}
	else
	{
		if(style.isCommonStyle (Styles::kVertical))
			return NEW BoxLayoutAlgorithm<VerticalDirection> (anchorLayoutContext, layoutData);
		else
			return NEW BoxLayoutAlgorithm<HorizontalDirection> (anchorLayoutContext, layoutData);
	}
}

//************************************************************************************************
// BoxLayoutAlgorithm
//************************************************************************************************

template <typename Direction>
BoxLayoutAlgorithm<Direction>::BoxLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
: AnchorLayoutAlgorithm (context, layoutData)
{
	Coord doubleMargin = 2 * coordFToInt (layoutData.margin * context->getZoomFactor ());
	preferredSize (doubleMargin, doubleMargin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::constrainWorkRect (AnchorLayoutItem& item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::setOtherDirEndCoord (AnchorLayoutItem* item, Coord end)
{
	Rect& r = item->workRect;
	Direction::OtherDirection::getEndCoord (r) = end;
	LayoutPrimitives::checkMaxSize<typename Direction::OtherDirection> (r, item->sizeLimits);
	item->getView ()->setSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* BoxLayoutAlgorithm<Direction>::onViewAdded (int index, AnchorLayoutItem* item)
{
	item->workRect = item->getInitialSize ();
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Direction::OtherDirection::offset (item->workRect, margin); // other dir: add margin

	ObjectArray& layoutItems = context->getLayoutItems ();

	// update total preferred size (main direction)
	Coord spacing = layoutItems.count () > 1 ? coordFToInt (layoutData.spacing * context->getZoomFactor ()) : 0;
	Direction::getCoord (preferredSize) += Direction::getCoord (item->preferredSize) + spacing;

	Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
	preferredOtherSize -= margin;
	
	if(context->getStyle ().isCustomStyle (Styles::kLayoutUnifySizes))
	{
		// check if end coord in other direction is greater than other views so far 
		Coord end = Direction::OtherDirection::getEndCoord (item->workRect);
		Coord& commonEnd = Direction::OtherDirection::getCoord (preferredSize);
		if(end > commonEnd)
		{
			commonEnd = end;

			// apply the new end coord to all views
			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				setOtherDirEndCoord (item, end);
			EndFor
		}
		else
			setOtherDirEndCoord (item, commonEnd); // enlarge new item to others end coord
	}
	else
	{
		// init workRect coords of other direction
		//Rect& r = item->workRect;
		//const Rect& viewRect = item->getView ()->getSize ();
		//Direction::OtherDirection::getStartCoord (r) = Direction::OtherDirection::getStartCoord (viewRect);
		//Direction::OtherDirection::getEndCoord   (r) = Direction::OtherDirection::getEndCoord   (viewRect);

		// check center mode
		if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
		{
			centerView<typename Direction::OtherDirection> (context->getLayoutRect (), item);

			ccl_lower_limit (Direction::OtherDirection::getCoord (preferredSize), margin + Direction::OtherDirection::getCoord (item->preferredSize));
		}
		else
		{
			// update total preferred size in other direction
			ccl_lower_limit (Direction::OtherDirection::getCoord (preferredSize), Direction::OtherDirection::getEndCoord (item->workRect));
		}
	}

	preferredOtherSize += margin;
	resizeContainer (context, layoutData);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* BoxLayoutAlgorithm<Direction>::onViewRemoved (AnchorLayoutItem* item)
{
	ObjectArray& layoutItems = context->getLayoutItems ();

	// update total preferred size
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = layoutItems.isEmpty () ? 0 : coordFToInt (layoutData.spacing * context->getZoomFactor ());

	Coord removedLength = Direction::getCoord (item->preferredSize) + spacing;
	if(Direction::getCoord (preferredSize) >= removedLength)
		Direction::getCoord (preferredSize) -= removedLength;

	Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
	preferredOtherSize -= 2 * margin;

	calcMaxItemEndCoord<typename Direction::OtherDirection> (context, preferredSize);

	if(context->getStyle ().isCustomStyle (Styles::kLayoutUnifySizes))
	{
		// check if the removed view had the greatest end coord in other direction
		Coord end = Direction::OtherDirection::getEndCoord (item->getView ()->getSize ());
		Coord& commonEnd = Direction::OtherDirection::getCoord (preferredSize);
		if(end == commonEnd)
		{
			// recalc the greatest end coord of remaining views
			Coord end = 0;
			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				ccl_lower_limit (end, Direction::OtherDirection::getEndCoord (item->getView ()->getSize ()));
			EndFor

			// apply the new end coord to all views
			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				setOtherDirEndCoord (item, end);
			EndFor
			commonEnd = end;
		}
	}

	preferredOtherSize += 2 * margin;

	// we modify the view's size below, so the wrong area might finally be invalidated in View::removeView
	// - we could check first if this additional invalidate is really required (only if not newRect.rectInside (oldRect))
	// - but that seems very likely anyway, since the removed view is not only sized, but also moved to 0,0, see constructor Rect (PointRef) below
	// - on the other hand, that might not be intended and could be changed (keeping the position), so a check would make more sense
	item->getView ()->invalidate ();
	
	// let the view take it's preferred size as souvenir, it might be added again
	item->getView ()->setSize (item->preferredSize, false);

	// todo: could switch to RigidBoxesAlgorithm if now appropriate
	resizeContainer (context, layoutData);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::onChildSized (View* child, const Point& delta)
{
	#if DEBUG_LOG
	MutableCString s (child->getTitle ());
	if(s.isEmpty ())
		s = child->getName ();
	PRINT_THIS CCL_PRINTF ("::onChildSized (%s, %d, %d)\n", s.str (), delta.x, delta.y);
	CCL_ADD_INDENT (2)
	#endif

	auto* item = static_cast<AnchorLayoutItem*> (context->findLayoutItem (child));
	if(item)
	{
		Direction::getCoord (preferredSize) -= Direction::getCoord (item->preferredSize);
		item->updatePreferredSize ();
		Direction::getCoord (preferredSize) += Direction::getCoord (item->preferredSize);

		if(context->getStyle ().isCustomStyle (Styles::kLayoutCommonBaseSize))
		{
			ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, otherItem)
				if(otherItem->preferredSize != item->preferredSize) // (includes condition otherItem != item)
				{
					Direction::getCoord (preferredSize) -= Direction::getCoord (otherItem->preferredSize);
					otherItem->preferredSize = item->preferredSize;
					Direction::getCoord (preferredSize) += Direction::getCoord (otherItem->preferredSize);
				}
			EndFor
		}

		ccl_lower_limit (Direction::getCoord (preferredSize), 0);

		item->updateSizeLimits (); // (after updatePreferredSize!)
		
		Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
		Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
		preferredOtherSize -= margin;

		item->setInitialSize (item->getView ()->getSize ());

		// for a child that was centered by us in other direction (see below), take back a previous centering first for correct calculation of total preferredSize
		if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
			Direction::OtherDirection::moveTo (item->getInitialSize (), margin);

		item->workRect = item->getInitialSize ();

		if(Direction::OtherDirection::getCoord (delta))
			calcMaxItemEndCoord<typename Direction::OtherDirection> (context, preferredSize);

		if(context->getStyle ().isCustomStyle (Styles::kLayoutUnifySizes))
		{
			ObjectArray& layoutItems = context->getLayoutItems ();

			Coord otherDirDelta = Direction::OtherDirection::getCoord (delta);
			if(otherDirDelta)
			{
				// calc the greatest end coord of other views
				Coord commonEnd = 0;
				ArrayForEachFast (layoutItems, AnchorLayoutItem, i)
					if(i != item)
						ccl_lower_limit (commonEnd, Direction::OtherDirection::getEndCoord (i->getView ()->getSize ()));
				EndFor

				Coord newEnd = Direction::OtherDirection::getEndCoord (item->workRect);
				if(newEnd > commonEnd)
				{
					// sized item is larger than others, try to apply the new end coord to all views
					ArrayForEachFast (layoutItems, AnchorLayoutItem, i)
						setOtherDirEndCoord (i, newEnd);
					EndFor
				}
				else if(newEnd < commonEnd)
				{
					// sized item is smaller than others, find largest preferred size of others, and try to apply this to all
					Coord commonEnd = newEnd;
					ArrayForEachFast (layoutItems, AnchorLayoutItem, i)
						if(i != item && !(i->getView ()->getSizeMode () & View::kPreferCurrentSize))
							ccl_lower_limit (commonEnd, Direction::OtherDirection::getCoord (i->preferredSize));
					EndFor
					ArrayForEachFast (layoutItems, AnchorLayoutItem, i)
						if(i != item)
							setOtherDirEndCoord (i, commonEnd);
					EndFor
				}
			}

			// recalc preferrred size from preferred sizes & offsets of items
			Coord prefEnd = 0;
			ArrayForEachFast (layoutItems, AnchorLayoutItem, i)
				if(i != item && (i->getView ()->getSizeMode () & View::kPreferCurrentSize))
					continue;

				ccl_lower_limit (prefEnd,
								Direction::OtherDirection::getStartCoord (i->workRect) +
								Direction::OtherDirection::getCoord (i->preferredSize));
			EndFor

			preferredOtherSize = prefEnd; // margin will be added below
		}
		preferredOtherSize += margin;

		// check the child's center option in other direction
		if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
			centerView<typename Direction::OtherDirection> (context->getLayoutRect (), item);
	}

	if(isSizeMode (View::kHFitSize) || isSizeMode (View::kVFitSize))
	{
		if(item)
			item->updateSize ();// todo: still needed? already done above

		resizeContainer (context, layoutData);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::onSize (const Point& delta)
{
	PRINT_THIS CCL_PRINTF ("::onSize (%d, %d)  -> now %d x %d\n", delta.x, delta.y, context->getLayoutRect ().getWidth (), context->getLayoutRect ().getHeight ());
	CCL_ADD_INDENT (2)

	Coord otherDirDelta = Direction::OtherDirection::getCoord (delta);
	if(otherDirDelta)
	{
		// handle the usual attachment of subviews in other direction
		ObjectArray& layoutItems = context->getLayoutItems ();

		if(context->isSizeModeDisabled ())
		{
			// center must be handled always
			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
					centerView<typename Direction::OtherDirection> (context->getLayoutRect (), item);
			EndFor
		}
		else
		{
			Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
			Coord TEST = preferredOtherSize;
			Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
			preferredOtherSize -= 2 * margin;

			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				item->updateSizeLimits ();
				Rect& r = item->workRect;

				int a = item->getView ()->getSizeMode ();
				if(a)
				{
					if((a & Direction::OtherDirection::kAttachStart) && (a & Direction::OtherDirection::kAttachEnd))
					{
						Direction::OtherDirection::getEndCoord (r) += otherDirDelta;
						constrainWorkRect (*item); // derived class can adjust workRect
					}
					else if((a & Direction::OtherDirection::kAttachEnd) != 0)
						Direction::OtherDirection::offset (r, otherDirDelta);
					else if(a & Direction::OtherDirection::kCenter)
					{
						Coord len = Direction::OtherDirection::getLength (r);
						Direction::OtherDirection::getStartCoord (r) = (Direction::OtherDirection::getLength (context->getLayoutRect ()) - len) / 2;
						Direction::OtherDirection::setLength (r, len);
					}

					#if 0 // this resizes views that shouldn't resize (#5922) and seems to break the genral attachment rules:
					ccl_upper_limit (Direction::OtherDirection::getEndCoord (r), Direction::OtherDirection::getLength (context->getLayoutRect ()));
					#endif
				}
				Rect limited (r);
				LayoutPrimitives::checkMaxSize<typename Direction::OtherDirection> (limited, item->sizeLimits);
				LayoutPrimitives::checkMinSize<typename Direction::OtherDirection> (limited, item->sizeLimits);
				item->getView ()->setSize (limited);
			EndFor

			preferredOtherSize += 2 * margin;
			ASSERT (TEST == preferredOtherSize)
		}
	}

	// wrap mode: resize in other direction
	if(context->getStyle ().isCustomStyle (Styles::kLayoutWrap))
		if(isSizeMode (Direction::OtherDirection::kFitSize))
			context->requestAutoSize (Direction::OtherDirection::isHorizontal (), Direction::OtherDirection::isVertical ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::doLayout ()
{
	ObjectArray& layoutItems = context->getLayoutItems ();
	ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
		item->updateSizeLimits ();
	EndFor

	Coord available = Direction::getLength (context->getLayoutRect ());
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());

	if(context->getStyle ().isCustomStyle (Styles::kLayoutWrap))
	{
		Coord rowStart = margin;
		for(int i = 0, numItems = layoutItems.count (); i < numItems;)
		{
			AnchorLayoutItem* item = static_cast<AnchorLayoutItem*> (layoutItems.at (i));
			Coord minRequired = 2 * margin + Direction::getMin (item->sizeLimits);
			Coord rowLength = 0;

			ObjectList workItems;
			while(1)
			{
				workItems.add (item);
				ccl_lower_limit (rowLength, Direction::OtherDirection::getLength (item->getView ()));

				// advance to next item
				item = static_cast<AnchorLayoutItem*> (layoutItems.at (++i));
				if(!item)
					break;

				// check if next item still fits in current row
				minRequired += Direction::getMin (item->sizeLimits) + spacing;
				if(minRequired > available)
					break;
			}

			// layout current row
			LayoutPrimitives::calcBoxLayout<Direction> (workItems, available, margin, spacing);
			ForEach (workItems, AnchorLayoutItem, item)
				Direction::OtherDirection::moveTo (item->workRect, rowStart);
			EndFor

			rowStart += (rowLength + spacing);
		}
		Direction::OtherDirection::getCoord (preferredSize) = ccl_max (0, rowStart - spacing + margin);
	}
	else if(context->getStyle ().isCustomStyle (Styles::kLayoutHidePriority))
	{
		ObjectList workItems;
		ObjectList hiddenItems;

		ObjectArray& layoutItems = context->getLayoutItems ();
		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			workItems.add (item);

			if(item->isHidden () && item->getView ()->getSizeMode () & (View::kFitSize)) // hidden view might have resized meanwhile (detached from LayoutView -> onChildSized not called)
			{
				item->updateSize ();
				item->updateSizeLimits ();
			}
		EndFor

		// calc minimum required size (todo: cache this)
		SizeLimit limits;
		limits.setUnlimited ();
		LayoutPrimitives::accumulateSizeLimits<Direction> (limits, layoutItems, margin, spacing);
		Coord required = Direction::getMin (limits);

		while(required > available)
		{
			// find item with lowest prio (to optimize: keep a list sorted by priority?)
			int lowestPrio = NumericLimits::kMaxInt;
			AnchorLayoutItem* lowestPrioItem = nullptr;

			ForEachReverse (workItems, AnchorLayoutItem, item)
				if(item->priority < lowestPrio && item->priority >= 0)
				{
					lowestPrioItem = item;
					lowestPrio = item->priority;
				}
			EndFor

			if(!lowestPrioItem)
				break;

			required -= Direction::getMin (lowestPrioItem->sizeLimits) + spacing;
			hiddenItems.add (lowestPrioItem);
			workItems.remove (lowestPrioItem);
		}

		// check grouping elements
		checkGroupItems (workItems);

		// layout the visible items
		LayoutPrimitives::calcBoxLayout<Direction> (workItems, available, margin, spacing);

		ForEach (hiddenItems, AnchorLayoutItem, item)
			context->hideItem (item);
		EndFor

		ForEach (workItems, AnchorLayoutItem, item)
			context->showItem (item);
		EndFor
	}
	else
	{
		// calculate layout in main direction
		LayoutPrimitives::calcBoxLayout<Direction> (layoutItems, available, margin, spacing);
	}

	// apply the calculated sizes
	bool mustRecalcPreferredSize = false;
	ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
		constrainWorkRect (*item);
		item->getView ()->setSize (item->workRect);

		if(item->getView ()->getSizeMode () & IView::kPreferCurrentSize)
		{
			mustRecalcPreferredSize = true;
			item->updatePreferredSize ();
		}
	EndFor

	if(mustRecalcPreferredSize)
	{
		// some views "like" any size we give them (see kPreferCurrentSize above), recalc our own preferred size then
		Direction::getCoord (preferredSize) = 2 * margin + (layoutItems.count () - 1) * spacing;

		Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
		preferredOtherSize = 0;

		ForEach (context->getLayoutItems (), AnchorLayoutItem, item)
			Direction::getCoord (preferredSize) += Direction::getCoord (item->preferredSize);
			if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter) // child was centered by us (in other direction)
				ccl_lower_limit (preferredOtherSize, Direction::OtherDirection::getLength (item->workRect));
			else
				ccl_lower_limit (preferredOtherSize, Direction::OtherDirection::getEndCoord (item->workRect));
		EndFor
		preferredOtherSize += margin;
		ccl_lower_limit (preferredOtherSize, 2 * margin);
	}

	#if DEBUG_LOG
	PRINT_THIS CCL_PRINT (":")
	Point total;
	ArrayForEachFast (layoutItems, LayoutItem, item)
		Point size (item->getView ()->getSize ().getWidth (), item->getView ()->getSize ().getHeight ());
		Direction::getCoord (total) += Direction::getCoord (size);
		ccl_lower_limit (Direction::OtherDirection::getCoord (total), Direction::OtherDirection::getCoord (size));
		CCL_PRINTF ("%c (%d,%d)", item == layoutItems.at (0)?' ':',', size.x, size.y);
	EndFor
	CCL_PRINTF ("  => total: (%d,%d) container: (%d, %d)", total.x, total.y, context->getLayoutWidth (), context->getLayoutHeight ());
	CCL_PRINTLN ("")
	#endif
	
	resizeContainer (context, layoutData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::checkGroupItems (ObjectList& workItems)
{
	AnchorLayoutItem* groupStartItem = nullptr;
	bool needsGroupStart = true;

	ForEach (workItems, AnchorLayoutItem, item)
		if(item->isGroupDecorItem ())
		{
			if(needsGroupStart)
			{
				groupStartItem = item;
				needsGroupStart = false;
			}
			else
			{
				if(groupStartItem)
				{
					// Hide both start and end if they are neighbors
					if(View* view = groupStartItem->getView ())
						view->isHidden (true);
					if(View* view = item->getView ())
						view->isHidden (true);
					groupStartItem = nullptr;
				}
				else
				{
					// Show end item if real item was predecessor
					if(View* view = item->getView ())
						view->isHidden (false);
				}
				needsGroupStart = true;
			}
		}
		else
		{
			// Show startItem if a real item is found after group start
			if(groupStartItem && Direction::getLength (item->workRect) > ccl_abs (layoutData.spacing))
			{
				if(View* view = groupStartItem->getView ())
					view->isHidden (false);
				groupStartItem = nullptr;
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void BoxLayoutAlgorithm<Direction>::calcSizeLimits (SizeLimit& limits)
{
	ObjectArray& layoutItems = context->getLayoutItems ();

	limits.setUnlimited ();
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());
	
	int sizeMode = context->getSizeMode ();

	// main direction
	if((sizeMode & (Direction::kFitSize|Direction::kAttachStart|Direction::kAttachEnd)) == Direction::kFitSize)
		LayoutPrimitives::setFixedLength<Direction> (limits, LayoutPrimitives::getMaxCoord<Direction> (layoutItems) + margin);
	else
		LayoutPrimitives::accumulateSizeLimits<Direction> (limits, layoutItems, margin, spacing);

	if(context->getStyle ().isCustomStyle (Styles::kLayoutHidePriority))
		Direction::getMin (limits) = 2 * margin; // all views can be hidden
	else if(context->getStyle ().isCustomStyle (Styles::kLayoutWrap))
	{
		// largest minimum item size
		Coord min = 0;
		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			ccl_lower_limit (min, Direction::getMin (item->sizeLimits));
		EndFor
		Direction::getMin (limits) = 2 * margin + min;
	}

	if(context->getStyle ().isCustomStyle (Styles::kLayoutNoMinLimit))
		Direction::getMin (limits) = 0;

	// other direction
	if((sizeMode & (Direction::OtherDirection::kFitSize|Direction::OtherDirection::kAttachStart|Direction::OtherDirection::kAttachEnd)) == Direction::OtherDirection::kFitSize)
		LayoutPrimitives::setFixedLength<typename Direction::OtherDirection> (limits, LayoutPrimitives::getMaxCoord<typename Direction::OtherDirection> (layoutItems) + margin);
	else if(context->getStyle ().isCustomStyle (Styles::kLayoutWrap))
	{
		Direction::OtherDirection::getMin (limits) = Direction::OtherDirection::getCoord (preferredSize);
		Direction::OtherDirection::getMax (limits) = kMaxCoord;
	}
	else
	{
		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			LayoutPrimitives::joinSubViewLimits<typename Direction::OtherDirection> (context->getLayoutRect (), limits, item->getView (), margin);
		EndFor
	}

	#if DEBUG_LOG
	PRINT_THIS CCL_PRINTF ("::calcSizeLimits:  H (%d -- %d)  V (%d -- %d)\n", limits.minWidth, limits.maxWidth, limits.minHeight, limits.maxHeight);
	#endif
}

//************************************************************************************************
// AdaptiveAlgorithm
//************************************************************************************************

template<typename Direction>
void AdaptiveAlgorithm<Direction>::constrainWorkRect (AnchorLayoutItem& item)
{
	item.getView ()->constrainSize (item.workRect);

	if(item.getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
	{
		// center view if constrained size in other direction is smaller than layout container
		Coord viewLength = Direction::OtherDirection::getLength (item.workRect);
		Coord containerLength = Direction::OtherDirection::getLength (this->context->getLayoutRect ());
		if(viewLength < containerLength && viewLength > 0)
		{
			Direction::OtherDirection::getStartCoord (item.workRect) = (containerLength - viewLength) / 2;
			Direction::OtherDirection::setLength (item.workRect, viewLength);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void AdaptiveAlgorithm<Direction>::onSize (const Point& delta)
{
	ObjectArray& layoutItems = this->context->getLayoutItems ();

	if(this->context->isSizeModeDisabled ())
	{
		// center must be handled always
		for(auto item : iterate_as<AnchorLayoutItem> (layoutItems))
			if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
				centerView<typename Direction::OtherDirection> (this->context->getLayoutRect (), item);
	}
	else
	{
		layoutInternal (this->context->getLayoutRect ());

		for(auto item : iterate_as<AnchorLayoutItem> (layoutItems))
		{
			item->updateSizeLimits ();
			constrainWorkRect (*item);

			Rect limited (item->workRect);
			LayoutPrimitives::checkMaxSize<typename Direction::OtherDirection> (limited, item->sizeLimits);
			LayoutPrimitives::checkMinSize<typename Direction::OtherDirection> (limited, item->sizeLimits);
			item->getView ()->setSize (limited);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
Point AdaptiveAlgorithm<Direction>::layoutInternal (RectRef containerSize)
{
	ObjectArray& layoutItems = this->context->getLayoutItems ();
	Coord margin = coordFToInt (this->layoutData.margin * this->context->getZoomFactor ());
	Coord spacing = coordFToInt (this->layoutData.spacing * this->context->getZoomFactor ());

	Coord available = Direction::getLength (containerSize);
	Coord otherAvailable = Direction::OtherDirection::getLength (containerSize) - 2 * margin;

	LayoutPrimitives::calcBoxLayout<Direction> (layoutItems, available, margin, spacing);
			
	Point totalDiff;
	for(auto item : iterate_as<AnchorLayoutItem> (layoutItems))
	{
		item->updateSizeLimits ();

		Rect offeredSize (item->workRect);
		if(this->context->getStyle ().isCustomStyle (Styles::kLayoutUnifySizes)
			&& LayoutPrimitives::isSizeMode<Direction::OtherDirection::kAttachStart|Direction::OtherDirection::kAttachEnd> (item->getView ())) // (is sizable)
		{
			Direction::OtherDirection::getStartCoord (offeredSize) = margin;
			Direction::OtherDirection::setLength (offeredSize, otherAvailable);
		}

		Rect constrainedSize (offeredSize);
		item->getView ()->constrainSize (constrainedSize);
		if(constrainedSize != offeredSize)
		{
			Point diff (constrainedSize.getSize () - offeredSize.getSize ());
			totalDiff += diff;
			CCL_PRINTF ("%sAdaptiveLayout: %d x %d -> %d x %d (diff: %d, %d)\n", CCL_INDENT, offeredSize.getWidth (), offeredSize.getHeight (), constrainedSize.getWidth (), constrainedSize.getHeight (), diff.x, diff.y)

			item->sizeLimits.setUnlimited ();
		}

		if(item->getView ()->getSizeMode () & Direction::OtherDirection::kCenter)
			centerRect<typename Direction::OtherDirection> (this->context->getLayoutRect (), constrainedSize);

		item->workRect = constrainedSize;
	}

	return totalDiff;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void AdaptiveAlgorithm<Direction>::constrainSize (Rect& rect)
{
	Point diff = layoutInternal (rect);
	rect.setSize (rect.getSize () + diff);
}

//************************************************************************************************
// RigidBoxesAlgorithm
//************************************************************************************************

template <typename Direction>
RigidBoxesAlgorithm<Direction>::RigidBoxesAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
: AnchorLayoutAlgorithm (context, layoutData)
{
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Direction::getCoord (preferredSize) = 2 * margin;
	Direction::OtherDirection::getCoord (preferredSize) = 2 * margin;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* RigidBoxesAlgorithm<Direction>::onViewAdded (int index, AnchorLayoutItem* item)
{
	if(item->getView ()->getSizeMode () != 0)
		return NEW BoxLayoutAlgorithm<Direction> (context, layoutData);

	// update total preferrred size
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = context->getLayoutItems ().count () > 1 ? coordFToInt (layoutData.spacing * context->getZoomFactor ()) : 0;
	Direction::getCoord (preferredSize) += Direction::getCoord (item->preferredSize) + spacing;

	Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
	preferredOtherSize -= 2 * margin;
	ccl_lower_limit (Direction::OtherDirection::getCoord (preferredSize), Direction::OtherDirection::getEndCoord (item->getInitialSize ()));
	preferredOtherSize += 2 * margin;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* RigidBoxesAlgorithm<Direction>::onViewRemoved (AnchorLayoutItem* item)
{
	ObjectArray& layoutItems = context->getLayoutItems ();

	// update total preferred size
	Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
	Coord spacing = layoutItems.isEmpty () ? 0 : coordFToInt (layoutData.spacing * context->getZoomFactor ());
	Direction::getCoord (preferredSize) -= Direction::getCoord (item->preferredSize) + spacing;

	Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
	preferredOtherSize -= 2 * margin;
	calcMaxItemEndCoord<typename Direction::OtherDirection> (context, preferredSize);
	preferredOtherSize += 2 * margin;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void RigidBoxesAlgorithm<Direction>::onChildSized (View* child, const Point& delta)
{
	auto* item = static_cast<AnchorLayoutItem*> (context->findLayoutItem (child));
	if(item)
	{
		item->updateSizeLimits ();

		Direction::getCoord (preferredSize) -= Direction::getCoord (item->preferredSize);
		item->updatePreferredSize ();
		Direction::getCoord (preferredSize) += Direction::getCoord (item->preferredSize);

		item->setInitialSize (item->getView ()->getSize ());

		Coord margin = coordFToInt (layoutData.margin * context->getZoomFactor ());
		Coord& preferredOtherSize = Direction::OtherDirection::getCoord (preferredSize);
		preferredOtherSize -= margin;
		if(Direction::OtherDirection::getCoord (delta))
			calcMaxItemEndCoord<typename Direction::OtherDirection> (context, preferredSize);
		preferredOtherSize += margin;
	}

	if(isSizeMode (View::kHFitSize) || isSizeMode (View::kVFitSize))
	{
		if(item)
			item->updateSize ();

		context->requestAutoSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void RigidBoxesAlgorithm<Direction>::doLayout ()
{
	ObjectArray& layoutItems = context->getLayoutItems ();
	StyleRef style (context->getStyle ());
	Coord spacing = coordFToInt (layoutData.spacing * context->getZoomFactor ());
	Coord otherDirStart = coordFToInt (layoutData.margin * context->getZoomFactor ());

	if(style.isCustomStyle (Styles::kLayoutReverse))
	{
		Coord endPos;
		if(isSizeMode (Direction::kFitSize))
		{
			endPos = coordFToInt (layoutData.margin * context->getZoomFactor ());
			ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
				endPos += Direction::getLength (item->getView ()) + spacing;
			EndFor
			endPos -= spacing;
		}
		else
			endPos = Direction::getLength (context->getLayoutRect ());

		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			View* v = item->getView ();
			Rect r (v->getSize ());
			Coord length = Direction::getLength (r);
			Direction::moveTo (r, endPos - length);
			if(v->getSizeMode () & Direction::OtherDirection::kCenter)
				Direction::OtherDirection::moveTo (r, (Direction::OtherDirection::getLength (context->getLayoutRect ()) - Direction::OtherDirection::getLength (r)) / 2);
			else
				Direction::OtherDirection::moveTo (r, otherDirStart);
			v->setSize (r);
			endPos -= length + spacing;
		EndFor
	}
	else
	{
		Coord pos = coordFToInt (layoutData.margin * context->getZoomFactor ());
		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			View* v = item->getView ();
			Rect r (v->getSize ());
			Direction::moveTo (r, pos);
			if(v->getSizeMode () & Direction::OtherDirection::kCenter)
				Direction::OtherDirection::moveTo (r, (Direction::OtherDirection::getLength (context->getLayoutRect ()) - Direction::OtherDirection::getLength (r)) / 2);
			else
				Direction::OtherDirection::moveTo (r, otherDirStart);
			v->setSize (r);
			pos += Direction::getLength (r) + spacing;
		EndFor
	}

	resizeContainer (context, layoutData);

	#if DEBUG_LOG
	CCL_PRINTF ("RigidBoxes<%c>:", DIR_CHAR);
	Point total;
	ArrayForEachFast (layoutItems, LayoutItem, item)
		Point size (item->getView ()->getSize ().getSize ());
		Direction::getCoord (total) += Direction::getCoord (size);
		ccl_lower_limit (Direction::OtherDirection::getCoord (total), Direction::OtherDirection::getCoord (size));
		CCL_PRINTF ("%c (%d,%d)", item == layoutItems.at (0)?' ':',', size.x, size.y);
	EndFor
	CCL_PRINTF ("  => total: (%d,%d) container: (%d, %d)", total.x, total.y, context->getLayoutWidth (), context->getLayoutHeight ());
	CCL_PRINTLN ("")
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void RigidBoxesAlgorithm<Direction>::calcSizeLimits (SizeLimit& limits)
{
	limits.minWidth = limits.maxWidth = preferredSize.x;
	limits.minHeight = limits.maxHeight = preferredSize.y;
}
