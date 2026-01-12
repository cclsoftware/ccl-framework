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
// Filename    : ccl/base/math/mathpoint.h
// Description : Mathematical Point
//
//************************************************************************************************

#ifndef _ccl_mathpoint_h
#define _ccl_mathpoint_h

#include "ccl/base/object.h"

#include "core/public/gui/corepoint.h"

namespace CCL {
namespace Math {

//************************************************************************************************
// Math::Point
//************************************************************************************************

typedef Core::TPoint<double> Point;
typedef const Point& PointRef;

} // namespace Math

namespace Boxed {

//************************************************************************************************
// Boxed::MathPoint
//************************************************************************************************

class MathPoint: public Object,
		         public Math::Point
{
public:
	DECLARE_CLASS (MathPoint, Object)

	MathPoint ();
	MathPoint (Math::PointRef p);

	// Object
	bool load (const Storage&) override;
	bool save (const Storage&) const override;
};

} // namespace Boxed
} // namespace CCL

#endif // _ccl_mathpoint_h
