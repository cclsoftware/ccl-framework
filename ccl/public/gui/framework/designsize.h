//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
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
	float value;

	DesignCoord (Unit unit = kAuto, float value = 0)
	: unit (unit),
	  value (value)
	{}
	
	int getIntValue () const;
	DesignCoord& setIntValue (int value);

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

typedef const DesignCoord& DesignCoordRef;

//************************************************************************************************
// DesignSize
/** A data representation of the design size as it can be specified for a skin element. */
//************************************************************************************************

struct DesignSize
{
	DesignCoord left;
	DesignCoord top;
	DesignCoord width;
	DesignCoord height;
	
	DesignSize (DesignCoordRef left = DesignCoord::kAuto,
				DesignCoordRef top = DesignCoord::kAuto,
				DesignCoordRef width = DesignCoord::kAuto,
				DesignCoordRef height = DesignCoord::kAuto);
	
	/** All coordinates will be set to DesignCoord::kCoord */
	DesignSize& fromRect (RectFRef rect);
	DesignSize& fromRect (RectRef rect);
	
	/** Set rect coordinates from plain coordinates (DesignCoord::kCoord). Coordinates with other units are skipped. */
	void toRect (RectF& rect) const;
	void toRect (Rect& rect) const;
};

} // namespace CCL

#endif // _ccl_designsize_h
