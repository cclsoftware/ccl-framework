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
// Filename    : ccl/public/base/primitives.cpp
// Description : Primitives
//
//************************************************************************************************

#include "ccl/public/base/primitives.h"

namespace CCL {

//************************************************************************************************
// Numerical Limits
//************************************************************************************************

namespace NumericLimits
{
	const uint8 kMaxUnsignedInt8 = 0xff;
	const int8 kMaxInt8 = 127;
	const int8 kMinInt8 = -128;

	const uint16 kMaxUnsignedInt16 = 0xffff;
	const int16 kMaxInt16 = 32767;
	const int16 kMinInt16 = -32768;

	const uint32 kMaxUnsignedInt32 = 0xffffffff;
	const int32 kMaxInt32 = 2147483647;
	const int32 kMinInt32 = -2147483648LL;
	
	const unsigned int kMaxUnsignedInt = kMaxUnsignedInt32;
	const int kMaxInt = kMaxInt32;
	const int kMinInt = kMinInt32;

	const uint64 kMaxUnsignedInt64 = 0xffffffffffffffffULL;
	const int64 kMaxInt64 = 9223372036854775807LL;
	const int64 kMinInt64 = -9223372036854775807LL - 1;

	const float kMaximumFloat = 3.402823466e+38F;
	const float kMinimumFloat = 1.175494351e-38F;

	const double kMaximumDouble = 1.7976931348623157e+308;
	const double kMinimumDouble = 2.2250738585072014e-308;

	const double kLargeDouble = 1.7976931348623157e+200;
	const double kSmallDouble = 2.2250738585072014e-200;

	const double kPrecision = 1e-12;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL
