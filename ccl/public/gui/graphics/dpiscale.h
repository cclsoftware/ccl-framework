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
// Filename    : ccl/public/gui/graphics/dpiscale.h
// Description : DPI Scaling
//
//************************************************************************************************

#ifndef _ccl_dpiscale_h
#define _ccl_dpiscale_h

#include "ccl/public/gui/graphics/rect.h"

#include <math.h>

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// DpiScale
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace DpiScale 
{
	constexpr float kFloatCoordPrecision = 0.00195; // precision of float to integer conversion (32768 / 2^24)

	float getDpi (float dpiFactor);
	float getFactor (int dpi);

	float coordToPixelF (int coord, float dpiFactor);
	float pixelToCoordF (int pixel, float dpiFactor);
	float coordFToPixelF (float coord, float dpiFactor);
	float pixelFToCoordF (float pixel, float dpiFactor);
	int coordToPixel (int coord, float dpiFactor);
	int pixelToCoord (int pixel, float dpiFactor);
	void toPixelPoint (Point& p, float dpiFactor);
	void toCoordPoint (Point& p, float dpiFactor);
	void toPixelRect (Rect& size, float dpiFactor);
	void toCoordRect (Rect& size, float dpiFactor);
	void toPixelPointF (PointF& p, float dpiFactor);
	void toCoordPointF (PointF& p, float dpiFactor);
	void toPixelRectF (RectF& size, float dpiFactor);
	void toCoordRectF (RectF& size, float dpiFactor);

	bool isIntAligned (float); // check if float can be converted to int without remainder
	bool isPointIntAligned (const PointF& p);
	bool isRectIntAligned (const RectF& r);
}

//************************************************************************************************
// PixelRect
//************************************************************************************************

class PixelRect: public Rect
{
public:
	PixelRect (RectRef rect, float dpiFactor)
	: Rect (rect)
	{
		DpiScale::toPixelRect (*this, dpiFactor);
	}
};

//************************************************************************************************
// PixelRectF
//************************************************************************************************

class PixelRectF: public RectF
{
public:
	PixelRectF (RectRef rect, float dpiFactor)
	: RectF (rectIntToF (rect))
	{
		DpiScale::toPixelRectF (*this, dpiFactor);
	}
    
    PixelRectF (RectFRef rect, float dpiFactor)
    : RectF (rect)
    {
        DpiScale::toPixelRectF (*this, dpiFactor);
    }
    
    bool isPixelAligned () const 
	{
		return DpiScale::isRectIntAligned (*this);
	}
};

//************************************************************************************************
// PixelPoint
//************************************************************************************************

class PixelPoint: public Point
{
public:
	PixelPoint (PointRef point, float dpiFactor)
	: Point (point)
	{
		DpiScale::toPixelPoint (*this, dpiFactor);
	}
};

//************************************************************************************************
// PixelPointF
//************************************************************************************************

class PixelPointF: public PointF
{
public:
	PixelPointF (PointRef point, float dpiFactor)
	: PointF (pointIntToF (point))
	{
		DpiScale::toPixelPointF (*this, dpiFactor);
	}

	bool isPixelAligned () const 
	{
		return DpiScale::isPointIntAligned (*this);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline float DpiScale::getDpi (float dpiFactor) {return 96.f * dpiFactor; }
inline float DpiScale::getFactor (int dpi) { return dpi / 96.f; }

inline float DpiScale::coordToPixelF (int coord, float dpiFactor) { return (coord * dpiFactor); }
inline float DpiScale::pixelToCoordF (int pixel, float dpiFactor) { return (pixel / dpiFactor); }
inline float DpiScale::coordFToPixelF (float coord, float dpiFactor) { return (coord * dpiFactor); }
inline float DpiScale::pixelFToCoordF (float pixel, float dpiFactor) { return (pixel / dpiFactor); }

inline int DpiScale::coordToPixel (int coord, float dpiFactor)
{
	if(coord >= 0)
		return int (coordToPixelF (coord, dpiFactor) + kFloatCoordPrecision);
	else
		return int (coordToPixelF (coord, dpiFactor) - kFloatCoordPrecision);
}

inline int DpiScale::pixelToCoord (int pixel, float dpiFactor)
{
	if(pixel >= 0)
		return int (ceil (pixelToCoordF (pixel, dpiFactor) - kFloatCoordPrecision));
	else
		return int (floor (pixelToCoordF (pixel, dpiFactor) + kFloatCoordPrecision));
}

inline void DpiScale::toPixelPoint (Point& p, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		p.x = coordToPixel (p.x, dpiFactor);
		p.y = coordToPixel (p.y, dpiFactor);
	}
}

inline void DpiScale::toCoordPoint (Point& p, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		p.x = pixelToCoord (p.x, dpiFactor);
		p.y = pixelToCoord (p.y, dpiFactor);
	}
}

inline void DpiScale::toPixelRect (Rect& size, float dpiFactor)
{ 
	if(dpiFactor != 1.f) 
	{
		int w = size.getWidth ();
		int h = size.getHeight ();		
		size.left = coordToPixel (size.left, dpiFactor);
		size.top = coordToPixel (size.top, dpiFactor);
		size.setWidth (coordToPixel (w, dpiFactor));
		size.setHeight (coordToPixel (h, dpiFactor));
	}
}

inline void DpiScale::toCoordRect (Rect& size, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		int w = size.getWidth ();
		int h = size.getHeight ();		
		size.left = pixelToCoord (size.left, dpiFactor);
		size.top = pixelToCoord (size.top, dpiFactor);
		size.setWidth (pixelToCoord (w, dpiFactor));
		size.setHeight (pixelToCoord (h, dpiFactor));
	}
}

inline void DpiScale::toPixelPointF (PointF& p, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		p.x = coordFToPixelF (p.x, dpiFactor);
		p.y = coordFToPixelF (p.y, dpiFactor);
	}
}

inline void DpiScale::toCoordPointF (PointF& p, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		p.x = pixelFToCoordF (p.x, dpiFactor);
		p.y = pixelFToCoordF (p.y, dpiFactor);
	}
}

inline void DpiScale::toPixelRectF (RectF& size, float dpiFactor)
{ 
	if(dpiFactor != 1.f) 
	{
		size.left = coordFToPixelF (size.left, dpiFactor);
		size.top = coordFToPixelF (size.top, dpiFactor);
		size.right = coordFToPixelF (size.right, dpiFactor);
		size.bottom = coordFToPixelF (size.bottom, dpiFactor);
	}
}

inline void DpiScale::toCoordRectF (RectF& size, float dpiFactor)
{
	if(dpiFactor != 1.f)
	{
		size.left = pixelFToCoordF (size.left, dpiFactor);
		size.top = pixelFToCoordF (size.top, dpiFactor);
		size.right = pixelFToCoordF (size.right, dpiFactor);
		size.bottom = pixelFToCoordF (size.bottom, dpiFactor);
	}
}

inline bool DpiScale::isIntAligned (float f)
{
	return fmod (f, 1) == 0;	
}

inline bool DpiScale::isPointIntAligned (const PointF& p)
{
	return isIntAligned (p.x) 
		&& isIntAligned (p.y);
}

inline bool DpiScale::isRectIntAligned (const RectF& r)
{
	return isIntAligned (r.left) 
		&& isIntAligned (r.top)
		&& isIntAligned (r.right)
		&& isIntAligned (r.bottom);
}

} // namespace CCL

#endif // _ccl_dpiscale_h
