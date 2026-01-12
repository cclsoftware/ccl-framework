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
// Filename    : ccl/public/gui/graphics/iuivalue.h
// Description : UI Value Interface
//
//************************************************************************************************

#ifndef _ccl_iuivalue_h
#define _ccl_iuivalue_h

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/3d/transform3d.h"

namespace CCL {

//************************************************************************************************
// IUIValue
/** Interface to wrap UI data structures like points and rectangles into IUnknown/Variant.
	\ingroup gui_graphics */
//************************************************************************************************

interface IUIValue: IUnknown
{
	DEFINE_ENUM (Type)
	{
		kNil,
		kPoint,
		kRect,
		kTransform,
		kColor,
		kColorF,
		kPointF,
		kRectF,
		kPointF3D,
		kPointF4D,
		kTransform3D
	};

	virtual void CCL_API reset () = 0;

	virtual tbool CCL_API copyFrom (const IUIValue* value) = 0;

	virtual Type CCL_API getType () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Conversion
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual void CCL_API fromPoint (PointRef p) = 0;

	virtual tbool CCL_API toPoint (Point& p) const = 0;

	virtual void CCL_API fromRect (RectRef r) = 0;

	virtual tbool CCL_API toRect (Rect& r) const = 0;

	virtual void CCL_API fromTransform (TransformRef t) = 0;

	virtual tbool CCL_API toTransform (Transform& t) const = 0;

	virtual void CCL_API fromColor (ColorRef c) = 0;

	virtual tbool CCL_API toColor (Color& color) const = 0;

	virtual void CCL_API fromColorF (ColorFRef c) = 0;

	virtual tbool CCL_API toColorF (ColorF& color) const = 0;

	virtual void CCL_API fromPointF (PointFRef p) = 0;

	virtual tbool CCL_API toPointF (PointF& p) const = 0;

	virtual void CCL_API fromRectF (RectFRef r) = 0;

	virtual tbool CCL_API toRectF (RectF& r) const = 0;

	virtual void CCL_API fromPointF3D (PointF3DRef p) = 0;

	virtual tbool CCL_API toPointF3D (PointF3D& p) const = 0;

	virtual void CCL_API fromPointF4D (PointF4DRef p) = 0;

	virtual tbool CCL_API toPointF4D (PointF4D& p) const = 0;

	virtual void CCL_API fromTransform3D (Transform3DRef t) = 0;

	virtual tbool CCL_API toTransform3D (Transform3D& t) const = 0;

	DECLARE_IID (IUIValue)

	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Get IUIValue object from Variant. */
	static INLINE IUIValue* toValue (VariantRef v)
	{
		return UnknownPtr<IUIValue> (v.asUnknown ());
	}

	/** Get value as PointF if conversion possible. */
	inline PointF convertToPointF () const
	{
		switch(getType ())
		{
		case kPoint : { Point p; toPoint (p); return {CoordF(p.x), CoordF(p.y)}; }
		case kPointF : { PointF p; toPointF (p); return p; }
		case kPointF3D : { PointF3D p; toPointF3D (p); return {p.x, p.y}; }
		case kPointF4D : { PointF4D p; toPointF4D (p); return {p.x, p.y}; }
		}
		return {};
	}
};

DEFINE_IID (IUIValue, 0xe492c93b, 0x8074, 0x4024, 0xba, 0xe9, 0x73, 0x73, 0x27, 0xc1, 0xc, 0x15)

} // namespace CCL

#endif // _ccl_iuivalue_h
