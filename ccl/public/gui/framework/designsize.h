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
// Filename    : ccl/public/gui/framework/designsize.h
// Description : Design Size
//
//************************************************************************************************

#ifndef _ccl_designsize_h
#define _ccl_designsize_h

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/base/variant.h"

namespace CCL {

//************************************************************************************************
// DesignCoord
//************************************************************************************************

struct DesignCoord
{
	enum Unit: int32
	{
		kUndefined,
		kAuto,
		kPercent,
		kCoord
	};
	
	static const String kStrAuto;
	static const String kStrUndefined;
	static const String kStrPercent;
	
	Unit unit;
	Coord value;

	DesignCoord (Unit unit = kAuto, Coord value = 0)
	: unit (unit),
	  value (value)
	{}
	
	Variant toVariant () const;
	DesignCoord& fromVariant (VariantRef variant);
	
	DesignCoord operator + (const DesignCoord& dc) const;
	DesignCoord operator - (const DesignCoord& dc) const;
	DesignCoord& operator += (const DesignCoord& dc);
	DesignCoord& operator -= (const DesignCoord& dc);
	bool operator == (const DesignCoord& dc) const;
	bool operator != (const DesignCoord& dc) const;
	
	bool isUndefined () const;
	bool isAuto () const;
	bool isCoord () const;
	bool isPercent () const;
};

//************************************************************************************************
// DesignSize
/** A data representation of the design size as it can be specified for a skin element */
//************************************************************************************************

struct DesignSize
{
	DesignCoord left;
	DesignCoord top;
	DesignCoord width;
	DesignCoord height;
	
	DesignSize (const DesignCoord& left = DesignCoord::kAuto,
				const DesignCoord& top = DesignCoord::kAuto,
				const DesignCoord& width = DesignCoord::kAuto,
				const DesignCoord& height = DesignCoord::kAuto);
	
	/** All coordinates will be set to DesignCoord::kCoord */
	DesignSize& fromRect (RectRef rect);
	
	/** Set rect coordinates from plain coordinates (DesignCoord::kCoord). Coordinates with other units are skipped. */
	void toRect (Rect& rect) const;
};

} // namespace CCL

#endif // _ccl_designsize_h
