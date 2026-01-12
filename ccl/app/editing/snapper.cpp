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
// Filename    : ccl/app/editing/snapper.cpp
// Description : Snapper
//
//************************************************************************************************

#include "ccl/app/editing/snapper.h"

using namespace CCL;

//************************************************************************************************
// Snapper
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Snapper, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Snapper::Snapper ()
: snapValue (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Snapper::snapPosition (Coord position) const
{
	return getSnapIndex (position) * snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Snapper::getSnapIndex (Coord position) const
{
	ASSERT (snapValue > 0)
	return position / snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Snapper::getSnapPosition (int index) const
{
	return index * snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Snapper::getSnapSize (int index) const
{
	return snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Snapper::unitToPixel (Unit value) const
{
	return getSnapPosition (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScale::Unit Snapper::pixelToUnit (Coord position) const
{
	return getSnapIndex (position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScale::Unit Snapper::getNumUnits () const
{
	CCL_NOT_IMPL ("Snapper::getNumUnits should not be called!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Snapper::isReversed () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* Snapper::createFormatter () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Snapper::getExtent (Unit startUnit, Unit endUnit, Coord& startCoord, Coord& endCoord) const
{
	startCoord = unitToPixel (startUnit);
	endCoord = unitToPixel (endUnit + 1);
}

//************************************************************************************************
// AdvancedSnapper
//************************************************************************************************

Coord AdvancedSnapper::snapPosition (Coord position) const
{
	if(position < 0)
		return 0;

	Coord pos = 0;
	int count = countSnaps ();
	for(int i = 0; i < count; i++)
	{
		Coord curSize = getSnapSize (i);
		if(position >= pos && position < pos + curSize)
			return pos;

		pos += curSize;
	}

	Coord remainder = position - pos;
	return pos + (remainder / snapValue) * snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AdvancedSnapper::getSnapIndex (Coord position) const
{
	if(position < 0)
		return 0;

	Coord pos = 0;
	int count = countSnaps ();
	for(int i = 0; i < count; i++)
	{
		Coord curSize = getSnapSize (i);
		if(position >= pos && position < pos + curSize)
			return i;

		pos += curSize;
	}

	Coord remainder = position - pos;
	return count + (remainder / snapValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord AdvancedSnapper::getSnapPosition (int index) const
{
	Coord pos = 0;
	int count = countSnaps ();
	for(int i = 0; i < count; i++)
	{
		if(i >= index)
			return pos;

		pos += getSnapSize (i);
	}

	return pos + (index - count) * snapValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScale::Unit AdvancedSnapper::getNumUnits () const
{
	return countSnaps ();
}

//************************************************************************************************
// TableSnapper
//************************************************************************************************

int TableSnapper::countSnaps () const
{
	return snapSizes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TableSnapper::getSnapSize (int index) const
{
	if(index >= 0 && index < snapSizes.count ())
		return snapSizes[index];

	return snapValue;
}

//************************************************************************************************
// FullVerticalSnapper
//************************************************************************************************

FullVerticalSnapper::FullVerticalSnapper (IView* view)
: view (view),
  numSnaps (1),
  mainIndex (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FullVerticalSnapper::countSnaps () const
{
	return numSnaps;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord FullVerticalSnapper::getSnapSize (int index) const
{
	if(view && index == mainIndex)
		return view->getSize ().getHeight ();

	return 0;
}
