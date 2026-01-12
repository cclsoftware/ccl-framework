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
// Filename    : ccl/app/editing/snapper.h
// Description : Snapper
//
//************************************************************************************************

#ifndef _ccl_snapper_h
#define _ccl_snapper_h

#include "ccl/base/object.h"

#include "ccl/app/editing/iscale.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// Snapper
//************************************************************************************************

class Snapper: public Object,
			   public IScale
{
public:
	DECLARE_CLASS (Snapper, Object)

	Snapper ();

	PROPERTY_VARIABLE (Coord, snapValue, SnapValue)

	virtual Coord snapPosition (Coord position) const;

	virtual int getSnapIndex (Coord position) const;
	virtual Coord getSnapPosition (int index) const;
	virtual Coord getSnapSize (int index) const;

	// IScale
	Coord unitToPixel (Unit value) const override;
	Unit pixelToUnit (Coord position) const override;
	void getExtent (Unit start, Unit end, Coord& startCoord, Coord& endCoord) const override;
	Unit getNumUnits () const override;
	bool isReversed () const override;
	IFormatter* createFormatter () const override;

	CLASS_INTERFACE (IScale, Object)
};

//************************************************************************************************
// AdvancedSnapper
//************************************************************************************************

class AdvancedSnapper: public Snapper
{
public:
	virtual int countSnaps () const = 0; ///< overwrite getSnapSize () for indiviual size per snap

	// Snapper
	Coord snapPosition (Coord position) const override;
	int getSnapIndex (Coord position) const override;
	Coord getSnapPosition (int index) const override;
	
	// IScale
	Unit getNumUnits () const override;
};

//********************************************************************************************
// TableSnapper
/** Uses a table of snap sizes. */
//********************************************************************************************

class TableSnapper: public CCL::AdvancedSnapper
{
public:
	void addSnap (Coord size);
	void removeAll ();

	// AdvancedSnapper
	int countSnaps () const override;
	Coord getSnapSize (int index) const override;

protected:
	Vector<Coord> snapSizes;
};

//************************************************************************************************
// FullVerticalSnapper
/** Maps the whole data height of a View to one index.
	Reports snapSize 0 for all other indices. */
//************************************************************************************************

class FullVerticalSnapper: public CCL::AdvancedSnapper
{
public:
	FullVerticalSnapper (IView* view);

	PROPERTY_VARIABLE (int, numSnaps, NumSnaps)
	PROPERTY_VARIABLE (int, mainIndex, MainIndex)

	// AdvancedSnapper
	int countSnaps () const override;
	CCL::Coord getSnapSize (int index) const override;

protected:
	ViewPtr view;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TableSnapper::addSnap (Coord size) { snapSizes.add (size); }
inline void TableSnapper::removeAll () { snapSizes.removeAll (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_snapper_h
