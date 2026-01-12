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
// Filename    : ccl/base/math/mathregion.cpp
// Description : Region class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/math/mathregion.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/base/primitives.h"
#include "ccl/public/base/debug.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;
using namespace Math;

//************************************************************************************************
// Math::Segment
//************************************************************************************************

bool Region::Segment::overlaps (const Segment& s) const
{
	double ox1 = ccl_max (x1, s.x1);
	double ox2 = ccl_min (x2, s.x2);
	double oy1 = ccl_max (y1, s.y1);
	double oy2 = ccl_min (y2, s.y2);
	return ox1 < ox2 && oy1 < oy2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Region::Segment::substract (const Segment& s, ObjectList& remainder) const
{
	if(overlaps (s))
	{
		// create the remaining parts of this (not covered by s)
		if(x1 < s.x1)
			remainder.prepend (NEW Segment (x1, ccl_max (y1, s.y1), s.x1, ccl_min (y2, s.y2)));
		if(s.x2 < x2)
			remainder.prepend (NEW Segment (s.x2, ccl_max (y1, s.y1), x2, ccl_min (y2, s.y2)));
		if(y1 < s.y1)
			remainder.prepend (NEW Segment (x1, y1, x2, s.y1));
		if(s.y2 < y2)
			remainder.prepend (NEW Segment (x1, s.y2, x2, y2));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isGreater (const Region::Segment& s1, const Region::Segment& s2)
{
	// order segments by first y1, then x1
	if(s1.y1 == s2.y1)
	{
		ASSERT (s1.x1 != s2.x1) // segments would overlap!
		return s1.x1 > s2.x1;
	}
	return s1.y1 > s2.y1;
}

//************************************************************************************************
// Math::Region
//************************************************************************************************

Region::Region ()
{
	segments.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region::Region (const Region& r)
{
	segments.objectCleanup ();

	ListForEachLinkable (r.segments, Segment, s)
		segments.append (NEW Segment (*s));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Region::insertSegment (Segment* segment)
{
	ListForEachLinkable (segments, Segment, s)
		if(isGreater (*s, *segment))
		{
			segments.insertBefore (s, segment);
			return;
		}
	EndFor
	segments.append (segment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region::Segment* Region::findExactTopNeighbour (Segment& s)
{
	// search backwards for a segment that exactly shares the top edge of s
	Segment* prev = &s;
	while((prev = prev->prevSegment ()))
	{
		if(prev->y2 == s.y1)
		{
			if(prev->x1 == s.x1 && prev->x2 == s.x2)
				return prev;
			else if(ccl_max (prev->x1, s.x1) < ccl_min (prev->x2, s.x2))
				return nullptr; // partly shares the edge: we won't find a full match
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region::Segment* Region::findBottomLineNeighbour (Segment& s)
{
	// search forward for a segment that lies on the same bottom line as s
	Segment* next = &s;
	while((next = next->nextSegment ()))
	{
		if(next->y2 == s.y2)
		{
			if(next->x1 == s.x2 || next->x2 == s.x1)
				return next;
			else if(next->y1 > s.y2)
				return nullptr; // top is already beyond, we won't find a match
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Region::simplify ()
{
	bool changes;
	do
	{
		// check for each segment if it can be combined with a neighbour
		changes = false;
		ListForEachLinkable (segments, Segment, s)
			Segment* left = s->prevSegment ();
			if(left && left->y1 == s->y1 && left->x2 == s->x1)
			{
				// horizontal neighbours: topRight of left is topLeft of s
				if(left->y2 == s->y2)
				{
					// left takes over the whole s
					ASSERT (left->x2 < s->x2)
					left->x2 = s->x2;
					segments.remove (s);
					s->release ();
					changes = true;
				}
				else if(left->y2 < s->y2)
				{
					// left takes over a part of s
					ASSERT (left->x2 < s->x2)
					left->x2 = s->x2;
					s->y1 = left->y2;
					segments.remove (s);
					insertSegment (s);
					changes = true;
				}
				else // left->y2 > s->y2
				{
					// s takes over a part of left
					ASSERT (left->x1 < s->x1)
					ASSERT (left->y1 < s->y2)
					s->x1 = left->x1;
					left->y1 = s->y2;
					segments.remove (s);
					segments.remove (left);
					insertSegment (s);
					insertSegment (left);
					ASSERT (segments.index (s) < segments.index (left))
					changes = true;
				}
			}
			else if(Segment* n = findBottomLineNeighbour (*s))
			{
				// n touches s horizontally at the bottom line
				if(n->y1 > s->y1)
				{
					// n takes over a part of s
					if(n->x1 == s->x2)
					{
						ASSERT (n->x1 > s->x1)
						n->x1 = s->x1;
					}
					else
					{
						ASSERT (n->x2 == s->x1)
						ASSERT (n->x2 < s->x2)
						n->x2 = s->x2;
					}
					ASSERT (n->y1 < s->y2)
					s->y2 = n->y1;
					changes = true;
				}
			}
			else if(Segment* top = findExactTopNeighbour (*s))
			{
				// the top neighbour takes over the whole s
				ASSERT (top->y2 < s->y2)
				top->y2 = s->y2;
				segments.remove (s);
				s->release ();
				changes = true;
			}
		EndFor
	} while(changes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Region::isEmpty () const
{
	return segments.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Region::isComplex () const
{
	return segments.getFirst () != segments.getLast ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Region::isIncluded (const Segment& segment) const
{
	// could this be implemented more efficiently?
	Region r;
	r.include (segment);

	// subtract this whole region from the given segment
	Segment s;
	RegionIterator iter (*this);
	while(iter.next (s))
		r.exclude (s);

	return r.isEmpty (); // if nothing remains, the segment must be included completely
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::include (const Segment& segment)
{
	ASSERT (segment.x1 <= segment.x2 && segment.y1 <= segment.y2)

	ObjectList newSegments;
	newSegments.append (NEW Segment (segment));

	// substract each existing segment from each new one
	ListForEachLinkable (segments, Segment, s)
		ListForEachObject (newSegments, Segment, n)
			if(n->substract (*s, newSegments))
			{
				newSegments.remove (n);
				n->release ();
			}
		EndFor
	EndFor

	ListForEachObject (newSegments, Segment, n)
		insertSegment (n);
	EndFor

	simplify ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::exclude (const Segment& segment)
{
	ObjectList newSegments; // parts remaining from segments that are not completely included anymore

	// substract segment from each existing segment
	ListForEachLinkable (segments, Segment, s)
		if(s->substract (segment, newSegments))
		{
			segments.remove (s);
			s->release ();
		}
	EndFor

	ListForEachObject (newSegments, Segment, n)
		insertSegment (n);
	EndFor

	simplify ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::makeEmpty ()
{
	segments.removeAll ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::makeInfinite ()
{
	segments.removeAll ();
	segments.add (NEW Segment (-NumericLimits::kLargeDouble, -NumericLimits::kLargeDouble, NumericLimits::kLargeDouble, NumericLimits::kLargeDouble));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::invert ()
{
	Region oldRegion (*this);

	makeInfinite ();

	ListForEachLinkable (oldRegion.segments, Segment, s)
		exclude (*s);
	EndFor
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Region& Region::invertHorizontally (double minX, double maxX)
{
	Region oldRegion (*this);

	makeEmpty ();

	// include full horizontal band
	ListForEachLinkable (oldRegion.segments, Segment, s)
		include (Segment (minX, s->y1, maxX, s->y2));
	EndFor

	// exclude selected segments
	ListForEachLinkable (oldRegion.segments, Segment, s)
		exclude (*s);
	EndFor
	return *this;
}

//************************************************************************************************
// Math::RegionIterator
//************************************************************************************************

RegionIterator::RegionIterator (const Region& region)
: LinkableListIterator (region.segments)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RegionIterator::next (Region::Segment& segment)
{
	if(LinkableListIterator::done ())
		return false;

	segment = *(Region::Segment*)LinkableListIterator::next ();
	return true;
}
