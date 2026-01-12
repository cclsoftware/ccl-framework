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
// Filename    : ccl/app/editing/iscale.h
// Description : Scale Interface
//
//************************************************************************************************

#ifndef _ccl_iscale_h
#define _ccl_iscale_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

interface IFormatter;

//************************************************************************************************
// IScale
//************************************************************************************************

interface IScale: IUnknown
{
	/** Data unit type (fixed-point!). */
	typedef int Unit;

	/** Convert from data unit to pixel position. */
	virtual Coord unitToPixel (Unit value) const = 0;

	/** Convert pixel position to data unit. */
	virtual Unit pixelToUnit (Coord position) const = 0;

	/** Get pixel extent from data units. */
	virtual void getExtent (Unit start, Unit end, Coord& startCoord, Coord& endCoord) const = 0;

	/** Get number of total data units. */
	virtual Unit getNumUnits () const = 0;

	/** Check if scale is reversed (unit counting from max down to zero). */
	virtual bool isReversed () const = 0;

	/** Create a formatter for values of this scale (optional). */
	virtual IFormatter* createFormatter () const = 0;

	DECLARE_IID (IScale)
};

} // namespace CCL

#endif // _ccl_iscale_h
