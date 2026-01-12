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
// Filename    : ccl/public/gui/graphics/brush.h
// Description : Brush definition
//
//************************************************************************************************

#ifndef _ccl_brush_h
#define _ccl_brush_h

#include "ccl/public/gui/graphics/igradient.h"

#include "ccl/public/base/cclmacros.h"

namespace CCL {

class Brush;
class SolidBrush;
class GradientBrush;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Brush references
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Brush reference type. 
	\ingroup gui_graphics */
typedef const Brush& BrushRef;

/** Solid brush reference type. 
	\ingroup gui_graphics */
typedef const SolidBrush& SolidBrushRef;

/** Gradient brush reference type. 
	\ingroup gui_graphics */
typedef const GradientBrush& GradientBrushRef;

//************************************************************************************************
// PlainBrush
/** The brush class below is binary equivalent to this C structure. 
	\ingroup gui_graphics */
//************************************************************************************************

struct PlainBrush
{
	int32 type = 0;
	Color color;
	IGradient* gradient = nullptr;
};

//************************************************************************************************
// Brush
/** Brush definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class Brush: protected PlainBrush
{
public:
	/** Brush type. */
	DEFINE_ENUM (BrushType)
	{
		kSolid,		///< solid color brush
		kGradient	///< gradient brush
	};

	Brush (BrushType _type)
	{
		type = _type;
		ASSERT (type == kSolid || type == kGradient)
	}

	Brush (BrushRef other)
	{
		type = other.type;
		color = other.color;
		take_shared (gradient, other.gradient);
	}

	~Brush ()
	{
		safe_release (gradient);
	}

	PROPERTY_BY_VALUE (BrushType, type, Type)
	PROPERTY_BY_VALUE (Color, color, Color)
	PROPERTY_SHARED_METHODS (IGradient, gradient, Gradient)

	Brush& operator = (BrushRef other)
	{
		type = other.type;
		color = other.color;
		take_shared (gradient, other.gradient);
		return *this;
	}

	bool isEqual (BrushRef other) const
	{
		return	type == other.type && 
				color == other.color && 
				gradient == other.gradient; 
	}

	bool operator == (BrushRef other) const
	{
		return isEqual (other);
	}

	bool operator != (BrushRef other) const
	{
		return !isEqual (other); 
	}
};

//************************************************************************************************
// SolidBrush
/** Solid brush definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class SolidBrush: public Brush
{
public:
	SolidBrush (Color _color = Colors::kBlack)
	: Brush (kSolid)
	{
		color = _color;
	}

	SolidBrush (BrushRef other)
	: Brush (other)
	{
		ASSERT (other.getType () == kSolid)
	}

	SolidBrush& blendBrushColor (BrushRef other, float alpha = 1.f)
	{
		color.alphaBlend (other.getColor (), alpha);
		return *this;		
	}

	static inline const SolidBrush* castRef (BrushRef brush)
	{
		return brush.getType () == kSolid ? static_cast<const SolidBrush*> (&brush) : nullptr;
	}
};

//************************************************************************************************
// GradientBrush
/** Gradient brush definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class GradientBrush: public Brush
{
public:
	GradientBrush (IGradient* gradient = nullptr)
	: Brush (kGradient)
	{
		setGradient (gradient);
	}

	GradientBrush (BrushRef other)
	: Brush (other)
	{
		ASSERT (other.getType () == kGradient)
	}

	static inline const GradientBrush* castRef (BrushRef brush)
	{
		return brush.getType () == kGradient ? static_cast<const GradientBrush*> (&brush) : nullptr;
	}
};

//************************************************************************************************
// LinearGradientBrush
/** Linear gradient brush. 
	\ingroup gui_graphics */
//************************************************************************************************

class LinearGradientBrush: public GradientBrush
{
public:
	LinearGradientBrush (PointFRef startPoint, PointFRef endPoint, 
						 ColorRef startColor, ColorRef endColor);

	LinearGradientBrush (PointFRef startPoint, PointFRef endPoint, 
						 const IGradient::Stop stops[], int stopCount);

	LinearGradientBrush (PointFRef startPoint, PointFRef endPoint,
						 GradientBrushRef other);

protected:
	void construct (PointFRef startPoint, PointFRef endPoint,
					const IGradient::Stop stops[], int stopCount,
					IGradient* other = nullptr);
};

//************************************************************************************************
// RadialGradientBrush
/** Radial gradient brush. 
	\ingroup gui_graphics */
//************************************************************************************************

class RadialGradientBrush: public GradientBrush
{
public:
	RadialGradientBrush (PointFRef center, float radius, 
						 ColorRef startColor, ColorRef endColor);

	RadialGradientBrush (PointFRef center, float radius,
						 const IGradient::Stop stops[], int stopCount);

	RadialGradientBrush (PointFRef center, float radius,
						 GradientBrushRef other);

protected:
	void construct (PointFRef center, float radius,
					const IGradient::Stop stops[], int stopCount,
					IGradient* other = nullptr);
};

} // namespace CCL

#endif // _ccl_brush_h
