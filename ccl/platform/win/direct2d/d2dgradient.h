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
// Filename    : ccl/platform/win/direct2d/d2dgradient.h
// Description : Direct2D Gradient
//
//************************************************************************************************

#ifndef _ccl_direct2d_gradient_h
#define _ccl_direct2d_gradient_h

#include "ccl/platform/win/direct2d/d2dbase.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/collections/vector.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DGradientBuilder
//************************************************************************************************

namespace D2DGradientBuilder
{
	static constexpr int kMaxStopCount = NativeGradient::kMaxStopCount;
	static ID2D1GradientStopCollection* createStopCollection (const IGradient::Stop stops[], int stopCount);
	static ID2D1LinearGradientBrush* createLinearBrush (PointFRef startPoint, PointFRef endPoint, 
														ID2D1GradientStopCollection* stops);
	static ID2D1RadialGradientBrush* createRadialBrush (PointFRef center, float radius, 
														ID2D1GradientStopCollection* stops);
}

//************************************************************************************************
// D2DGradient
//************************************************************************************************

class D2DGradient: public NativeGradient,
				   public D2DResource
{
public:
	DECLARE_CLASS_ABSTRACT (D2DGradient, NativeGradient)

	ID2D1Brush* getD2DBrush () { return brush; }

	// NativeGradient
	bool isValid () const override;

	// D2DResource
	void discardDirect2dResource (bool isShutdown) override;

protected:
	ComPtr<ID2D1Brush> brush;
};

//************************************************************************************************
// D2DLinearGradient
//************************************************************************************************

class D2DLinearGradient: public D2DGradient,
						 public ILinearGradient
{
public:
	DECLARE_CLASS (D2DLinearGradient, D2DGradient)

	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	CLASS_INTERFACE (ILinearGradient, D2DGradient)
};

//************************************************************************************************
// D2DRadialGradient
//************************************************************************************************

class D2DRadialGradient: public D2DGradient,
						 public IRadialGradient
{
public:
	DECLARE_CLASS (D2DRadialGradient, D2DGradient)

	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	CLASS_INTERFACE (IRadialGradient, D2DGradient)
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_gradient_h
