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
// Filename    : ccl/platform/cocoa/quartz/gradient.h
// Description : Quartz Gradient
//
//************************************************************************************************

#ifndef _ccl_quartz_gradient_h
#define _ccl_quartz_gradient_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/cocoa/macutils.h"

#include <CoreGraphics/CGGradient.h>

namespace CCL {
namespace MacOS {

//************************************************************************************************
// QuartzGradient
//************************************************************************************************

class QuartzGradient: public NativeGradient
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzGradient, NativeGradient)

	virtual void draw (CGContextRef context) const = 0;
	
protected:
	CFObj<CGGradientRef> gradient;

	void construct (const Stop stops[], int stopCount);
};

//************************************************************************************************
// QuartzLinearGradient
//************************************************************************************************

class QuartzLinearGradient: public QuartzGradient,
							public ILinearGradient
{
public:
	DECLARE_CLASS (QuartzLinearGradient, QuartzGradient)

	// QuartzGradient
	void draw (CGContextRef context) const override;
	
	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	CLASS_INTERFACE (ILinearGradient, QuartzGradient)
	
protected:
	CGPoint startPoint;
	CGPoint endPoint;
};

//************************************************************************************************
// QuartzRadialGradient
//************************************************************************************************

class QuartzRadialGradient: public QuartzGradient,
							public IRadialGradient
{
public:
	DECLARE_CLASS (QuartzRadialGradient, QuartzGradient)

	// QuartzGradient
	void draw (CGContextRef context) const override;
	
	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	CLASS_INTERFACE (IRadialGradient, QuartzGradient)
	
protected:
	CGPoint center;
	CGFloat radius;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_quartz_gradient_h
