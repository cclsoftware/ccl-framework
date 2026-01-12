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
// Filename    : ccl/base/math/mathregion.h
// Description : Region class
//
//************************************************************************************************

#ifndef _ccl_mathregion_h
#define _ccl_mathregion_h

#include "ccl/base/collections/linkablelist.h"

namespace CCL {

class ObjectList;

namespace Math {

//************************************************************************************************
// Math::Region
//************************************************************************************************

class Region
{
public:
	struct Segment: public Linkable
	{
		double x1;
		double y1;
		double x2;
		double y2;

		Segment (double x1 = 0., double y1 = 0., double x2 = 0., double y2 = 0.)
		: x1 (x1),
		  y1 (y1),
		  x2 (x2),
		  y2 (y2)
		{}

		bool overlaps (const Segment& s) const;
		bool substract (const Segment& s, ObjectList& remainder) const;

		Segment* nextSegment () const;
		Segment* prevSegment () const;
	};

	Region ();
	Region (const Region& r);

	bool isEmpty () const;
	bool isComplex () const;
	bool isIncluded (const Segment& segment) const;

	Region& include (const Segment& segment);
	Region& exclude (const Segment& segment);
	
	Region& makeEmpty ();
	Region& makeInfinite ();

	Region& invert ();
	Region& invertHorizontally (double minX = -NumericLimits::kLargeDouble, double maxX = NumericLimits::kLargeDouble); ///< only invert in horizontal "bands", keep the vertical range of segments

protected:
	void insertSegment (Segment* segment);
	Segment* findExactTopNeighbour (Segment& s);
	Segment* findBottomLineNeighbour (Segment& s);
	void simplify ();

	friend class RegionIterator;
	LinkableList segments;
};

//************************************************************************************************
// Math::RegionIterator
//************************************************************************************************

class RegionIterator: protected LinkableListIterator
{
public:
	RegionIterator (const Region& region);

	bool next (Region::Segment& segment);
	using LinkableListIterator::first;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Region::Segment* Region::Segment::nextSegment () const { return (Segment*)getNext (); }
inline Region::Segment* Region::Segment::prevSegment () const { return (Segment*)getPrevious (); }

} // namespace Math
} // namespace CCL

#endif // _ccl_mathregion_h
