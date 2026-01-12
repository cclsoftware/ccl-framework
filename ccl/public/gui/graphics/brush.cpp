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
// Filename    : ccl/public/gui/graphics/brush.cpp
// Description : Brush definition
//
//************************************************************************************************

#include "ccl/public/gui/graphics/brush.h"
#include "ccl/public/gui/graphics/igraphicshelper.h"

using namespace CCL;

//************************************************************************************************
// LinearGradientBrush
//************************************************************************************************

LinearGradientBrush::LinearGradientBrush (PointFRef startPoint, PointFRef endPoint,
										  ColorRef startColor, ColorRef endColor)
{
	IGradient::Stop stops[2] = {{0.f, startColor}, {1.f, endColor}};
	construct (startPoint, endPoint, stops, 2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinearGradientBrush::LinearGradientBrush (PointFRef startPoint, PointFRef endPoint, 
										  const IGradient::Stop stops[], int stopCount)
{
	construct (startPoint, endPoint, stops, stopCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinearGradientBrush::LinearGradientBrush (PointFRef startPoint, PointFRef endPoint,
										  GradientBrushRef other)
{
	construct (startPoint, endPoint, nullptr, 0, other.getGradient ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinearGradientBrush::construct (PointFRef startPoint, PointFRef endPoint, 
									 const IGradient::Stop stops[], int stopCount, 
									 IGradient* other)
{
	ASSERT (gradient == nullptr)
	gradient = System::GetGraphicsHelper ().Factory_createGradient (IGradient::kLinearGradient);
	UnknownPtr<ILinearGradient> linearGradient (gradient);
	ASSERT (linearGradient.isValid ())
	if(linearGradient)
		linearGradient->construct (startPoint, endPoint, stops, stopCount, other);
}

//************************************************************************************************
// RadialGradientBrush
//************************************************************************************************

RadialGradientBrush::RadialGradientBrush (PointFRef center, float radius, 
										  ColorRef startColor, ColorRef endColor)
{
	IGradient::Stop stops[2] = {{0.f, startColor}, {1.f, endColor}};
	construct (center, radius, stops, 2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RadialGradientBrush::RadialGradientBrush (PointFRef center, float radius,
										  const IGradient::Stop stops[], int stopCount)
{
	construct (center, radius, stops, stopCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RadialGradientBrush::RadialGradientBrush (PointFRef center, float radius,
										  GradientBrushRef other)
{
	construct (center, radius, nullptr, 0, other.getGradient ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadialGradientBrush::construct (PointFRef center, float radius, 
									 const IGradient::Stop stops[], int stopCount,
									 IGradient* other)
{
	ASSERT (gradient == nullptr)
	gradient = System::GetGraphicsHelper ().Factory_createGradient (IGradient::kRadialGradient);
	UnknownPtr<IRadialGradient> radialGradient (gradient);
	ASSERT (radialGradient.isValid ())
	if(radialGradient)
		radialGradient->construct (center, radius, stops, stopCount, other);
}
