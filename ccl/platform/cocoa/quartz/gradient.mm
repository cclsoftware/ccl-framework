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
// Filename    : ccl/platform/cocoa/quartz/gradient.mm
// Description : Quartz Gradient
//
//************************************************************************************************

#include "ccl/platform/cocoa/quartz/gradient.h"
#include "ccl/platform/cocoa/quartz/device.h"

/*
	http://developer.apple.com/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_shadings/chapter_9_section_5.html
*/

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// QuartzGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (QuartzGradient, NativeGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzGradient::construct (const Stop stops[], int stopCount)
{
	ASSERT (stopCount <= kMaxStopCount)
	stopCount = ccl_min (stopCount, kMaxStopCount);

	CGFloat components[kMaxStopCount * 4];
	CGFloat locations[kMaxStopCount];
	for(int i = 0; i < stopCount; i++)
	{
		components[i * 4] = stops[i].color.getRedF ();
		components[i * 4 + 1] = stops[i].color.getGreenF ();
		components[i * 4 + 2] = stops[i].color.getBlueF ();
		components[i * 4 + 3] = stops[i].color.getAlphaF ();
		locations[i] = stops[i].position;
	}
			
	CFObj<CGColorSpaceRef> colorSpace = ::CGColorSpaceCreateDeviceRGB ();
	gradient = ::CGGradientCreateWithColorComponents (colorSpace, components, locations, stopCount);
}

//************************************************************************************************
// QuartzLinearGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (QuartzLinearGradient, QuartzGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzLinearGradient::construct (PointFRef start, PointFRef end, const Stop stops[], int stopCount,
									     IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented
	QuartzGradient::construct (stops, stopCount);
	
	startPoint.x = start.x;
	startPoint.y = start.y;
	
	endPoint.x = end.x;
	endPoint.y = end.y;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzLinearGradient::draw (CGContextRef context) const
{
	::CGContextDrawLinearGradient (context, gradient, startPoint, endPoint, 0);
}

//************************************************************************************************
// QuartzRadialGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (QuartzRadialGradient, QuartzGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzRadialGradient::construct (PointFRef point, float r, const Stop stops[], int stopCount,
									     IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented
	QuartzGradient::construct (stops, stopCount);

	center.x = point.x;
	center.y = point.y;

	radius = r;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzRadialGradient::draw (CGContextRef context) const
{
	::CGContextDrawRadialGradient (context, gradient, center, 0, center, radius, 0);
}
