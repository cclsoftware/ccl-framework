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
// Filename    : ccl/platform/shared/skia/skiagradient.cpp
// Description : Skia Gradient
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiagradient.h"
#include "ccl/platform/shared/skia/skiadevice.h"

using namespace CCL;

//************************************************************************************************
// SkiaGradient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaGradient, NativeGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaGradient::SkiaGradient ()
: needsUpdate (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkShader> SkiaGradient::getGradientShader () const
{
	if(needsUpdate)
		updateShader ();
	return shader;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaGradient::setStops (const Stop stops[], int stopCount)
{
	ASSERT (stopCount <= kMaxStopCount)
	stopCount = ccl_min (stopCount, kMaxStopCount);

	stopColors.setCount (stopCount);
	stopPositions.setCount (stopCount);
	for(int i = 0; i < stopCount; i++)
	{
		stopColors[i].fR = stops[i].color.getRedF ();
		stopColors[i].fG = stops[i].color.getGreenF ();
		stopColors[i].fB = stops[i].color.getBlueF ();
		stopColors[i].fA = stops[i].color.getAlphaF ();
		
		stopPositions[i] = stops[i].position;
	}
}

//************************************************************************************************
// SkiaLinearGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaLinearGradient, SkiaGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLinearGradient::construct (PointFRef start, PointFRef end, const Stop stops[], int stopCount,
											   IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented

	SkiaDevice::toSkPoint (startPoint, start);
	SkiaDevice::toSkPoint (endPoint, end);
	setStops (stops, stopCount);
	needsUpdate = true;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLinearGradient::updateShader () const
{
	SkPoint points[2] { startPoint, endPoint };
	shader = SkGradientShader::MakeLinear (points, stopColors, SkColorSpace::MakeSRGB (), stopPositions, stopPositions.count (), SkTileMode::kClamp);
	needsUpdate = false;
}

//************************************************************************************************
// SkiaRadialGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaRadialGradient, SkiaGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaRadialGradient::construct (PointFRef position, float r, const Stop stops[], int stopCount,
											   IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented

	SkiaDevice::toSkPoint (center, position);
	radius = r;
	setStops (stops, stopCount);
	needsUpdate = true;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaRadialGradient::updateShader () const
{
	shader = SkGradientShader::MakeRadial (center, radius, stopColors, SkColorSpace::MakeSRGB (), stopPositions, stopPositions.count (), SkTileMode::kClamp);
	needsUpdate = false;
}
