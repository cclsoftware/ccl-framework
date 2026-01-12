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
// Filename    : ccl/public/gui/graphics/pen.h
// Description : Pen definition
//
//************************************************************************************************

#ifndef _ccl_pen_h
#define _ccl_pen_h

#include "ccl/public/gui/graphics/color.h"

#include "ccl/public/base/cclmacros.h"

#include "ccl/meta/generated/cpp/graphics-constants-generated.h"

namespace CCL {

class Pen;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Pen reference
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Pen reference type. 
	\ingroup gui_graphics */
typedef const Pen& PenRef;

//************************************************************************************************
// PlainPen
/** The pen class below is binary equivalent to this C structure. 
	\ingroup gui_graphics */
//************************************************************************************************

struct PlainPen
{
	int32 style = 0;
	Color color;
	float width = 1.f;
};

//************************************************************************************************
// Pen
/** Pen definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class Pen: protected PlainPen
{
public:
	/** Pen type. */
	DEFINE_ENUM (PenType)
	{
		kSolid ///< solid color pen
	};

	/** Line Cap style. */
	DEFINE_ENUM (LineCap)
	{
		kLineCapButt   = kPenLineCapButt,
		kLineCapSquare = kPenLineCapSquare,
		kLineCapRound  = kPenLineCapRound
	};

	/** Line Join style. */
	DEFINE_ENUM (LineJoin)
	{
		kLineJoinMiter = kPenLineJoinMiter,
		kLineJoinBevel = kPenLineJoinBevel,
		kLineJoinRound = kPenLineJoinRound
	};

	typedef float Size;

	Pen (Color _color = Colors::kBlack, Size _width = Size (1))
	{
		style = kSolid;
		color = _color;
		width = _width;
	}

	PROPERTY_BY_VALUE (int32, style, Style)
	PROPERTY_BY_VALUE (Color, color, Color)
	PROPERTY_BY_VALUE (Size, width, Width)

	void setPenType (PenType type)		{ style = (style & ~kPenTypeMask) | (type & kPenTypeMask); }
	void setLineCap (LineCap cap)		{ style = (style & ~kLineCapMask) | (cap & kLineCapMask); }
	void setLineJoin (LineJoin join)	{ style = (style & ~kLineJoinMask) | (join & kLineJoinMask); }
	
	PenType getPenType () const		{ return style & kPenTypeMask; }
	LineCap getLineCap () const		{ return style & kLineCapMask; }
	LineJoin getLineJoin () const	{ return style & kLineJoinMask; }

	bool isEqual (PenRef pen) const 
	{ 
		return	pen.getStyle () == style && 
				pen.getColor () == color &&
				pen.getWidth () == width; 
	}

	bool operator == (PenRef other) const
	{
		return isEqual (other);
	}

	bool operator != (PenRef other) const
	{
		return !isEqual (other); 
	}

	enum
	{
		kPenTypeMask  = 0xff,
		kLineCapMask  = 0xff<<8,
		kLineJoinMask = 0xff<<16
	};
};

} // namespace CCL

#endif // _ccl_pen_h
