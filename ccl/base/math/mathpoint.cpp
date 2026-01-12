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
// Filename    : ccl/base/math/mathpoint.coo
// Description : Mathematical Point
//
//************************************************************************************************

#include "ccl/base/math/mathpoint.h"

#include "ccl/base/storage/storage.h"

using namespace CCL;
using namespace Math;

//************************************************************************************************
// Boxed::MathPoint
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::MathPoint, Object, "MathPoint")

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MathPoint::MathPoint ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::MathPoint::MathPoint (CCL::Math::PointRef p)
: Math::Point (p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::MathPoint::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	x = a.getFloat ("x");
	y = a.getFloat ("y");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::MathPoint::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("x", x);
	a.set ("y", y);
	return true;
}
