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
// Filename    : ccl/public/math/mathprimitives.cpp
// Description : Mathematical primitives
//
//************************************************************************************************

#include "ccl/public/math/mathprimitives.h"

namespace CCL {
namespace Math {

//************************************************************************************************
// Mathematical constants
//************************************************************************************************

#define PI				3.1415926535897932384626433832795
#define TWOPI			6.283185307179586476925286766559
#define HALFPI			1.5707963267948966192313216916398
#define PIINV			0.31830988618379067153776752674503
#define TWOPIINV		0.15915494309189533576888376337251
#define HALFPIINV		0.63661977236758134307553505349006
#define E				2.71828182845904523536
#define SQRTTWO			1.4142135623730951
#define SQRTTWOINV		0.70710678118654746
#define ANTIDENORMAL	1e-20

const float Constants<float>::kPi = (float)PI;
const float Constants<float>::kTwoPi = (float)TWOPI;
const float Constants<float>::kHalfPi = (float)HALFPI;
const float Constants<float>::kPiInv = (float)PIINV;
const float Constants<float>::kTwoPiInv = (float)TWOPIINV;
const float Constants<float>::kHalfPiInv = (float)HALFPIINV;
const float Constants<float>::kE = (float)E;
const float Constants<float>::kSqrtTwo = (float)SQRTTWO;
const float Constants<float>::kSqrtTwoInv = (float)SQRTTWOINV;							
const float Constants<float>::kAntiDenormal = (float)ANTIDENORMAL;

const double Constants<double>::kPi = PI;
const double Constants<double>::kTwoPi = TWOPI;
const double Constants<double>::kHalfPi = HALFPI;
const double Constants<double>::kPiInv = PIINV;
const double Constants<double>::kTwoPiInv = TWOPIINV;
const double Constants<double>::kHalfPiInv = HALFPIINV;
const double Constants<double>::kE = E;
const double Constants<double>::kSqrtTwo = SQRTTWO;
const double Constants<double>::kSqrtTwoInv = SQRTTWOINV;					
const double Constants<double>::kAntiDenormal =  ANTIDENORMAL;

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Math
} // namespace CCL
