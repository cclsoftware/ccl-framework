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
// Filename    : ccl/gui/layout/layoutprimitives.h
// Description : Layout primitives
//
//************************************************************************************************

#ifndef _ccl_layoutprimitives_h
#define _ccl_layoutprimitives_h

#include "ccl/gui/layout/anchorlayout.h"

namespace CCL {

//************************************************************************************************
// LayoutPrimitives
//************************************************************************************************

namespace LayoutPrimitives
{
	/// include the SizeLimits of subView into parentLimits, taking the attachment into account.
void joinSubViewLimits (RectRef parentSize, SizeLimit& parentLimits, View* subView);				   ///< for both directions
template <class Direction> void joinSubViewLimits (RectRef parentSize, SizeLimit& parentLimits, View* subView, Coord margin = 0); ///< for one direction
	
	/// calculate a child's sizeLimits from it's parent; used when the parent has fitsize mode
	template<class Direction> void calcSizeLimitsFromParent (SizeLimit& childLimits, const SizeLimit& parentLimits, Coord margins);

	/// calculate sizeLimits for a view that mainly displays a title
	bool calcTitleLimits (SizeLimit& sizeLimits, StringRef title, FontRef font, int sizeMode);
	bool calcTitleLimits (SizeLimit& sizeLimits, View* view);
	bool calcMultiLineLimits (SizeLimit& sizeLimits, View* view);
	bool calcMultiLineLimits (SizeLimit& sizeLimits, int lineWidth, StringRef title, FontRef font, int sizeMode);

	// resize child views according to attachments after parent was sized
	void resizeChildItems (ObjectArray& items, RectRef parentSize, const Point& delta, bool attachDisabled);
	void resizeChildViews (Container& childViews, RectRef parentSize, const Point& delta, bool attachDisabled);
	void resizeChild (View* view, RectRef parentSize, const Point& delta);
	void checkCenter (RectRef parentSize, View& child);

	/// arrange items in one direction, distributing the available space fairly
	template<class Direction> void calcBoxLayout (Container& layoutItems, Coord availableSpace, Coord margin, Coord spacing);

	/// sum up all size limits when items are arranged in one direction
	template<class Direction> void accumulateSizeLimits (SizeLimit& limits, ObjectArray& layoutItems, Coord margin, Coord spacing);

	/// sum up all preferred sizes when items are arranged in one direction
	template<class Direction> Coord accumulatePreferredSize (ObjectArray& layoutItems, Coord margin, Coord spacing);

	/// get the highest coord of all views in one direction
	template<class Direction> Coord getMaxCoord (ObjectArray& layoutItems);
	template<class Direction> Coord getMaxCoord (const View* parent);

	/// check size limits in each direction separately
	template<class Direction> void checkMinSize (Rect& rect, const SizeLimit& limits);
	template<class Direction> void checkMaxSize (Rect& rect, const SizeLimit& limits);
	template<class Direction> void checkMinSize (Point& p, const SizeLimit& limits);
	template<class Direction> void checkMaxSize (Point& p, const SizeLimit& limits);

	/// set size limits to a fixed length in one direction
	template<class Direction> void setFixedLength (SizeLimit& limits, Coord length);

	/// set size limits to only this view, never passing them down to childs
	void applySizeLimitsShallow (View& view, const SizeLimit& limits);
	
	/// check if all of the given sizemode flags are set
	template<int flags> bool isSizeMode (int sizeMode);
	template<int flags> bool isSizeMode (View* view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// LayoutPrimitives implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Direction>
void LayoutPrimitives::joinSubViewLimits (RectRef parentSize, SizeLimit& parentLimits, View* subView, Coord parentMargin)
{
	Coord minSize = 0;
	Coord maxSize = 0;
	switch (subView->getSizeMode () & (Direction::kAttachStart|Direction::kAttachEnd|Direction::kCenter))
	{
		case Direction::kAttachStart|Direction::kAttachEnd:
		case Direction::kAttachStart|Direction::kAttachEnd|Direction::kCenter:
		{
			// child gets resized with parent, margins stay fix
			const SizeLimit& childLimits = subView->getSizeLimits ();
			Coord marginL = Direction::getStartCoord (subView->getSize ());
			Coord marginR = ccl_max (0, Direction::getLength (parentSize) - Direction::getEndCoord (subView->getSize ()));
			ccl_lower_limit (marginL, parentMargin); // at least parentMargin
			ccl_lower_limit (marginR, parentMargin);
			Coord margins = marginL + marginR;
			minSize = Direction::getMin (childLimits) + margins;
			maxSize = Direction::getMax (childLimits) + margins;
			break;
		}
		case Direction::kAttachEnd:
		{
			// child gets never resized, but moves
			Coord marginR = ccl_max (0, Direction::getLength (parentSize) - Direction::getEndCoord (subView->getSize ()));
			ccl_lower_limit (marginR, parentMargin);
			minSize = Direction::getLength (subView) + marginR + parentMargin; // parentMargin at left
			maxSize = kMaxCoord;
			break;
		}
		case Direction::kCenter:
		{
			// child gets never resized, but centered
			minSize = Direction::getLength (subView) + 2 * parentMargin;
			maxSize = kMaxCoord;
			break;
		}
		default: // child gets never resized or moved
		{
			minSize = Direction::getEndCoord (subView->getSize ()) + parentMargin; // parentMargin at right
			maxSize = kMaxCoord;
			break;
		}
	}

	ccl_lower_limit (Direction::getMin (parentLimits), minSize);
	ccl_upper_limit (Direction::getMax (parentLimits), maxSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::calcSizeLimitsFromParent (SizeLimit& childLimits, const SizeLimit& parentLimits, Coord margins)
{
	// parent resizes to child, margins stay fix
	Direction::getMin (childLimits) = ccl_max<Coord> (Direction::getMin (parentLimits) - margins, 0);
	Direction::getMax (childLimits) = ccl_min<Coord> (Direction::getMax (parentLimits) - margins, kMaxCoord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::calcBoxLayout (Container& layoutItems, Coord availableSpace, Coord margin, Coord spacing)
{
	ObjectList workItems;

	// first reset all to their preferred sizes
	Coord totalPreferredSize = 0;
	float totalFill = 0;
	ForEach (layoutItems, AnchorLayoutItem, item)

		// limit preferred size
		checkMinSize<Direction> (item->preferredSize, item->sizeLimits);
		checkMaxSize<Direction> (item->preferredSize, item->sizeLimits);

		Coord preferredSize = Direction::getCoord (item->preferredSize);
		Direction::getStartCoord (item->workRect) = 0;
		Direction::getEndCoord (item->workRect) = preferredSize;

		totalPreferredSize += preferredSize;
		if(item->fillFactor > 0)
			totalFill += item->fillFactor;
		workItems.add (item);
	EndFor

	bool hasFill = (totalFill > 0);
	Coord usedSpace =  2 * margin + totalPreferredSize + (layoutItems.count () - 1) * spacing;

	// distribute additional or missing space among the views, respecting minimum and maximum sizes
	bool mustShrink = usedSpace > availableSpace;

	while(usedSpace != availableSpace && !workItems.isEmpty ())
	{
		float diffSpace = float(availableSpace - usedSpace);
		float distributeFactor;
		if(hasFill)
			distributeFactor = diffSpace / totalFill;
		else
			distributeFactor = (totalPreferredSize == 0) ? 1.f : diffSpace / float (totalPreferredSize);

		ListForEachObject (workItems, AnchorLayoutItem, item)

			// determine how much this item grows/shrinks
			Coord preferredSize = Direction::getCoord (item->preferredSize);
			float idealDelta;
			if(hasFill)
			{
				idealDelta = distributeFactor * item->fillFactor;
 				totalFill -= item->fillFactor;
				if(totalFill <= 0)
				{
					// done with last "fill" item
					hasFill = false;
					if(!__iter.done ())
					{
						// before proceeding with next item, we have to exit the inner loop to recalculate distributeFactor
						__iter.last ();
						__iter.next ();
					}
				}
			}
			else
				idealDelta = distributeFactor * preferredSize;

			Coord delta = Coord (idealDelta); 
			if(delta == 0 && !(hasFill && item->fillFactor == 0)) // force a change when delta was truncated to 0, but not if another item is preferred via fill option
				delta = mustShrink ? -1 : 1;

			Rect& r = item->workRect;

			if(mustShrink)
			{
				Coord possibleDelta = Direction::getMin (item->sizeLimits) - Direction::getEndCoord (r);
				if(possibleDelta > 0)
					possibleDelta = 0;
				if(possibleDelta >= delta)
				{
					delta = possibleDelta;
					workItems.remove (item); // this item cannot shrink anymore
					totalPreferredSize -= preferredSize;
				}
			}
			else
			{
				Coord possibleDelta = Direction::getMax (item->sizeLimits) - Direction::getEndCoord (r);
				if(possibleDelta < 0)
					possibleDelta = 0;
				if(possibleDelta <= delta)
				{
					delta = possibleDelta;
					workItems.remove (item); // this item cannot grow anymore
					totalPreferredSize -= preferredSize;
				}
			}

			Direction::getEndCoord (r) += delta;
			usedSpace += delta;

			if(mustShrink)
			{
				if(usedSpace <= availableSpace)
				{
					usedSpace = availableSpace;
					break;
				}
			}
			else
			{
				if(usedSpace > availableSpace)
				{
					// shrink item back to fit into available space
					Coord deltaOver = usedSpace - availableSpace;
					Direction::getEndCoord (r) -= deltaOver;
					usedSpace = availableSpace;
					break;
				}
				else if(usedSpace == availableSpace)
					break;
			}

		EndFor
	}

	// sizes have been calculated, now move the rectangles
	Coord pos = margin;
	ForEach (layoutItems, AnchorLayoutItem, item)
		Direction::moveTo (item->workRect, pos);
		pos += Direction::getLength (item->workRect) + spacing;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::accumulateSizeLimits (SizeLimit& limits, ObjectArray& layoutItems, Coord margin, Coord spacing)
{
	if(layoutItems.isEmpty ())
		Direction::getMin (limits) = Direction::getMax (limits) = 2 * margin;
	else
	{
		Direction::getMin (limits) = Direction::getMax (limits) = 2 * margin - spacing;

		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			const SizeLimit& itemLimits = item->sizeLimits;
			Direction::getMin (limits) += Direction::getMin (itemLimits) + spacing;
			Direction::getMax (limits) += Direction::getMax (itemLimits) + spacing;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
Coord LayoutPrimitives::accumulatePreferredSize (ObjectArray& layoutItems, Coord margin, Coord spacing)
{
	Coord total;
	if(layoutItems.isEmpty ())
		total = 2 * margin;
	else
	{
		total = 2 * margin - spacing;
		ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
			total += Direction::getCoord (item->preferredSize) + spacing;
		EndFor
	}
	return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
Coord LayoutPrimitives::getMaxCoord (ObjectArray& layoutItems)
{
	Coord max = 0;
	ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
		ccl_lower_limit (max, Direction::getEndCoord (item->getView ()->getSize ()));
	EndFor
	return max;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
Coord LayoutPrimitives::getMaxCoord (const View* parent)
{
	Coord max = 0;
	if(parent)
		ForEachViewFast (*parent, view)
			ccl_lower_limit (max, Direction::getEndCoord (view->getSize ()));
		EndFor
	return max;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::checkMinSize (Rect& rect, const SizeLimit& limits)
{
	Coord min = Direction::getMin (limits);
	if(Direction::getLength (rect) < min)
		Direction::setLength (rect, min);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::checkMaxSize (Rect& rect, const SizeLimit& limits)
{
	Coord max = Direction::getMax (limits);
	if(Direction::getLength (rect) > max)
		Direction::setLength (rect, max);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::checkMinSize (Point& p, const SizeLimit& limits)
{
	Coord min = Direction::getMin (limits);
	if(Direction::getCoord (p) < min)
		Direction::getCoord (p) = min;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::checkMaxSize (Point& p, const SizeLimit& limits)
{
	Coord max = Direction::getMax (limits);
	if(Direction::getCoord (p) > max)
		Direction::getCoord (p) = max;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void LayoutPrimitives::setFixedLength (SizeLimit& limits, Coord length)
{
	Direction::getMin (limits) = Direction::getMax (limits) = length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void LayoutPrimitives::resizeChildViews (Container& views, RectRef parentSize, const Point& delta, bool attachDisabled)
{
	if(attachDisabled)	// center must be checked anyway
	{
		ForEach (views, View, view)
			checkCenter (parentSize, *view);
		EndFor
	}
	else
	{
		ForEach (views, View, view)
			resizeChild (view, parentSize, delta);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void LayoutPrimitives::resizeChildItems (ObjectArray& items, RectRef parentSize, const Point& delta, bool attachDisabled)
{
	if(attachDisabled)	// center must be checked anyway
	{
		ForEach (items, LayoutItem, item)
			checkCenter (parentSize, *item->getView ());
		EndFor
	}
	else
	{
		ForEach (items, LayoutItem, item)
			resizeChild (item->getView (), parentSize, delta);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int flags>
inline bool LayoutPrimitives::isSizeMode (int sizeMode)
{ return (sizeMode & flags) == flags; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int flags>
inline bool LayoutPrimitives::isSizeMode (View* view)
{ return (view->getSizeMode () & flags) == flags; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_layoutprimitives_h
