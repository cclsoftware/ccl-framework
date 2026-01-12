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
// Filename    : ccl/platform/win/direct2d/d2dinterop.h
// Description : Direct2D Interoperability Helpers
//
//************************************************************************************************

#ifndef _ccl_direct2d_interop_h
#define _ccl_direct2d_interop_h

#include "ccl/public/base/debug.h"
#include "ccl/public/gui/graphics/types.h"

#include "ccl/platform/win/system/cclcom.h" // needs to be included before Windows headers

#include <D2d1_1.h>
#include <math.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DError
//************************************************************************************************

#if DEBUG
namespace D2DError
{
	void print (HRESULT hr);
}
#endif

//************************************************************************************************
// D2DClientRenderDevice
//************************************************************************************************

struct D2DClientRenderDevice
{
	virtual void suspend (bool state) = 0;
};

namespace D2DInterop {

//************************************************************************************************
// Transform
//************************************************************************************************

inline D2D1::Matrix3x2F toMatrix (CCL::TransformRef src)
{
	return D2D1::Matrix3x2F (src.a0, src.a1, src.b0, src.b1, src.t0, src.t1);
}

inline CCL::Transform fromMatrix (const D2D1_MATRIX_3X2_F& src)
{
	return CCL::Transform (src._11, src._12, src._21, src._22, src._31, src._32);
}

//************************************************************************************************
// Point
//************************************************************************************************

inline D2D1_POINT_2F toPointF (CCL::PointRef src)
{
	D2D1_POINT_2F dst = {(FLOAT)src.x, (FLOAT)src.y};
	return dst;
}

inline D2D1_POINT_2U toPointU (CCL::PointRef src)
{
	D2D1_POINT_2U dst = {(UINT32)src.x, (UINT32)src.y};
	return dst;
}

inline PointFRef toCCL (const D2D1_POINT_2F& p)
{
	return reinterpret_cast<PointFRef> (p); // types are compatible
}

inline const D2D1_POINT_2F& fromCCL (CCL::PointFRef p)
{
	return reinterpret_cast<const D2D1_POINT_2F&> (p); // types are compatible
}

//************************************************************************************************
// Rect
//************************************************************************************************

inline D2D1_RECT_F toRectF (CCL::RectRef rect)
{
	return D2D1::RectF ((FLOAT)rect.left, (FLOAT)rect.top, (FLOAT)rect.right, (FLOAT)rect.bottom);
}

inline D2D_RECT_U toRectU (CCL::RectRef rect)
{
	return D2D1::RectU (rect.left, rect.top, rect.right, rect.bottom);
}

inline Rect fromRectF (const D2D1_RECT_F& rect)
{
	return Rect ((Coord)rect.left, (Coord)rect.top, (Coord)rect.right, (Coord)rect.bottom);
}

inline RectFRef toCCL (const D2D1_RECT_F& rect)
{
	return reinterpret_cast<RectFRef> (rect); // types are compatible
}

inline const D2D1_RECT_F& fromCCL (RectFRef rect)
{
	return reinterpret_cast<const D2D1_RECT_F&> (rect); // types are compatible
}

inline D2D1_ELLIPSE toEllipse (CCL::RectRef rect)
{
	D2D1_ELLIPSE e;
	e.point = toPointF (rect.getCenter ());
	e.radiusX = (FLOAT)rect.getWidth () / 2.f;
	e.radiusY = (FLOAT)rect.getHeight () / 2.f;
	return e;
}

inline D2D1_ELLIPSE toEllipse (CCL::RectFRef rect)
{
	D2D1_ELLIPSE e;
	e.point = fromCCL (rect.getCenter ());
	e.radiusX = rect.getWidth () / 2.f;
	e.radiusY = rect.getHeight () / 2.f;
	return e;
}

//************************************************************************************************
// Color
//************************************************************************************************

inline D2D1_COLOR_F toColorF (CCL::Color color)
{
	return D2D1::ColorF (color.getRedF (), color.getGreenF (), color.getBlueF (), color.getAlphaF ());
}

} // namespace D2DInterop

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_interop_h
