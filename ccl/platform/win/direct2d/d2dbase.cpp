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
// Filename    : ccl/platform/win/direct2d/d2dbase.cpp
// Description : Direct2D Base Classes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/direct2d/d2dbase.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/platform/win/direct2d/d2dgradient.h"
#include "ccl/platform/win/system/system.win.h"

#include "ccl/public/gui/graphics/dpiscale.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DError
//************************************************************************************************

/*
	Direct2D Error Codes
	http://msdn.microsoft.com/en-us/library/windows/desktop/dd370979%28v=vs.85%29.aspx
*/

#if DEBUG
void D2DError::print (HRESULT hr)
{
	String message;
	Win32::formatSystemDebugMessage (message, hr);
	Debugger::println (message);			
}
#endif

//************************************************************************************************
// D2DRenderTarget
//************************************************************************************************

D2DRenderTarget::D2DRenderTarget (ID2D1DeviceContext* ownDeviceContext)
: engine (DXGIEngine::instance ()),
  ownDeviceContext (ownDeviceContext),
  oldClientDevice (nullptr),
  oldDpi (DpiScale::getDpi (1.f))
{
	engine.getDirect2dFactory ()->CreateDrawingStateBlock (oldDrawingState);
	ASSERT (oldDrawingState.isValid ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DRenderTarget::~D2DRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1DeviceContext* D2DRenderTarget::getContext ()
{
	// Note: ownDeviceContext is used for printing, everything else is sharing the global D2D context
	return ownDeviceContext ? ownDeviceContext : engine.getDirect2dDeviceContext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DRenderTarget::setActive (D2DClientRenderDevice& device, bool state)
{
	ASSERT (outputImage.isValid ())
	if(!outputImage)
		return;

	ID2D1DeviceContext* deviceContext = getContext ();
	if(state)
	{
		deviceContext->GetTarget (oldOutputImage);
		deviceContext->SaveDrawingState (oldDrawingState);

		oldClientDevice = engine.getCurrentClientDevice ();
		if(oldClientDevice)
			oldClientDevice->suspend (true);
		engine.setCurrentClientDevice (&device);

		deviceContext->GetDpi (&oldDpi, &oldDpi);
		deviceContext->SetTarget (outputImage);
		deviceContext->SetDpi (getDpi (), getDpi ());

		CCL_PRINTF ("D2D Render target set active new %p old %p\n", outputImage, oldOutputImage)
		
		this->target.share (deviceContext);
		this->target.as (this->gdiTarget);
	}
	else
	{
		this->target.release ();
		this->gdiTarget.release ();

		deviceContext->SetTarget (oldOutputImage);
		deviceContext->SetDpi (oldDpi, oldDpi);
		deviceContext->RestoreDrawingState (oldDrawingState);
		oldOutputImage.release ();

		engine.setCurrentClientDevice (oldClientDevice);
		if(oldClientDevice)
			oldClientDevice->suspend (false);

		CCL_PRINTF ("D2D Render target set inactive old %p\n", oldOutputImage)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DRenderTarget::beginDraw ()
{
	if(ownDeviceContext)
		ownDeviceContext->BeginDraw ();
	else
		engine.beginDraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DRenderTarget::endDraw ()
{
	if(ownDeviceContext)
	{
		HRESULT hr = ownDeviceContext->EndDraw ();
		if(FAILED (hr))
			ownDeviceContext->Flush ();
		return hr;
	}
	else
		return engine.endDraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DRenderTarget::isGdiCompatible () const
{
	return engine.isGdiCompatible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Brush* D2DRenderTarget::getBrushForColor (ColorRef color)
{
	return engine.getPrimaryBrush (color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Brush* D2DRenderTarget::getUnderlyingBrush (BrushRef brush)
{
	if(brush.getType () == Brush::kGradient)
		if(auto d2dGradient = NativeGradient::resolveTo<D2DGradient> (brush.getGradient ()))
			if(auto d2dBrush = d2dGradient->getD2DBrush ())
				return d2dBrush;

	ASSERT (brush.getType () == Brush::kSolid)
	return engine.getPrimaryBrush (brush.getColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Brush* D2DRenderTarget::getBrushForPen (PenRef pen)
{
	return engine.getPrimaryBrush (pen.getColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1StrokeStyle* D2DRenderTarget::getStyleForPen (PenRef pen)
{
	return engine.getStrokeStyle (pen.getStyle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
float D2DRenderTarget::getDpi () const
{
	return DpiScale::getDpi (getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2D1_TEXT_ANTIALIAS_MODE D2DRenderTarget::getDefaultTextAntialiasMode () const
{
	// disable ClearType rendering when drawing to a transparent surface
	// see https://docs.microsoft.com/windows/win32/direct2d/supported-pixel-formats-and-alpha-modes#cleartype-and-alpha-modes
	if(isAlphChannelUsed ())
		return D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;
	else
		return D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
}
