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
// Filename    : ccl/platform/win/direct2d/dxgiengine.h
// Description : DXGI (DirectX Graphics Infrastructure) Engine
//
//************************************************************************************************

#ifndef _ccl_dxgiengine_h
#define _ccl_dxgiengine_h

#include "ccl/platform/win/direct2d/d2dinterop.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include <d3d11_1.h>

namespace CCL {

struct BitmapLockData;

namespace Win32 {

//************************************************************************************************
// DXGIEngine
//************************************************************************************************

class DXGIEngine: public Object,
				  public StaticSingleton<DXGIEngine>
{
public:
	DXGIEngine ();
	~DXGIEngine ();

	ID3D11Device* getDirect3dDevice ()		{ return direct3dDevice; }
	IDXGIDevice* getDXGIDevice ()			{ return dxgiDevice; }
	ID2D1Factory1* getDirect2dFactory ()	{ return direct2dFactory; }
	ID2D1Device* getDirect2dDevice ()       { return direct2dDevice; }

	ID3D11DeviceContext1* getDirect3dDeviceContext ()	{ return direct3dDeviceContext; }
	ID2D1DeviceContext* getDirect2dDeviceContext ()		{ return direct2dDeviceContext; }
	
	bool startup ();
	void shutdown ();
	void handleDeviceLost ();
	void dumpFeatureSupport ();

	PROPERTY_VARIABLE (HRESULT, lastError, LastError)
	void reportError (CStringPtr message, HRESULT hr, bool warn);

	void beginDraw ();
	HRESULT endDraw ();
	PROPERTY_POINTER (D2DClientRenderDevice, currentClientDevice, CurrentClientDevice)
	ID2D1SolidColorBrush* getPrimaryBrush (ColorRef color);
	ID2D1StrokeStyle* getStrokeStyle (int penStyle);

	PROPERTY_BOOL (gdiCompatible, GdiCompatible)
	PROPERTY_BOOL (flipModelEnabled, FlipModelEnabled)
	bool isFlipModel () const;
	UINT getSwapChainFlags () const;
	IDXGISwapChain1* createSwapChainForWindow (HWND hwnd);
	ID2D1Bitmap1* createBitmapForSwapChain (IDXGISwapChain1* swapChain);
	ID2D1Bitmap1* takeScreenshot (HWND hwnd, IDXGISwapChain1* swapChain);
	ID2D1Bitmap1* takeScreenshot (IDXGIOutput1* output, const D2D_RECT_U& screenRect);
		
	D2D1_BITMAP_PROPERTIES1 getBitmapProperties (ID2D1Bitmap1* bitmap);
	ID2D1Bitmap1* createBitmap (PointRef sizeInPixel, bool alphaChannelUsed, bool isTarget, float scaleFactor);
	ID2D1Bitmap1* createBitmapWithOptions (ID2D1Bitmap1* sourceBitmap, D2D1_BITMAP_OPTIONS desiredOptions, bool mustCopy = false);
	ID2D1Bitmap1* createBitmapForCPUReadAccess (ID2D1Bitmap1* sourceBitmap);

	bool clearBitmap (ID2D1Bitmap1* bitmap);
	bool lockBitmap (BitmapLockData& data, ID2D1Bitmap1* bitmap, int mode);
	bool unlockBitmap (BitmapLockData& data);
	ID2D1Bitmap1* getScratchBitmap (const D2D1_SIZE_U& size, const D2D1_PIXEL_FORMAT& format);
	bool scrollBitmap (ID2D1Bitmap1* bitmap, RectRef rect, PointRef delta);
	bool copyFromWICBitmap (ID2D1Bitmap1* bitmap, IWICBitmap* wicBitmap);

protected:
    ComPtr<ID3D11Device> direct3dDevice;
	ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<ID3D11DeviceContext1> direct3dDeviceContext;

	ComPtr<ID2D1Factory1> direct2dFactory;
	ComPtr<ID2D1Device> direct2dDevice;
	ComPtr<ID2D1DeviceContext> direct2dDeviceContext;
	
	ComPtr<ID2D1SolidColorBrush> primaryBrush;
	CCL::ObjectArray strokeStyles;
	
	Color primaryColor;
	int beginDrawCount;
	ComPtr<ID2D1Bitmap1> tempAlphaBitmap;
	ComPtr<ID2D1Bitmap1> tempNonAlphaBitmap;
	bool warningShown;

	class StrokeStyle;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// DXGIEngine inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ID2D1SolidColorBrush* DXGIEngine::getPrimaryBrush (ColorRef color)
{
	if(color != primaryColor)
	{
		primaryBrush->SetColor (D2DInterop::toColorF (color));
		primaryColor = color;
	}
	return primaryBrush;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Win32
} // namespace CCL

#endif // _ccl_dxgiengine_h
