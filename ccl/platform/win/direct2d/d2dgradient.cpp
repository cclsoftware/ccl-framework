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
// Filename    : ccl/platform/win/direct2d/d2dgradient.cpp
// Description : Direct2D Gradient
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dgradient.h"
#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/d2dinterop.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DGradientBuilder
//************************************************************************************************

ID2D1GradientStopCollection* D2DGradientBuilder::createStopCollection (const IGradient::Stop stops[], int stopCount)
{
	ASSERT (stopCount <= kMaxStopCount)
	stopCount = ccl_min (stopCount, kMaxStopCount);

	D2D1_GRADIENT_STOP d2dStops[kMaxStopCount] = {0};
	for(int i = 0; i < stopCount; i++)
	{
		d2dStops[i].color = D2DInterop::toColorF (stops[i].color);
		d2dStops[i].position = stops[i].position;
	}

	ComPtr<ID2D1GradientStopCollection> gradientStopCollection;
	ID2D1DeviceContext* deviceContext = DXGIEngine::instance ().getDirect2dDeviceContext ();
	HRESULT hr = deviceContext->CreateGradientStopCollection (d2dStops, stopCount, gradientStopCollection);
	ASSERT (SUCCEEDED (hr))
	return gradientStopCollection.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1LinearGradientBrush* D2DGradientBuilder::createLinearBrush (PointFRef startPoint, PointFRef endPoint,
																 ID2D1GradientStopCollection* stops)
{
	ComPtr<ID2D1LinearGradientBrush> brush;	
	D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = {D2DInterop::fromCCL (startPoint), D2DInterop::fromCCL (endPoint)};
	ID2D1DeviceContext* deviceContext = DXGIEngine::instance ().getDirect2dDeviceContext ();
	HRESULT hr = deviceContext->CreateLinearGradientBrush (props, stops, brush);
	ASSERT (SUCCEEDED (hr))
	return brush.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1RadialGradientBrush* D2DGradientBuilder::createRadialBrush (PointFRef center, float radius, 
																 ID2D1GradientStopCollection* stops)
{
	ComPtr<ID2D1RadialGradientBrush> brush;	
	D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props = {D2DInterop::fromCCL (center), {0,0}, radius, radius};
	ID2D1DeviceContext* deviceContext = DXGIEngine::instance ().getDirect2dDeviceContext ();
	HRESULT hr = deviceContext->CreateRadialGradientBrush (props, stops, brush);
	ASSERT (SUCCEEDED (hr))
	return brush.detach ();
}

//************************************************************************************************
// D2DGradient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (D2DGradient, NativeGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DGradient::isValid () const
{
	// Brush is a device-dependent resource that can be discarded during D2D error handling
	return brush.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DGradient::discardDirect2dResource (bool isShutdown)
{
	brush.release ();
}

//************************************************************************************************
// D2DLinearGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D2DLinearGradient, D2DGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////
 
tresult CCL_API D2DLinearGradient::construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
											  IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented

	ComPtr<ID2D1GradientStopCollection> stopCollection;	
	stopCollection = D2DGradientBuilder::createStopCollection (stops, stopCount);
	brush = D2DGradientBuilder::createLinearBrush (startPoint, endPoint, stopCollection);	

	setRegistered (true); // register for D2D error handling
	return kResultOk;
}

//************************************************************************************************
// D2DRadialGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D2DRadialGradient, D2DGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DRadialGradient::construct (PointFRef center, float radius, const Stop stops[], int stopCount,
											  IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented

	ComPtr<ID2D1GradientStopCollection> stopCollection;	
	stopCollection = D2DGradientBuilder::createStopCollection (stops, stopCount);
	brush = D2DGradientBuilder::createRadialBrush (center, radius, stopCollection);
	
	setRegistered (true); // register for D2D error handling
	return kResultOk;
}
