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
// Filename    : ccl/public/gui/framework/designsize.cpp
// Description : Design Size
//
//************************************************************************************************

#include "ccl/public/gui/framework/designsize.h"

using namespace CCL;

//************************************************************************************************
// DesignCoord
//************************************************************************************************

const String DesignCoord::kStrAuto = CCLSTR ("auto");
const String DesignCoord::kStrUndefined = CCLSTR ("undefined");
const String DesignCoord::kStrPercent = CCLSTR ("%");

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant DesignCoord::toVariant () const
{
	Variant variant (value);
	variant.setUserValue (unit);
	return variant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord& DesignCoord::fromVariant (VariantRef variant)
{
	value = variant.asInt ();
	unit = Unit(variant.getUserValue ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord DesignCoord::operator + (const DesignCoord& dc) const
{
	ASSERT (dc.unit == unit)
	
	DesignCoord result;
	result.unit = unit;
	if(unit == kPercent || unit == kCoord)
		result.value = value + dc.value;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord DesignCoord::operator - (const DesignCoord& dc) const
{
	ASSERT (dc.unit == unit)
	
	DesignCoord result;
	result.unit = unit;
	if(unit == kPercent || unit == kCoord)
		result.value = value - dc.value;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord& DesignCoord::operator += (const DesignCoord& dc)
{
	ASSERT (dc.unit == unit)
	
	if(unit == kPercent || unit == kCoord)
		value += dc.value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord& DesignCoord::operator -= (const DesignCoord& dc)
{
	ASSERT (dc.unit == unit)
	
	if(unit == kPercent || unit == kCoord)
		value -= dc.value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::operator == (const DesignCoord& dc) const
{
	return (unit == dc.unit) && (value == dc.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::operator != (const DesignCoord& dc) const
{
	return !operator == (dc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::isUndefined () const
{
	return unit == kUndefined;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::isAuto () const
{
	return unit == kAuto;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::isCoord () const
{
	return unit == kCoord;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DesignCoord::isPercent () const 
{
	return unit == kPercent;
}

//************************************************************************************************
// DesignSize
//************************************************************************************************

DesignSize::DesignSize (const DesignCoord& left, const DesignCoord& top, const DesignCoord& width, const DesignCoord& height)
: left (left),
  top (top),
  width (width),
  height (height)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignSize& DesignSize::fromRect (RectRef rect)
{
	left.unit = DesignCoord::kCoord;
	top.unit = DesignCoord::kCoord;
	width.unit = DesignCoord::kCoord;
	height.unit = DesignCoord::kCoord;
	
	left.value = rect.left;
	top.value = rect.top;
	width.value = rect.getWidth ();
	height.value = rect.getHeight ();
	
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DesignSize::toRect (Rect& rect) const
{
	if(left.unit == DesignCoord::kCoord)
		rect.left = left.value;
	
	if(top.unit == DesignCoord::kCoord)
		rect.top = top.value;

	if(width.unit == DesignCoord::kAuto)
		rect.setWidth (0);
	else if(width.unit == DesignCoord::kCoord)
		rect.setWidth (width.value);

	if(height.unit == DesignCoord::kAuto)
		rect.setHeight (0);
	if(height.unit == DesignCoord::kCoord)
		rect.setHeight (height.value);
}
