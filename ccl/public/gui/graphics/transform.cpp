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
// Filename    : ccl/public/gui/graphics/transform.cpp
// Description : 2D Transformation Matrix
//
//************************************************************************************************

#include "ccl/public/gui/graphics/transform.h"

#include <math.h>

using namespace CCL;

//************************************************************************************************
// Transform
//************************************************************************************************

Transform& Transform::translate (float tx, float ty)
{
	//	| a0 b0 t0 |   | 1 0 tx |   | a0  b0  a0 * tx + b0 * ty + t0 |
	//	| a1 b1 t1 | x | 0 1 ty | = | a1  b1  a1 * tx + b1 * ty + t1 |
	//	|  0  0  1 |   | 0 0  1 |   |  0   0          1              |

	t0 += a0 * tx + b0 * ty;
	t1 += a1 * tx + b1 * ty;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform& Transform::scale (float sx, float sy)
{
	//	| a0 b0 t0 |   | sx  0  0 |   | a0 * sx  b0 * sy  t0 |
	//	| a1 b1 t1 | x | 0  sy  0 | = | a1 * sx  b1 * sy  t1 |
	//	|  0  0  1 |   | 0   0  1 |   |    0        0      1 |

	a0 *= sx;
	a1 *= sx;
	b0 *= sy;
	b1 *= sy;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform& Transform::rotate (float angle)
{
	//	| a0 b0 t0 |   | cos(a) -sin(a) 0 |   | a0*cos(a) + b0*sin(a)  -a0*sin(a) + b0*cos(a)  t0 |
	//	| a1 b1 t1 | x | sin(a)  cos(a) 0 | = | a1*cos(a) + b1*sin(a)  -a1*sin(a) + b1*cos(a)  t1 |
	//	|  0  0  1 |   |   0       0    1 |   |           0                       0             1 |

	float cosa = cosf (angle);
	float sina = sinf (angle);
	float a0New = a0 * cosa + b0 * sina;
	float a1New = a1 * cosa + b1 * sina;
	b0 = b0 * cosa - a0 * sina;
	b1 = b1 * cosa - a1 * sina;
	a0 = a0New;
	a1 = a1New;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform& Transform::skewX (float angle)
{
	//	| a0 b0 t0 |   | 1 tan(a) 0 |   | a0  a0*tan(a) + b0  t0 |
	//	| a1 b1 t1 | x | 0   1    0 | = | a1  a1*tan(a) + b1  t1 |
	//	|  0  0  1 |   | 0   0    1 |   |  0           0       1 |

	float tana = tanf (angle);
	b0 += a0 * tana;
	b1 += a1 * tana;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform& Transform::skewY (float angle)
{
	//	| a0 b0 t0 |   |   1    0 0 |   | a0 + b0*tan(a)  b0  t0 |
	//	| a1 b1 t1 | x | tan(a) 1 0 | = | a1 + b1*tan(a)  b1  t1 |
	//	|  0  0  1 |   |   0    0 1 |   |       0          0   1 |

	float tana = tanf (angle);
	a0 += b0 * tana;
	a1 += b1 * tana;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform& Transform::multiply (const Transform& t)
{
	//	| a0 b0 t0 |   | t.a0 t.b0 t.t0 |   | a0*t.a0 + b0*t.a1  a0*t.b0 + b0*t.b1  a0*t.t0 + b0*t.t1 + t0 |
	//	| a1 b1 t1 | x | t.a1 t.b1 t.t1 | = | a1*t.a0 + b1*t.a1  a1*t.b0 + b1*t.b1  a1*t.t0 + b1*t.t1 + t1 |
	//	|  0  0  1 |   |  0    0    1   |   |         0                  0                      1          |

	t0 = a0 * t.t0 + b0 * t.t1 + t0;
	t1 = a1 * t.t0 + b1 * t.t1 + t1;
	float a0New = a0 * t.a0 + b0 * t.a1;
	float a1New = a1 * t.a0 + b1 * t.a1;
	b0 = a0 * t.b0 + b0 * t.b1;
	b1 = a1 * t.b0 + b1 * t.b1;
	a0 = a0New;
	a1 = a1New;
	return *this;
}
