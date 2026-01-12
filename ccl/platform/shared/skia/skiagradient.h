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
// Filename    : ccl/platform/shared/skia/skiagradient.h
// Description : Skia Gradient
//
//************************************************************************************************

#ifndef _ccl_skia_gradient_h
#define _ccl_skia_gradient_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {

//************************************************************************************************
// SkiaGradient
//************************************************************************************************

class SkiaGradient: public NativeGradient
{
public:	
	DECLARE_CLASS_ABSTRACT (SkiaGradient, NativeGradient)

	SkiaGradient ();
	
	sk_sp<SkShader> getGradientShader () const;
	
protected:
	mutable sk_sp<SkShader> shader;
	mutable bool needsUpdate;
	
	FixedSizeVector<SkColor4f, kMaxStopCount> stopColors;
	FixedSizeVector<SkScalar, kMaxStopCount> stopPositions;
	
	void setStops (const Stop stops[], int stopCount);
	virtual void updateShader () const = 0;
};

//************************************************************************************************
// SkiaLinearGradient
//************************************************************************************************

class SkiaLinearGradient: public SkiaGradient,
						  public ILinearGradient
{
public:
	DECLARE_CLASS (SkiaLinearGradient, SkiaGradient)

	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	CLASS_INTERFACE (ILinearGradient, SkiaGradient)

protected:
	SkPoint startPoint;
	SkPoint endPoint;
	
	// SkiaGradient
	void updateShader () const override;
};

//************************************************************************************************
// SkiaRadialGradient
//************************************************************************************************

class SkiaRadialGradient: public SkiaGradient,
						  public IRadialGradient
{
public:
	DECLARE_CLASS (SkiaRadialGradient, SkiaGradient)

	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	CLASS_INTERFACE (IRadialGradient, SkiaGradient)

protected:
	SkPoint center;
	SkScalar radius;
	
	// SkiaGradient
	void updateShader () const override;
};

} // namespace CCL

#endif // _ccl_skia_gradient_h
