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
// Filename    : ccl/platform/android/graphics/androidgradient.h
// Description : Android Gradient
//
//************************************************************************************************

#ifndef _ccl_androidgradient_h
#define _ccl_androidgradient_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// AndroidGradient
//************************************************************************************************

class AndroidGradient: public NativeGradient
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidGradient, NativeGradient)

	virtual jobject getPaint () = 0;

protected:
	FixedSizeVector<jint, kMaxStopCount> colors;
	FixedSizeVector<jfloat, kMaxStopCount> positions;

	void initStops (const Stop stops[], int stopCount);
};

//************************************************************************************************
// AndroidLinearGradient
//************************************************************************************************

class AndroidLinearGradient: public AndroidGradient,
							 public ILinearGradient
{
public:
	DECLARE_CLASS (AndroidLinearGradient, AndroidGradient)

	// AndroidGradient
	jobject getPaint () override;

	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	CLASS_INTERFACE (ILinearGradient, AndroidGradient)

protected:
	JniObject paint;
	PointF startPoint;
	PointF endPoint;
};

//************************************************************************************************
// AndroidRadialGradient
//************************************************************************************************

class AndroidRadialGradient: public AndroidGradient,
							 public IRadialGradient
{
public:
	DECLARE_CLASS (AndroidRadialGradient, AndroidGradient)

	// AndroidGradient
	jobject getPaint () override;

	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	CLASS_INTERFACE (IRadialGradient, AndroidGradient)

protected:
	JniObject paint;
	PointF center;
	float radius;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_androidgradient_h
